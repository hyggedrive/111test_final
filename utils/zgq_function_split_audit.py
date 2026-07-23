#!/usr/bin/env python3
"""Audit whether coarse executable ET_REL sections can be split by function.

This tool is read-only.  It does not modify input assembly, object files, or
linked ELFs.  The intended use is to estimate whether an LLVM LLD experiment
that virtually splits coarse executable input sections into function-level
InputSections is structurally safe enough to pursue.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple


BRANCH_MNEMONICS = {"beq", "bne", "blt", "bge", "bltu", "bgeu", "c.beqz", "c.bnez"}
DIRECT_JUMP_MNEMONICS = {"jal", "j", "c.j", "c.jal"}
CALL_MNEMONICS = {"call", "tail"}
INDIRECT_CALL_MNEMONICS = {"jalr", "c.jalr"}
INDIRECT_JUMP_MNEMONICS = {"jr", "c.jr"}
RETURN_MNEMONICS = {"ret", "c.ret"}
PRIVILEGED_RETURN_MNEMONICS = {"mret", "sret", "uret", "dret"}
TARGET_RELOCATION_PRIORITY = [
    "R_RISCV_CALL",
    "R_RISCV_CALL_PLT",
    "R_RISCV_JAL",
    "R_RISCV_BRANCH",
]
CONTROL_RELOCATION_TYPES = set(TARGET_RELOCATION_PRIORITY)
COMPLEX_SECTION_FLAGS = {"M": "SHF_MERGE", "O": "SHF_LINK_ORDER", "G": "SHF_GROUP"}


@dataclass
class Section:
    index: int
    name: str
    typ: str
    addr: int
    offset: int
    size: int
    flags: str
    link: int
    info: int

    @property
    def end(self) -> int:
        return self.addr + self.size

    @property
    def is_executable_progbits(self) -> bool:
        return self.typ == "PROGBITS" and "A" in self.flags and "X" in self.flags

    @property
    def is_group_or_comdat(self) -> bool:
        return "G" in self.flags

    @property
    def complex_flags(self) -> List[str]:
        return [name for flag, name in COMPLEX_SECTION_FLAGS.items() if flag in self.flags]


@dataclass
class Symbol:
    num: int
    value: int
    size: int
    typ: str
    bind: str
    vis: str
    ndx: str
    name: str
    section_index: Optional[int]

    @property
    def is_external_visible(self) -> bool:
        return self.bind in {"GLOBAL", "WEAK"} or self.vis not in {"DEFAULT", "HIDDEN", "INTERNAL"}


@dataclass
class Relocation:
    section_name: str
    target_section_index: Optional[int]
    offset: int
    typ: str
    symbol_value: Optional[int]
    symbol_name: str
    addend: int
    raw: str
    source_function: Optional[str] = None
    source_region: str = "unknown"
    target_kind: str = "unknown"
    target_function: Optional[str] = None
    unique_owner: bool = False


@dataclass
class Instruction:
    section_index: int
    address: int
    size: int
    raw: bytes
    mnemonic: str
    operands: str

    @property
    def end(self) -> int:
        return self.address + self.size


@dataclass
class FunctionInfo:
    object_path: str
    section_index: int
    section_name: str
    name: str
    aliases: List[str]
    start: int
    size: int
    end: int
    bind: str
    visibility: str
    valid_boundary: bool = True
    reasons: List[str] = field(default_factory=list)
    relocation_count: int = 0
    section_symbol_relocations: int = 0
    cross_function_relocations: int = 0
    local_label_relocations: int = 0
    unknown_control_flow_count: int = 0
    no_reloc_cross_edges: int = 0
    relocated_direct_calls: int = 0
    relocated_jals: int = 0
    relocated_conditional_branches: int = 0
    indirect_calls: int = 0
    indirect_jumps: int = 0
    computed_jump_blocked: int = 0
    unresolved_relocation_targets: int = 0
    status: str = "unknown"
    cluster_id: Optional[int] = None


@dataclass
class Edge:
    object_path: str
    section: str
    source_function: Optional[str]
    source_addr: int
    target_addr: Optional[int]
    target_region: str
    target_function: Optional[str]
    mnemonic: str
    has_relocation: bool
    edge_kind: str
    reason: str
    paired_jalr_addr: Optional[int] = None


@dataclass
class SectionAudit:
    object_path: str
    section: Section
    function_total: int
    sized_function_count: int
    zero_size_function_count: int
    out_of_bounds_count: int
    alias_groups: List[List[str]]
    overlap_count: int
    gap_bytes: int
    group_or_comdat: bool
    complex_flags: List[str]


class AuditError(RuntimeError):
    pass


def run_tool(cmd: Sequence[str]) -> str:
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.PIPE)
    except FileNotFoundError as exc:
        raise AuditError(f"missing tool: {cmd[0]}") from exc
    except subprocess.CalledProcessError as exc:
        msg = exc.stderr.strip() or exc.output.strip()
        raise AuditError(f"command failed: {' '.join(cmd)}\n{msg}") from exc


def parse_int_auto(text: str) -> int:
    text = text.strip()
    if text.lower().startswith("0x"):
        return int(text, 16)
    return int(text, 0)


def validate_elf_header(text: str, obj: Path) -> None:
    values: Dict[str, str] = {}
    for line in text.splitlines():
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        values[key.strip()] = value.strip()
    elf_class = values.get("Class", "")
    elf_type = values.get("Type", "")
    machine = values.get("Machine", "")
    if elf_class != "ELF32":
        raise AuditError(f"{obj}: expected ELF32 ET_REL object, got Class={elf_class or '<missing>'}")
    if not elf_type.startswith("REL"):
        raise AuditError(f"{obj}: expected ET_REL object, got Type={elf_type or '<missing>'}")
    if "RISC-V" not in machine and "RISCV" not in machine:
        raise AuditError(f"{obj}: expected RISC-V object, got Machine={machine or '<missing>'}")


def parse_sections(text: str, obj: Path) -> Dict[int, Section]:
    sections: Dict[int, Section] = {}
    sec_re = re.compile(
        r"^\s*\[\s*(\d+)\]\s+(\S+)\s+(\S+)\s+([0-9a-fA-F]+)\s+"
        r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\S+\s+(\S*)\s+(\d+)\s+(\d+)\s+\d+"
    )
    for line in text.splitlines():
        m = sec_re.match(line)
        if not m:
            continue
        idx = int(m.group(1))
        sections[idx] = Section(
            index=idx,
            name=m.group(2),
            typ=m.group(3),
            addr=int(m.group(4), 16),
            offset=int(m.group(5), 16),
            size=int(m.group(6), 16),
            flags=m.group(7),
            link=int(m.group(8)),
            info=int(m.group(9)),
        )
    if not sections:
        raise AuditError(f"{obj}: failed to parse any section from llvm-readelf -SW output")
    return sections


def parse_symbols(text: str, sections: Dict[int, Section], obj: Path) -> List[Symbol]:
    symbols: List[Symbol] = []
    sym_re = re.compile(
        r"^\s*(\d+):\s+([0-9a-fA-F]+)\s+(\d+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s*(.*)$"
    )
    for line in text.splitlines():
        m = sym_re.match(line)
        if not m:
            continue
        ndx = m.group(7)
        sec_idx = int(ndx) if ndx.isdigit() else None
        if sec_idx is not None and sec_idx not in sections:
            raise AuditError(f"{obj}: symbol references unknown section index {sec_idx}: {line}")
        symbols.append(
            Symbol(
                num=int(m.group(1)),
                value=int(m.group(2), 16),
                size=int(m.group(3)),
                typ=m.group(4),
                bind=m.group(5),
                vis=m.group(6),
                ndx=ndx,
                name=m.group(8).strip() or f"<anon:{m.group(1)}>",
                section_index=sec_idx,
            )
        )
    if not symbols:
        raise AuditError(f"{obj}: failed to parse any symbol from llvm-readelf -sW output")
    return symbols


def parse_relocations(text: str, sections: Dict[int, Section], obj: Path) -> List[Relocation]:
    relocs: List[Relocation] = []
    current_name = ""
    current_target: Optional[int] = None
    header_re = re.compile(r"Relocation section '([^']+)'")
    rel_re = re.compile(
        r"^\s*([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+(\S+)"
        r"(?:\s+([0-9a-fA-F]+)\s+(.+?))?\s*$"
    )
    reloc_target_by_name = {
        s.name: s.info
        for s in sections.values()
        if s.typ in {"REL", "RELA"} and s.info in sections
    }
    saw_reloc_header = False
    for line in text.splitlines():
        hm = header_re.search(line)
        if hm:
            saw_reloc_header = True
            current_name = hm.group(1)
            current_target = reloc_target_by_name.get(current_name)
            if current_target is None:
                raise AuditError(f"{obj}: relocation section {current_name} has no parsable sh_info target")
            continue
        if "There are no relocations" in line:
            return []
        m = rel_re.match(line)
        if not m:
            continue
        if current_target is None:
            raise AuditError(f"{obj}: relocation entry before relocation section header: {line}")
        tail = m.group(4).strip() if m.group(4) else ""
        addend = 0
        name = ""
        if tail:
            am = re.search(r"\s+([+-])\s+(0x[0-9a-fA-F]+|[0-9]+)\s*$", tail)
            if am:
                addend = parse_int_auto(am.group(2))
                if am.group(1) == "-":
                    addend = -addend
            name = re.split(r"\s+[+-]\s+", tail, maxsplit=1)[0].strip()
        relocs.append(
            Relocation(
                section_name=current_name,
                target_section_index=current_target,
                offset=int(m.group(1), 16),
                typ=m.group(2),
                symbol_value=int(m.group(3), 16) if m.group(3) else None,
                symbol_name=name,
                addend=addend,
                raw=line.strip(),
            )
        )
    if not relocs and saw_reloc_header:
        raise AuditError(f"{obj}: saw relocation section header but parsed no relocation entries")
    return relocs


def parse_raw_bytes(tokens: Sequence[str]) -> bytes:
    raw = bytearray()
    for token in tokens:
        if not re.fullmatch(r"[0-9a-fA-F]{2}|[0-9a-fA-F]{4}|[0-9a-fA-F]{8}", token):
            break
        if len(token) == 2:
            raw.append(int(token, 16))
        else:
            value = int(token, 16)
            for _ in range(len(token) // 2):
                raw.append(value & 0xFF)
                value >>= 8
    return bytes(raw)


def parse_objdump(text: str, sections: Dict[int, Section], obj: Path) -> List[Instruction]:
    instructions: List[Instruction] = []
    current_section_index: Optional[int] = None
    section_header_re = re.compile(r"^Disassembly of section (\S+):")
    insn_re = re.compile(r"^\s*([0-9a-fA-F]+):\s*(.*?)\s*$")
    name_to_index = {s.name: i for i, s in sections.items()}
    for line in text.splitlines():
        hm = section_header_re.match(line)
        if hm:
            current_section_index = name_to_index.get(hm.group(1))
            continue
        m = insn_re.match(line)
        if not m:
            continue
        if current_section_index is None:
            raise AuditError(f"{obj}: instruction appears before objdump section header: {line}")
        fields = m.group(2).split()
        raw_tokens: List[str] = []
        while fields and re.fullmatch(r"[0-9a-fA-F]{2}|[0-9a-fA-F]{4}|[0-9a-fA-F]{8}", fields[0]):
            raw_tokens.append(fields.pop(0))
        raw = parse_raw_bytes(raw_tokens)
        if not raw or not fields:
            raise AuditError(f"{obj}: failed to parse instruction bytes/asm: {line}")
        asm = " ".join(fields)
        parts = asm.split(None, 1)
        instructions.append(
            Instruction(
                section_index=current_section_index,
                address=int(m.group(1), 16),
                size=len(raw),
                raw=raw,
                mnemonic=parts[0].lower(),
                operands=parts[1].strip() if len(parts) > 1 else "",
            )
        )
    return instructions


def direct_cf_target(ins: Instruction) -> Optional[int]:
    if ins.mnemonic not in BRANCH_MNEMONICS and ins.mnemonic not in DIRECT_JUMP_MNEMONICS:
        return None
    # Prefer objdump's absolute target annotation: 00000020 <foo> or 0x20 <foo>.
    m = re.search(r"(?:^|[\s,])0x([0-9a-fA-F]+)(?:\s*<|$)", ins.operands)
    if m:
        return int(m.group(1), 16)
    m = re.search(r"(?:^|[\s,])([0-9a-fA-F]{4,})(?:\s*<|$)", ins.operands)
    if m:
        return int(m.group(1), 16)
    # LLVM sometimes prints decimal offsets less often for object files.
    m = re.search(r"(?:^|[\s,])([+-]?\d+)(?:\s*<|$)", ins.operands)
    if m:
        return int(m.group(1), 10)
    return None


def dedupe_inputs(paths: Iterable[Path]) -> List[Path]:
    seen: Set[Path] = set()
    out: List[Path] = []
    for path in paths:
        resolved = path.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        out.append(path)
    return out


def owner_for_range(functions: List[FunctionInfo], offset: int) -> Tuple[Optional[FunctionInfo], str]:
    owners = [fn for fn in functions if fn.start <= offset < fn.end]
    if len(owners) == 1:
        return owners[0], "function"
    if len(owners) > 1:
        return None, "ambiguous-function"
    return None, "padding"


def find_symbol(symbols: List[Symbol], name: str, value: Optional[int] = None) -> Optional[Symbol]:
    if name:
        for sym in symbols:
            if sym.name == name:
                return sym
    if value is not None:
        for sym in symbols:
            if sym.value == value:
                return sym
    return None


def select_primary_relocation(relocs_at_offset: List[Relocation]) -> Optional[Relocation]:
    non_relax = [r for r in relocs_at_offset if r.typ != "R_RISCV_RELAX"]
    if not non_relax:
        return None
    for typ in TARGET_RELOCATION_PRIORITY:
        for rel in non_relax:
            if rel.typ == typ:
                return rel
    for rel in non_relax:
        if rel.symbol_name or rel.symbol_value is not None:
            return rel
    return non_relax[0]


def select_control_relocation(relocs_at_offset: List[Relocation]) -> Optional[Relocation]:
    rel = select_primary_relocation(relocs_at_offset)
    if rel and rel.typ in CONTROL_RELOCATION_TYPES:
        return rel
    return None


def split_operands(operands: str) -> List[str]:
    return [part.strip() for part in operands.replace("\t", " ").split(",") if part.strip()]


def jalr_registers(ins: Instruction) -> Tuple[Optional[str], Optional[str]]:
    ops = split_operands(ins.operands)
    if ins.mnemonic == "c.jalr":
        return "ra", ops[0] if ops else None
    if ins.mnemonic == "c.jr":
        return "zero", ops[0] if ops else None
    if ins.mnemonic in {"jr", "ret"}:
        return "zero", ops[0] if ops else "ra"
    if ins.mnemonic != "jalr":
        return None, None
    if len(ops) == 1:
        return "ra", register_from_memory_operand(ops[0])
    if len(ops) == 2:
        return ops[0], register_from_memory_operand(ops[1])
    if len(ops) >= 3:
        return ops[0], ops[1]
    return None, None


def register_from_memory_operand(operand: str) -> str:
    m = re.search(r"\(([^)]+)\)", operand)
    if m:
        return m.group(1).strip()
    return operand.strip()


def is_return_instruction(ins: Instruction) -> bool:
    if ins.mnemonic in RETURN_MNEMONICS:
        return True
    rd, rs1 = jalr_registers(ins)
    return ins.mnemonic in {"jr", "c.jr"} and rs1 == "ra" and rd in {None, "zero"}


def relocation_target(
    rel: Relocation,
    symbols: List[Symbol],
    section_functions: Dict[int, List[FunctionInfo]],
) -> Tuple[str, Optional[int], Optional[FunctionInfo], Optional[Symbol]]:
    sym = find_symbol(symbols, rel.symbol_name, rel.symbol_value)
    if sym is None:
        return "unresolved", None, None, None
    if sym.section_index is None:
        return "external", None, None, sym
    target_addr = sym.value + rel.addend
    target_fn = None
    if sym.section_index in section_functions:
        target_fn, _ = owner_for_range(section_functions[sym.section_index], target_addr)
    return "internal", target_addr, target_fn, sym


def classify_target_region(
    section: Section,
    funcs: List[FunctionInfo],
    src_fn: Optional[FunctionInfo],
    target: int,
) -> Tuple[str, Optional[FunctionInfo]]:
    target_fn, target_region = owner_for_range(funcs, target)
    if target < section.addr or target >= section.end:
        return "section-out", None
    if target_fn and src_fn and target_fn.name == src_fn.name:
        return "same-function", target_fn
    if target_fn and src_fn and target_fn.name != src_fn.name:
        return "other-function", target_fn
    return target_region, target_fn


def build_section_functions(obj: Path, section: Section, symbols: List[Symbol]) -> Tuple[List[FunctionInfo], SectionAudit]:
    all_func_syms_in_sec = [s for s in symbols if s.typ == "FUNC" and s.section_index == section.index]
    by_range: Dict[Tuple[int, int], List[Symbol]] = {}
    by_start: Dict[int, List[Symbol]] = {}
    out_of_bounds = 0
    for sym in all_func_syms_in_sec:
        if sym.size == 0 or not (section.addr <= sym.value < section.end) or sym.value + sym.size > section.end:
            if sym.size != 0:
                out_of_bounds += 1
        if sym.size > 0:
            by_range.setdefault((sym.value, sym.value + sym.size), []).append(sym)
            by_start.setdefault(sym.value, []).append(sym)
    functions: List[FunctionInfo] = []
    represented_symbol_nums: Set[int] = set()
    for (start, end), group in sorted(by_range.items()):
        primary = sorted(group, key=lambda s: (s.bind != "GLOBAL", s.name))[0]
        aliases = sorted(s.name for s in group)
        represented_symbol_nums.update(s.num for s in group)
        fn = FunctionInfo(
            object_path=str(obj),
            section_index=section.index,
            section_name=section.name,
            name=primary.name,
            aliases=aliases,
            start=start,
            size=end - start,
            end=end,
            bind=primary.bind,
            visibility=primary.vis,
        )
        if not (section.addr <= start < end <= section.end):
            fn.valid_boundary = False
            fn.reasons.append("out-of-bounds-function")
        functions.append(fn)
    for sym in sorted(all_func_syms_in_sec, key=lambda s: (s.value, s.name, s.num)):
        if sym.num in represented_symbol_nums:
            continue
        reasons = []
        if sym.size == 0:
            reasons.append("zero-size-function")
        if not (section.addr <= sym.value < section.end) or (sym.size > 0 and sym.value + sym.size > section.end):
            reasons.append("out-of-bounds-function")
        functions.append(
            FunctionInfo(
                object_path=str(obj),
                section_index=section.index,
                section_name=section.name,
                name=sym.name,
                aliases=[sym.name],
                start=sym.value,
                size=sym.size,
                end=sym.value + sym.size,
                bind=sym.bind,
                visibility=sym.vis,
                valid_boundary=False,
                reasons=reasons or ["incomplete-function-boundary"],
                status="hard-blocked",
            )
        )
    zero_size = sum(1 for s in all_func_syms_in_sec if s.size == 0)
    alias_groups = [sorted(s.name for s in group) for group in by_start.values() if len(group) > 1]
    overlaps = 0
    sorted_ranges = sorted(functions, key=lambda f: (f.start, f.end))
    covered = 0
    cursor = section.addr
    for i, fn in enumerate(sorted_ranges):
        if fn.start > cursor:
            covered += fn.start - cursor
        cursor = max(cursor, fn.end)
        if i and fn.start < sorted_ranges[i - 1].end:
            overlaps += 1
            fn.valid_boundary = False
            fn.reasons.append("overlapping-function-range")
            sorted_ranges[i - 1].valid_boundary = False
            sorted_ranges[i - 1].reasons.append("overlapping-function-range")
    if cursor < section.end:
        covered += section.end - cursor
    audit = SectionAudit(
        object_path=str(obj),
        section=section,
        function_total=len(all_func_syms_in_sec),
        sized_function_count=sum(1 for s in all_func_syms_in_sec if s.size > 0),
        zero_size_function_count=zero_size,
        out_of_bounds_count=out_of_bounds,
        alias_groups=alias_groups,
        overlap_count=overlaps,
        gap_bytes=covered,
        group_or_comdat=section.is_group_or_comdat,
        complex_flags=section.complex_flags,
    )
    section_block_reasons = []
    if section.is_group_or_comdat:
        section_block_reasons.append("section-group-or-comdat")
    for flag_name in section.complex_flags:
        section_block_reasons.append(f"complex-section-flag-{flag_name}")
    if section_block_reasons:
        for fn in functions:
            fn.reasons.extend(section_block_reasons)
    return functions, audit


def empty_object_result(obj: Path) -> Dict[str, object]:
    result: Dict[str, object] = {
        "object": str(obj),
        "object_status": "no-executable-code",
        "executable_section_count": 0,
        "stt_func_total": 0,
        "stt_func_symbol_total": 0,
        "unique_function_range_total": 0,
        "valid_sized_functions": 0,
        "hard_blocked_functions": 0,
        "clustered_functions": 0,
        "independently_splittable_functions": 0,
        "hard_blocked_function_ranges": 0,
        "clustered_function_ranges": 0,
        "independently_splittable_function_ranges": 0,
        "independently_splittable_bytes": 0,
        "executable_bytes": 0,
        "section_symbol_relocations": 0,
        "cross_function_relocations": 0,
        "no_reloc_cross_function_edges": 0,
        "unknown_control_flow_count": 0,
        "relocated_direct_calls": 0,
        "relocated_jals": 0,
        "relocated_conditional_branches": 0,
        "indirect_calls": 0,
        "indirect_jumps": 0,
        "computed_jump_blocked": 0,
        "paired_call_jalr_skipped": 0,
        "zero_size_blocked_symbols": 0,
        "unresolved_relocation_targets": 0,
        "unowned_relocations": 0,
        "padding_relocations": 0,
        "sections": [],
        "functions": [],
        "edges": [],
        "clusters": [],
        "independently_splittable_byte_ratio": 0.0,
        "independently_splittable_function_ratio": 0.0,
    }
    return result


def analyze_object(obj: Path, readelf: str, objdump: str) -> Dict[str, object]:
    hdr_text = run_tool([readelf, "-hW", str(obj)])
    sec_text = run_tool([readelf, "-SW", str(obj)])
    sym_text = run_tool([readelf, "-sW", str(obj)])
    rel_text = run_tool([readelf, "-rW", str(obj)])

    validate_elf_header(hdr_text, obj)
    sections = parse_sections(sec_text, obj)
    symbols = parse_symbols(sym_text, sections, obj)
    relocs = parse_relocations(rel_text, sections, obj)
    exec_sections = [s for s in sections.values() if s.is_executable_progbits]
    if not exec_sections:
        return empty_object_result(obj)
    executable_bytes = sum(s.size for s in exec_sections)
    if executable_bytes == 0:
        return empty_object_result(obj)

    dis_text = run_tool([objdump, "-d", str(obj)])
    instructions = parse_objdump(dis_text, sections, obj)
    exec_func_symbols = [
        s for s in symbols
        if s.typ == "FUNC" and s.section_index is not None and sections[s.section_index].is_executable_progbits
    ]
    if not instructions:
        if not exec_func_symbols:
            return empty_object_result(obj)
        raise AuditError(f"{obj}: executable code exists but no instructions were parsed")

    all_functions: List[FunctionInfo] = []
    section_audits: List[SectionAudit] = []
    section_functions: Dict[int, List[FunctionInfo]] = {}
    for sec in exec_sections:
        funcs, audit = build_section_functions(obj, sec, symbols)
        all_functions.extend(funcs)
        section_functions[sec.index] = funcs
        section_audits.append(audit)

    functions_by_key = {(fn.section_index, fn.start, fn.end): fn for fn in all_functions}
    functions_by_name = {name: fn for fn in all_functions for name in fn.aliases}
    reloc_by_addr: Dict[Tuple[int, int], List[Relocation]] = {}
    section_symbol_relocs = 0
    cross_function_relocs = 0
    unowned_relocs = 0
    padding_relocs = 0
    unresolved_relocation_targets = 0
    section_level_reasons: Dict[int, Set[str]] = {}

    for rel in relocs:
        if rel.target_section_index not in section_functions:
            continue
        funcs = section_functions[rel.target_section_index]
        owner, region = owner_for_range(funcs, rel.offset)
        rel.source_region = region
        if owner:
            rel.source_function = owner.name
            rel.unique_owner = True
            owner.relocation_count += 1
        elif region == "padding":
            padding_relocs += 1
            section_level_reasons.setdefault(rel.target_section_index, set()).add("relocation-in-padding-gap")
        else:
            unowned_relocs += 1
            section_level_reasons.setdefault(rel.target_section_index, set()).add("relocation-not-uniquely-owned")
        target_sym = find_symbol(symbols, rel.symbol_name, rel.symbol_value)
        if target_sym and target_sym.typ == "SECTION":
            rel.target_kind = "section-symbol"
            section_symbol_relocs += 1
            if owner:
                owner.section_symbol_relocations += 1
                owner.reasons.append("complex-section-symbol-relocation")
            else:
                section_level_reasons.setdefault(rel.target_section_index, set()).add(
                    "complex-section-symbol-relocation"
                )
        elif target_sym:
            rel.target_kind = target_sym.typ
            if target_sym.section_index in section_functions:
                target_owner, target_region = owner_for_range(section_functions[target_sym.section_index], target_sym.value)
                if target_owner:
                    rel.target_function = target_owner.name
                    if owner and target_owner.name != owner.name:
                        cross_function_relocs += 1
                        owner.cross_function_relocations += 1
                    if owner and target_sym.bind == "LOCAL" and target_sym.typ == "NOTYPE" and target_owner.name == owner.name:
                        owner.local_label_relocations += 1
        else:
            unresolved_relocation_targets += 1
            rel.target_kind = "unresolved-symbol"
            if owner:
                owner.unresolved_relocation_targets += 1
                owner.reasons.append("unresolved-relocation-target")
        if not rel.unique_owner:
            if owner is None:
                for fn in funcs:
                    if rel.offset == fn.start:
                        fn.reasons.append("relocation-ambiguous-at-boundary")
            else:
                owner.reasons.append("relocation-not-uniquely-owned")
        reloc_by_addr.setdefault((rel.target_section_index, rel.offset), []).append(rel)

    for sec_index, reasons in section_level_reasons.items():
        for fn in section_functions.get(sec_index, []):
            fn.reasons.extend(sorted(reasons))

    edges: List[Edge] = []
    uf_parent: Dict[str, str] = {fn.name: fn.name for fn in all_functions}
    paired_call_jalr_addrs: Set[Tuple[int, int]] = set()
    paired_call_jalr_skipped = 0

    def find(x: str) -> str:
        while uf_parent[x] != x:
            uf_parent[x] = uf_parent[uf_parent[x]]
            x = uf_parent[x]
        return x

    def union(a: str, b: str) -> None:
        ra, rb = find(a), find(b)
        if ra != rb:
            uf_parent[rb] = ra

    for ins in instructions:
        if ins.section_index not in section_functions:
            continue
        funcs = section_functions[ins.section_index]
        src_fn, src_region = owner_for_range(funcs, ins.address)
        if (ins.section_index, ins.address) in paired_call_jalr_addrs:
            paired_call_jalr_skipped += 1
            continue
        relocs_here = reloc_by_addr.get((ins.section_index, ins.address), [])
        primary_reloc = select_control_relocation(relocs_here)
        call_pair_reloc = None
        if primary_reloc is None and ins.mnemonic in INDIRECT_CALL_MNEMONICS:
            call_pair_reloc = select_control_relocation(reloc_by_addr.get((ins.section_index, ins.address - 4), []))
            if call_pair_reloc and call_pair_reloc.typ not in {"R_RISCV_CALL", "R_RISCV_CALL_PLT"}:
                call_pair_reloc = None
        control_reloc = primary_reloc or call_pair_reloc
        has_target_reloc = control_reloc is not None

        if is_return_instruction(ins):
            continue

        if ins.mnemonic in PRIVILEGED_RETURN_MNEMONICS:
            if src_fn:
                src_fn.unknown_control_flow_count += 1
                edges.append(
                    Edge(str(obj), sections[ins.section_index].name, src_fn.name, ins.address, None,
                         "unknown", None, ins.mnemonic, False, "unknown", "unknown-control-flow")
                )
            continue

        if has_target_reloc and control_reloc:
            target_class, reloc_target, target_fn, target_sym = relocation_target(control_reloc, symbols, section_functions)
            edge_kind = "relocated-control-flow"
            reason = ""
            target_region = target_class
            target_name = target_fn.name if target_fn else None
            if control_reloc.typ in {"R_RISCV_CALL", "R_RISCV_CALL_PLT"}:
                edge_kind = "relocated-tail-call" if ins.mnemonic == "tail" else "relocated-call"
                if src_fn:
                    src_fn.relocated_direct_calls += 1
                if primary_reloc is control_reloc:
                    paired_call_jalr_addrs.add((ins.section_index, ins.address + 4))
            elif control_reloc.typ == "R_RISCV_JAL":
                edge_kind = "relocated-jal"
                if src_fn:
                    src_fn.relocated_jals += 1
            elif control_reloc.typ == "R_RISCV_BRANCH":
                edge_kind = "relocated-branch"
                if src_fn:
                    src_fn.relocated_conditional_branches += 1
            if target_class == "external":
                reason = "external-relocated-target"
                target_region = "external"
            elif target_class == "unresolved":
                reason = "unresolved-relocation-target"
                target_region = "unresolved"
                if src_fn:
                    src_fn.reasons.append("unresolved-relocation-target")
            elif reloc_target is not None:
                if target_sym and target_sym.section_index == ins.section_index:
                    target_region, target_fn = classify_target_region(
                        sections[ins.section_index], funcs, src_fn, reloc_target
                    )
                    target_name = target_fn.name if target_fn else None
                    if src_fn and target_fn and target_fn.name != src_fn.name:
                        cross_function_relocs += 1
                        src_fn.cross_function_relocations += 1
                        reason = "relocated-cross-function-control-flow"
                else:
                    target_region = "other-section"
            edges.append(
                Edge(
                    str(obj),
                    sections[ins.section_index].name,
                    src_fn.name if src_fn else None,
                    ins.address,
                    reloc_target,
                    target_region,
                    target_name,
                    ins.mnemonic,
                    True,
                    edge_kind,
                    reason,
                    ins.address + 4 if control_reloc.typ in {"R_RISCV_CALL", "R_RISCV_CALL_PLT"} else None,
                )
            )
            continue

        if ins.mnemonic in CALL_MNEMONICS:
            if src_fn:
                src_fn.indirect_calls += 1
            edges.append(
                Edge(
                    str(obj),
                    sections[ins.section_index].name,
                    src_fn.name if src_fn else None,
                    ins.address,
                    None,
                    "external-or-indirect",
                    None,
                    ins.mnemonic,
                    False,
                    "indirect-call",
                    "call-without-target-relocation",
                )
            )
            continue

        if ins.mnemonic in INDIRECT_CALL_MNEMONICS:
            if src_fn:
                rd, rs1 = jalr_registers(ins)
                if rd == "ra" or ins.mnemonic == "c.jalr":
                    src_fn.indirect_calls += 1
                    edge_kind = "indirect-call"
                    reason = "ordinary-indirect-call"
                elif rd == "zero":
                    src_fn.indirect_jumps += 1
                    src_fn.computed_jump_blocked += 1
                    src_fn.reasons.append("computed-indirect-jump")
                    edge_kind = "computed-indirect-jump"
                    reason = "computed-indirect-jump"
                else:
                    src_fn.unknown_control_flow_count += 1
                    src_fn.reasons.append("unknown-control-flow")
                    edge_kind = "unknown"
                    reason = "unknown-jalr-form"
            else:
                edge_kind = "indirect-call"
                reason = "ordinary-indirect-call"
            edges.append(
                Edge(
                    str(obj),
                    sections[ins.section_index].name,
                    src_fn.name if src_fn else None,
                    ins.address,
                    None,
                    "indirect",
                    None,
                    ins.mnemonic,
                    False,
                    edge_kind,
                    reason,
                )
            )
            continue

        if ins.mnemonic in INDIRECT_JUMP_MNEMONICS:
            if src_fn:
                rd, rs1 = jalr_registers(ins)
                src_fn.indirect_jumps += 1
                if rs1 != "ra":
                    src_fn.computed_jump_blocked += 1
                    src_fn.reasons.append("computed-indirect-jump")
                    edge_kind = "computed-indirect-jump"
                    reason = "computed-indirect-jump"
                else:
                    edge_kind = "indirect-control-flow"
                    reason = "ordinary-indirect-jump"
            else:
                edge_kind = "indirect-control-flow"
                reason = "ordinary-indirect-jump"
            edges.append(
                Edge(
                    str(obj),
                    sections[ins.section_index].name,
                    src_fn.name if src_fn else None,
                    ins.address,
                    None,
                    "indirect",
                    None,
                    ins.mnemonic,
                    False,
                    edge_kind,
                    reason,
                )
            )
            continue

        target = direct_cf_target(ins)
        if target is None:
            if ins.mnemonic in BRANCH_MNEMONICS or ins.mnemonic in DIRECT_JUMP_MNEMONICS:
                if src_fn:
                    src_fn.unknown_control_flow_count += 1
                    src_fn.reasons.append("unknown-control-flow")
                edges.append(
                    Edge(str(obj), sections[ins.section_index].name, src_fn.name if src_fn else None,
                         ins.address, None, "unknown", None, ins.mnemonic, False, "unknown",
                         "direct-control-target-not-parsed")
                )
            continue
        region, target_fn = classify_target_region(sections[ins.section_index], funcs, src_fn, target)
        target_name = target_fn.name if target_fn else None
        edge_kind = "direct-control-flow"
        reason = ""
        if src_fn and region == "other-function":
            reason = "no-reloc-cross-function-control-flow"
            src_fn.no_reloc_cross_edges += 1
            union(src_fn.name, target_fn.name)
        elif src_fn and region in {"padding", "section-out", "ambiguous-function"}:
            reason = f"control-flow-target-{region}"
            src_fn.unknown_control_flow_count += 1
            src_fn.reasons.append("unknown-control-flow")
        edges.append(
            Edge(str(obj), sections[ins.section_index].name, src_fn.name if src_fn else None, ins.address,
                 target, region, target_name, ins.mnemonic, False, edge_kind, reason)
        )

    clusters_by_root: Dict[str, List[str]] = {}
    for fn in all_functions:
        root = find(fn.name)
        clusters_by_root.setdefault(root, []).append(fn.name)
    clusters = [sorted(v) for v in clusters_by_root.values() if len(v) > 1]
    cluster_index: Dict[str, int] = {}
    for i, names in enumerate(clusters):
        for name in names:
            cluster_index[name] = i

    symbol_by_sec = [s for s in symbols if s.section_index in section_functions]
    for fn in all_functions:
        for sym in symbol_by_sec:
            if sym.typ == "FUNC":
                continue
            if fn.start < sym.value < fn.end and sym.is_external_visible:
                fn.reasons.append("external-visible-alternate-entry")

    for rel in relocs:
        if rel.target_section_index in section_functions and rel.source_region != "function":
            for fn in section_functions[rel.target_section_index]:
                if fn.start <= rel.offset <= fn.end:
                    fn.reasons.append("relocation-not-uniquely-owned")

    for fn in all_functions:
        fn.reasons = sorted(set(fn.reasons))
        if not fn.valid_boundary or fn.reasons:
            hard_reasons = [
                r for r in fn.reasons
                if r not in {"relocated-cross-function-control-flow"}
            ]
            if hard_reasons:
                fn.status = "hard-blocked"
            elif fn.name in cluster_index:
                fn.status = "clustered"
                fn.cluster_id = cluster_index[fn.name]
            else:
                fn.status = "independently-splittable"
        elif fn.name in cluster_index:
            fn.status = "clustered"
            fn.cluster_id = cluster_index[fn.name]
        else:
            fn.status = "independently-splittable"

    result = {
        "object": str(obj),
        "object_status": "analyzed",
        "executable_section_count": len(exec_sections),
        "stt_func_total": sum(a.function_total for a in section_audits),
        "stt_func_symbol_total": sum(a.function_total for a in section_audits),
        "unique_function_range_total": len(all_functions),
        "valid_sized_functions": sum(1 for fn in all_functions if fn.size > 0 and fn.valid_boundary),
        "hard_blocked_functions": sum(len(fn.aliases) for fn in all_functions if fn.status == "hard-blocked"),
        "clustered_functions": sum(len(fn.aliases) for fn in all_functions if fn.status == "clustered"),
        "independently_splittable_functions": sum(
            len(fn.aliases) for fn in all_functions if fn.status == "independently-splittable"
        ),
        "hard_blocked_function_ranges": sum(1 for fn in all_functions if fn.status == "hard-blocked"),
        "clustered_function_ranges": sum(1 for fn in all_functions if fn.status == "clustered"),
        "independently_splittable_function_ranges": sum(
            1 for fn in all_functions if fn.status == "independently-splittable"
        ),
        "independently_splittable_bytes": sum(fn.size for fn in all_functions if fn.status == "independently-splittable"),
        "executable_bytes": sum(s.size for s in exec_sections),
        "section_symbol_relocations": section_symbol_relocs,
        "cross_function_relocations": cross_function_relocs,
        "no_reloc_cross_function_edges": sum(fn.no_reloc_cross_edges for fn in all_functions),
        "unknown_control_flow_count": sum(fn.unknown_control_flow_count for fn in all_functions),
        "relocated_direct_calls": sum(fn.relocated_direct_calls for fn in all_functions),
        "relocated_jals": sum(fn.relocated_jals for fn in all_functions),
        "relocated_conditional_branches": sum(fn.relocated_conditional_branches for fn in all_functions),
        "indirect_calls": sum(fn.indirect_calls for fn in all_functions),
        "indirect_jumps": sum(fn.indirect_jumps for fn in all_functions),
        "computed_jump_blocked": sum(fn.computed_jump_blocked for fn in all_functions),
        "paired_call_jalr_skipped": paired_call_jalr_skipped,
        "zero_size_blocked_symbols": sum(a.zero_size_function_count for a in section_audits),
        "unresolved_relocation_targets": unresolved_relocation_targets,
        "unowned_relocations": unowned_relocs,
        "padding_relocations": padding_relocs,
        "sections": [section_audit_to_json(a) for a in section_audits],
        "functions": [function_to_json(fn) for fn in all_functions],
        "edges": [edge_to_json(e) for e in edges],
        "clusters": clusters,
    }
    result["independently_splittable_byte_ratio"] = (
        result["independently_splittable_bytes"] / result["executable_bytes"]
        if result["executable_bytes"] else 0.0
    )
    result["independently_splittable_function_ratio"] = (
        result["independently_splittable_functions"] / result["stt_func_total"]
        if result["stt_func_total"] else 0.0
    )
    status_symbol_total = (
        result["hard_blocked_functions"]
        + result["clustered_functions"]
        + result["independently_splittable_functions"]
    )
    if result["stt_func_total"] != status_symbol_total:
        raise AuditError(
            f"{obj}: STT_FUNC symbol total mismatch: stt_func_total={result['stt_func_total']} "
            f"status_sum={status_symbol_total} diff={result['stt_func_total'] - status_symbol_total}"
        )
    return result


def section_audit_to_json(a: SectionAudit) -> Dict[str, object]:
    return {
        "object": a.object_path,
        "section_index": a.section.index,
        "section_name": a.section.name,
        "section_size": a.section.size,
        "stt_func_total": a.function_total,
        "sized_stt_func_count": a.sized_function_count,
        "zero_size_function_count": a.zero_size_function_count,
        "out_of_bounds_function_count": a.out_of_bounds_count,
        "same_start_alias_groups": a.alias_groups,
        "overlap_count": a.overlap_count,
        "gap_padding_bytes": a.gap_bytes,
        "group_or_comdat": a.group_or_comdat,
        "complex_flags": a.complex_flags,
    }


def function_to_json(fn: FunctionInfo) -> Dict[str, object]:
    return {
        "object": fn.object_path,
        "section": fn.section_name,
        "name": fn.name,
        "aliases": fn.aliases,
        "start": fn.start,
        "size": fn.size,
        "end": fn.end,
        "bind": fn.bind,
        "visibility": fn.visibility,
        "status": fn.status,
        "cluster_id": fn.cluster_id,
        "reasons": fn.reasons,
        "relocation_count": fn.relocation_count,
        "section_symbol_relocations": fn.section_symbol_relocations,
        "cross_function_relocations": fn.cross_function_relocations,
        "local_label_relocations": fn.local_label_relocations,
        "unknown_control_flow_count": fn.unknown_control_flow_count,
        "no_reloc_cross_edges": fn.no_reloc_cross_edges,
        "relocated_direct_calls": fn.relocated_direct_calls,
        "relocated_jals": fn.relocated_jals,
        "relocated_conditional_branches": fn.relocated_conditional_branches,
        "indirect_calls": fn.indirect_calls,
        "indirect_jumps": fn.indirect_jumps,
        "computed_jump_blocked": fn.computed_jump_blocked,
        "unresolved_relocation_targets": fn.unresolved_relocation_targets,
    }


def edge_to_json(e: Edge) -> Dict[str, object]:
    return {
        "object": e.object_path,
        "section": e.section,
        "source_function": e.source_function,
        "source_addr": e.source_addr,
        "target_addr": e.target_addr,
        "target_region": e.target_region,
        "target_function": e.target_function,
        "mnemonic": e.mnemonic,
        "has_relocation": e.has_relocation,
        "edge_kind": e.edge_kind,
        "reason": e.reason,
        "paired_jalr_addr": e.paired_jalr_addr,
    }


def aggregate(results: List[Dict[str, object]]) -> Dict[str, object]:
    keys = [
        "executable_section_count",
        "stt_func_total",
        "stt_func_symbol_total",
        "unique_function_range_total",
        "valid_sized_functions",
        "hard_blocked_functions",
        "clustered_functions",
        "independently_splittable_functions",
        "hard_blocked_function_ranges",
        "clustered_function_ranges",
        "independently_splittable_function_ranges",
        "independently_splittable_bytes",
        "executable_bytes",
        "section_symbol_relocations",
        "cross_function_relocations",
        "no_reloc_cross_function_edges",
        "unknown_control_flow_count",
        "relocated_direct_calls",
        "relocated_jals",
        "relocated_conditional_branches",
        "indirect_calls",
        "indirect_jumps",
        "computed_jump_blocked",
        "paired_call_jalr_skipped",
        "zero_size_blocked_symbols",
        "unresolved_relocation_targets",
        "unowned_relocations",
        "padding_relocations",
    ]
    total = {k: sum(int(r[k]) for r in results) for k in keys}
    total["objects_without_executable_code"] = sum(
        1 for r in results if r.get("object_status") == "no-executable-code"
    )
    total["independently_splittable_byte_ratio"] = (
        total["independently_splittable_bytes"] / total["executable_bytes"]
        if total["executable_bytes"] else 0.0
    )
    total["independently_splittable_function_ratio"] = (
        total["independently_splittable_functions"] / total["stt_func_total"]
        if total["stt_func_total"] else 0.0
    )
    return total


def write_outputs(results: List[Dict[str, object]], out_dir: Path, name: str) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    txt_path = out_dir / f"{name}_function_split_audit.txt"
    summary_csv = out_dir / f"{name}_function_split_audit_summary.csv"
    functions_csv = out_dir / f"{name}_function_split_audit_functions.csv"
    edges_csv = out_dir / f"{name}_function_split_audit_edges.csv"
    json_path = out_dir / f"{name}_function_split_audit.json"

    total = aggregate(results)
    lines: List[str] = []
    lines.append(f"Function split audit: {name}")
    lines.append("")
    for r in results:
        lines.append(f"== {r['object']} ==")
        lines.append(f"object status: {r['object_status']}")
        lines.append(f"executable sections: {r['executable_section_count']}")
        lines.append(f"STT_FUNC symbol total: {r['stt_func_symbol_total']}")
        lines.append(f"unique function range total: {r['unique_function_range_total']}")
        lines.append(f"valid-sized function ranges: {r['valid_sized_functions']}")
        lines.append(f"hard-blocked functions (symbol-weighted): {r['hard_blocked_functions']}")
        lines.append(f"clustered functions (symbol-weighted): {r['clustered_functions']}")
        lines.append(
            f"independently-splittable functions (symbol-weighted): {r['independently_splittable_functions']}"
        )
        lines.append(f"hard-blocked function ranges: {r['hard_blocked_function_ranges']}")
        lines.append(f"clustered function ranges: {r['clustered_function_ranges']}")
        lines.append(f"independently-splittable function ranges: {r['independently_splittable_function_ranges']}")
        lines.append(f"independently-splittable bytes: {r['independently_splittable_bytes']}")
        lines.append(f"executable bytes: {r['executable_bytes']}")
        lines.append(f"independently-splittable byte ratio: {r['independently_splittable_byte_ratio']:.4f}")
        lines.append(f"independently-splittable function ratio: {r['independently_splittable_function_ratio']:.4f}")
        lines.append(f"section-symbol relocations: {r['section_symbol_relocations']}")
        lines.append(f"cross-function relocations: {r['cross_function_relocations']}")
        lines.append(f"no-reloc cross-function edges: {r['no_reloc_cross_function_edges']}")
        lines.append(f"unknown control-flow count: {r['unknown_control_flow_count']}")
        lines.append(f"relocated direct calls: {r['relocated_direct_calls']}")
        lines.append(f"relocated JALs: {r['relocated_jals']}")
        lines.append(f"relocated conditional branches: {r['relocated_conditional_branches']}")
        lines.append(f"indirect calls: {r['indirect_calls']}")
        lines.append(f"indirect jumps: {r['indirect_jumps']}")
        lines.append(f"computed jump blocked: {r['computed_jump_blocked']}")
        lines.append(f"paired call JALR skipped: {r['paired_call_jalr_skipped']}")
        lines.append(f"zero-size blocked symbols: {r['zero_size_blocked_symbols']}")
        lines.append(f"unresolved relocation targets: {r['unresolved_relocation_targets']}")
        lines.append("")
        lines.append("sections:")
        for sec in r["sections"]:
            lines.append(
                f"  [{sec['section_index']}] {sec['section_name']} size={sec['section_size']} "
                f"funcs={sec['stt_func_total']} sized={sec['sized_stt_func_count']} "
                f"zero={sec['zero_size_function_count']} gaps={sec['gap_padding_bytes']} "
                f"group={sec['group_or_comdat']} flags={','.join(sec['complex_flags']) or 'none'}"
            )
        blocked = [fn for fn in r["functions"] if fn["status"] == "hard-blocked"]
        lines.append("")
        lines.append("blocked functions:")
        for fn in blocked:
            lines.append(
                f"  {fn['section']}:{fn['name']} 0x{fn['start']:x}..0x{fn['end']:x} "
                f"size={fn['size']} reasons={','.join(fn['reasons']) or 'none'}"
            )
        if not blocked:
            lines.append("  none")
        lines.append("")
        lines.append("function clusters:")
        for idx, cluster in enumerate(r["clusters"]):
            lines.append(f"  cluster#{idx}: " + ", ".join(cluster))
        if not r["clusters"]:
            lines.append("  none")
        lines.append("")
    lines.append("== total ==")
    for key, value in total.items():
        if isinstance(value, float):
            lines.append(f"{key}: {value:.4f}")
        else:
            lines.append(f"{key}: {value}")
    txt_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    summary_fields = [
        "object",
        "object_status",
        "executable_section_count",
        "stt_func_total",
        "stt_func_symbol_total",
        "unique_function_range_total",
        "valid_sized_functions",
        "hard_blocked_functions",
        "clustered_functions",
        "independently_splittable_functions",
        "hard_blocked_function_ranges",
        "clustered_function_ranges",
        "independently_splittable_function_ranges",
        "independently_splittable_bytes",
        "executable_bytes",
        "independently_splittable_byte_ratio",
        "independently_splittable_function_ratio",
        "section_symbol_relocations",
        "cross_function_relocations",
        "no_reloc_cross_function_edges",
        "unknown_control_flow_count",
        "relocated_direct_calls",
        "relocated_jals",
        "relocated_conditional_branches",
        "indirect_calls",
        "indirect_jumps",
        "computed_jump_blocked",
        "paired_call_jalr_skipped",
        "zero_size_blocked_symbols",
        "unresolved_relocation_targets",
        "unowned_relocations",
        "padding_relocations",
    ]
    with summary_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=summary_fields)
        writer.writeheader()
        for r in results:
            writer.writerow({k: r[k] for k in summary_fields})
    function_fields = [
        "object", "section", "name", "aliases", "start", "size", "end", "bind", "visibility",
        "status", "cluster_id", "reasons", "relocation_count", "section_symbol_relocations",
        "cross_function_relocations", "local_label_relocations", "unknown_control_flow_count",
        "no_reloc_cross_edges", "relocated_direct_calls", "relocated_jals",
        "relocated_conditional_branches",
        "indirect_calls", "indirect_jumps", "computed_jump_blocked",
        "unresolved_relocation_targets",
    ]
    with functions_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=function_fields)
        writer.writeheader()
        for r in results:
            for fn in r["functions"]:
                row = dict(fn)
                row["aliases"] = ";".join(row["aliases"])
                row["reasons"] = ";".join(row["reasons"])
                writer.writerow({k: row[k] for k in function_fields})
    edge_fields = [
        "object", "section", "source_function", "source_addr", "target_addr", "target_region",
        "target_function", "mnemonic", "has_relocation", "edge_kind", "reason", "paired_jalr_addr",
    ]
    with edges_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=edge_fields)
        writer.writeheader()
        for r in results:
            for edge in r["edges"]:
                writer.writerow({k: edge[k] for k in edge_fields})
    json_path.write_text(json.dumps({"summary": total, "objects": results}, indent=2), encoding="utf-8")

    print(txt_path)
    print(summary_csv)
    print(functions_csv)
    print(edges_csv)
    print(json_path)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("objects", nargs="+", type=Path, help="input ET_REL RV32 object files")
    parser.add_argument("--readelf", required=True, help="path to llvm-readelf")
    parser.add_argument("--objdump", required=True, help="path to llvm-objdump")
    parser.add_argument("--out-dir", required=True, type=Path, help="output directory")
    parser.add_argument("--name", required=True, help="benchmark/report name")
    args = parser.parse_args(argv)

    objects = dedupe_inputs(args.objects)
    if not objects:
        raise AuditError("no input objects after de-duplication")
    results = [analyze_object(obj, args.readelf, args.objdump) for obj in objects]
    write_outputs(results, args.out_dir, args.name)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
