# REQUIRES: aarch64-registered-target

## Check that type units keep their line table references when llvm-dwarfutil
## rewrites DWARF. Child DIEs may carry DW_AT_decl_file and need the type unit's
## DW_AT_stmt_list to resolve those file indexes.

# RUN: llvm-mc -triple=aarch64-unknown-linux-gnu -filetype=obj %s -o %t.o
# RUN: llvm-dwarfutil --no-garbage-collection %t.o %t.out
# RUN: llvm-dwarfdump --verify %t.out | FileCheck %s
# RUN: llvm-dwarfdump --debug-line %t.out | FileCheck %s --check-prefix=LINE

# CHECK: No errors.
# LINE: debug_line[0x00000000]
# LINE-NOT: debug_line[

	.text
	.file	"-"
	.file	1 "." "type-unit-stmt-list.cpp"
	.section	.debug_types,"G",@progbits,4068369915778327548,comdat
	.word	.Ldebug_types_end0-.Ldebug_types_start0
.Ldebug_types_start0:
	.hword	4
	.word	.debug_abbrev
	.byte	8
	.xword	4068369915778327548
	.word	30
	.byte	1
	.hword	33
	.word	.Lline_table_start0
	.byte	2
	.byte	5
	.word	.Linfo_string6
	.byte	4
	.byte	1
	.byte	1
	.byte	3
	.word	.Linfo_string4
	.word	52
	.byte	1
	.byte	1
	.byte	0
	.byte	0
	.byte	4
	.word	.Linfo_string5
	.byte	5
	.byte	4
	.byte	0
.Ldebug_types_end0:

	.text
	.globl	main
	.p2align	2
	.type	main,@function
main:
.Lfunc_begin0:
	.loc	1 3 0
	.cfi_startproc
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	wzr, [sp, #12]
.Ltmp1:
	.loc	1 3 23 prologue_end
	adrp	x8, f
	ldr	w0, [x8, :lo12:f]
	.loc	1 3 14 epilogue_begin is_stmt 0
	add	sp, sp, #16
	.cfi_def_cfa_offset 0
	ret
.Ltmp2:
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc

	.type	f,@object
	.bss
	.globl	f
	.p2align	2, 0x0
f:
	.zero	4
	.size	f, 4

	.section	.debug_abbrev,"",@progbits
	.byte	1
	.byte	65
	.byte	1
	.byte	19
	.byte	5
	.byte	16
	.byte	23
	.byte	0
	.byte	0
	.byte	2
	.byte	19
	.byte	1
	.byte	54
	.byte	11
	.byte	3
	.byte	14
	.byte	11
	.byte	11
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	0
	.byte	0
	.byte	3
	.byte	13
	.byte	0
	.byte	3
	.byte	14
	.byte	73
	.byte	19
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	56
	.byte	11
	.byte	0
	.byte	0
	.byte	4
	.byte	36
	.byte	0
	.byte	3
	.byte	14
	.byte	62
	.byte	11
	.byte	11
	.byte	11
	.byte	0
	.byte	0
	.byte	5
	.byte	17
	.byte	1
	.byte	37
	.byte	14
	.byte	19
	.byte	5
	.byte	3
	.byte	14
	.byte	16
	.byte	23
	.byte	27
	.byte	14
	.byte	17
	.byte	1
	.byte	18
	.byte	6
	.byte	0
	.byte	0
	.byte	6
	.byte	52
	.byte	0
	.byte	3
	.byte	14
	.byte	73
	.byte	19
	.byte	63
	.byte	25
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	2
	.byte	24
	.byte	0
	.byte	0
	.byte	7
	.byte	19
	.byte	0
	.byte	60
	.byte	25
	.byte	105
	.byte	32
	.byte	0
	.byte	0
	.byte	8
	.byte	46
	.byte	0
	.byte	17
	.byte	1
	.byte	18
	.byte	6
	.byte	64
	.byte	24
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	73
	.byte	19
	.byte	63
	.byte	25
	.byte	0
	.byte	0
	.byte	0

	.section	.debug_info,"",@progbits
.Lcu_begin0:
	.word	.Ldebug_info_end1-.Ldebug_info_start1
.Ldebug_info_start1:
	.hword	4
	.word	.debug_abbrev
	.byte	8
	.byte	5
	.word	.Linfo_string0
	.hword	33
	.word	.Linfo_string1
	.word	.Lline_table_start0
	.word	.Linfo_string2
	.xword	.Lfunc_begin0
	.word	.Lfunc_end0-.Lfunc_begin0
	.byte	6
	.word	.Linfo_string3
	.word	63
	.byte	1
	.byte	2
	.byte	9
	.byte	3
	.xword	f
	.byte	7
	.xword	4068369915778327548
	.byte	8
	.xword	.Lfunc_begin0
	.word	.Lfunc_end0-.Lfunc_begin0
	.byte	1
	.byte	111
	.word	.Linfo_string7
	.byte	1
	.byte	3
	.word	97
	.byte	4
	.word	.Linfo_string5
	.byte	5
	.byte	4
	.byte	0
.Ldebug_info_end1:

	.section	.debug_str,"MS",@progbits,1
.Linfo_string0:
	.asciz	"clang"
.Linfo_string1:
	.asciz	"type-unit-stmt-list.cpp"
.Linfo_string2:
	.asciz	"."
.Linfo_string3:
	.asciz	"f"
.Linfo_string4:
	.asciz	"x"
.Linfo_string5:
	.asciz	"int"
.Linfo_string6:
	.asciz	"foo"
.Linfo_string7:
	.asciz	"main"

	.ident	"clang"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym f
	.section	.debug_line,"",@progbits
.Lline_table_start0:
