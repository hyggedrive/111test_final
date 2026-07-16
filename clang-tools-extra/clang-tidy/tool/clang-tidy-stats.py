#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import tempfile
import yaml
import pandas as pd


class DiagTranslator(object):
    def __init__(self, diag):
        self.FixesMaxWidth = 0
        self.Level = diag["Level"]
        self.DiagnosticName = diag["DiagnosticName"]
        self.FilePath = diag["DiagnosticMessage"]["FilePath"]
        self.Line = diag["DiagnosticMessage"]["FileLine"]
        self.Col = diag["DiagnosticMessage"]["FileCol"]
        self.Message = diag["DiagnosticMessage"]["Message"]
        self.Fixes = None
        self.getFixes(diag["DiagnosticMessage"]["Replacements"])
        if "Ranges" in diag["DiagnosticMessage"]:
            ranges = diag["DiagnosticMessage"]["Ranges"]
        else:
            ranges = []
        self.Detail = None
        self.DetailsMaxWidth = 0
        self.mergeDetail(diag["DiagnosticMessage"]["WholeText"], ranges, diag["DiagnosticMessage"]["MainLine"])

    def mergeDetail(self, text, ranges, mainline):
        text = text.rstrip().split("\n")
        base = dict()
        baseline = self.Line - mainline
        for index, value in enumerate(text):
            base[baseline + index + 1] = value
        point = " " * (self.Col - 1) + "^"
        point = point.ljust(len(base[self.Line]))
        points = dict()
        points[self.Line] = point

        for Range in ranges:
            Range["WholeText"] = Range["WholeText"].rstrip().split("\n")
            baseline = Range["FileLine"] - Range["MainLine"]
            for i, value in enumerate(Range["WholeText"]):
                index = baseline + i + 1
                if index not in base:
                    base[index] = value
            if Range["MainLine"] in base:
                if Range["FileLine"] not in points:
                    points[Range["FileLine"]] = " " * len(base[Range["FileLine"]])
                points[Range["FileLine"]] = (points[Range["FileLine"]][:Range["FileCol"] - 1] +
                                             "~" * Range["Length"] +
                                             points[Range["FileLine"]][Range["FileCol"] + Range["Length"] - 1:])
            else:
                point = " " * (Range["FileCol"] - 1) + "~" * Range["Length"]
                point = point.ljust(len(base[Range["FileLine"]]))
                points[Range["FileLine"]] = point

        Detail = ""
        for_i = sorted(base.keys())
        align = len(str(for_i[-1])) + 1
        last = 0
        DetailsMaxWidth = 0
        for index in for_i:
            if not (last == 0 or index == last + 1):
                Detail += " " + "-" * (DetailsMaxWidth - 3) + " \n"
            last = index
            lineNo = str(index)
            baseText = " " * (align - len(lineNo)) + lineNo + " | " + base[index] + "\n"
            Detail += baseText
            DetailsMaxWidth = max(DetailsMaxWidth, len(baseText))
            if index in points:
                Detail += " " * align + " | " + points[index] + "\n"

        self.DetailsMaxWidth = DetailsMaxWidth
        self.Detail = "\n" + Detail

    def getFixes(self, replacements):
        fixes = ""
        FixesMaxWidth = 0
        for replacement in replacements:
            fix = ReplacementTranslator(replacement)
            fileText = "\'" + fix.WholeLineText + "\' in line " + str(fix.FileLine) + " of " + fix.FilePath
            if fix.Type == "insert":
                text = "Insert \'" + fix.ReplacementText + "\' to "
                fixes += text + fileText + "\n" + " " * (len(text) + fix.FileCol) + "^\n"
            elif fix.Type == "remove":
                text = "Remove \'" + fix.OriginalText + "\' from "
                fixes += text + fileText + "\n" + " " * (len(text) + fix.FileCol) + "~" * fix.Length + "\n"
            else:
                text = "Replace \'" + fix.OriginalText + "\' with \'" + fix.ReplacementText + "\' from "
                fixes += text + fileText + "\n" + " " * (len(text) + fix.FileCol) + "~" * fix.Length + "\n"
            FixesMaxWidth = max(FixesMaxWidth, len(text + fileText))

        self.FixesMaxWidth = FixesMaxWidth
        self.Fixes = fixes


class ReplacementTranslator(object):
    def __init__(self, replacement):
        self.OriginalText = replacement["OriginalText"]
        self.ReplacementText = replacement["ReplacementText"]
        self.WholeLineText = replacement["WholeLineText"].rstrip()
        self.Length = replacement["Length"]
        self.FileLine = replacement["FileLine"]
        self.FileCol = replacement["FileCol"]
        self.FilePath = replacement["FilePath"]
        if self.Length == 0:
            self.Type = "insert"
        elif self.ReplacementText == '':
            self.Type = "remove"
        else:
            self.Type = "replace"


def stats(yaml_file, output_file):
    try:
        with open(yaml_file, encoding='utf-8') as infile:
            Diagnostics = yaml.safe_load(infile)
            Diagnostics = Diagnostics["Diagnostics"]
    except FileNotFoundError as e:
        print("File \"" + yaml_file + "\" not found")
        return
    except TypeError as e:
        print("No errors to be collected.\n")
        return

    details = []
    stats = {}
    total_stats = {"Error": 0, "Warning": 0, "Remark": 0}
    FixesMaxWidth = 0
    DetailsMaxWidth = 0

    for diag in Diagnostics:
        diagTrans = DiagTranslator(diag)
        FixesMaxWidth = max(FixesMaxWidth, diagTrans.FixesMaxWidth)
        DetailsMaxWidth = max(DetailsMaxWidth, diagTrans.DetailsMaxWidth)

        total_stats[diagTrans.Level] += 1
        if diagTrans.FilePath in stats:
            if diagTrans.Level in stats[diagTrans.FilePath]:
                stats[diagTrans.FilePath][diagTrans.Level] += 1
            else:
                stats[diagTrans.FilePath][diagTrans.Level] = 1
        else:
            stats[diagTrans.FilePath] = {"Error": 0, "Warning": 0, "Remark": 0}
            stats[diagTrans.FilePath][diagTrans.Level] += 1

        details.append({"Level": diagTrans.Level, "DiagnosticName": diagTrans.DiagnosticName,
                        "FilePath": diagTrans.FilePath, "Line": diagTrans.Line, "Col": diagTrans.Col,
                        "Message": diagTrans.Message,
                        "Detail": diagTrans.Detail, "Fixes": diagTrans.Fixes})

    statsExcel = [["TotalHandle:", "TotalError", "TotalWarning"],
                  [total_stats["Error"] + total_stats["Warning"],
                   total_stats["Error"], total_stats["Warning"]],
                  [], ["FileName", "Error", "Warning"]]
    for key, value in stats.items():
        statsExcel.append([key, value["Error"], value["Warning"]])

    with pd.ExcelWriter(output_file, engine='xlsxwriter') as writer:
        df = pd.DataFrame(details)
        df.to_excel(writer, sheet_name='Details', index=False)
        df = pd.DataFrame(statsExcel)
        df.to_excel(writer, sheet_name='Stats', index=False, header=False)

        wb = writer.book
        wrap = wb.add_format({'text_wrap': True, 'align': 'left', 'valign': 'vcenter'})
        alignCenter = wb.add_format({'align': 'center', 'valign': 'vcenter'})
        fileName = wb.add_format({'bold': True, 'align': 'center', 'valign': 'vcenter'})
        allStyle = wb.add_format({'border': 1})
        errorColor = wb.add_format({'bg_color': '#C00000'})
        warningColor = wb.add_format({'bg_color': '#FFFF00'})
        titleColor = wb.add_format({'bg_color': '#00B0F0'})
        bgColor = wb.add_format({'bg_color': '#EBF1DE'})

        ws1 = writer.sheets['Details']
        ws1.set_column('A:A', 8, alignCenter)
        ws1.set_column('B:C', 30, wrap)
        ws1.set_column('D:E', 5, alignCenter)
        ws1.set_column('F:F', 30, wrap)
        if DetailsMaxWidth < 8:
            DetailsMaxWidth = 8
        if FixesMaxWidth < 8:
            FixesMaxWidth = 8
        ws1.set_column('G:G', DetailsMaxWidth + 2, wrap)
        ws1.set_column('H:H', FixesMaxWidth + 2, wrap)
        ws1.conditional_format('A1:H%d' % (len(details) + 1), {'type': 'formula', 'criteria': '=TRUE()', 'format': allStyle})
        ws1.conditional_format('A1:H1', {'type': 'no_blanks', 'format': titleColor})
        ws1.conditional_format('A2:H%d' % (len(details) + 1), {'type': 'formula', 'criteria': '=TRUE()', 'format': bgColor})

        ws2 = writer.sheets['Stats']
        ws2.set_column('A:A', df.iloc[:, 0].astype(str).apply(lambda x: len(x)).max() + 5, fileName)
        ws2.set_column('B:C', 15, alignCenter)
        ws2.conditional_format('B1', {'type': 'no_blanks', 'format': errorColor})
        ws2.conditional_format('B4', {'type': 'no_blanks', 'format': errorColor})
        ws2.conditional_format('C1', {'type': 'no_blanks', 'format': warningColor})
        ws2.conditional_format('C4', {'type': 'no_blanks', 'format': warningColor})
    print("Done! output to " + output_file + "\n")
   

def main():
    parser = argparse.ArgumentParser(
        description="Run clang-tidy and "
        "output diagnostics stats. "
    )
    parser.add_argument(
        "-clang-tidy-binary",
        metavar="PATH",
        default="clang-tidy",
        help="path to clang-tidy binary",
    )
    parser.add_argument(
        "-export-stats",
        metavar="FILE",
        dest="export_stats",
        default="stats.xlsx",
        help="Create an excel file to store all report and stats.",
    )
    parser.add_argument(
        "-export-stats-input",
        metavar="FILE",
        dest="export_stats_input",
        default="fix.yaml",
        help="Get the yaml file which from export-details.",
    )

    clang_tidy_args = []
    argv = sys.argv[1:]
    if "--args" in argv:
        clang_tidy_args.extend(argv[: argv.index("--args")])
        argv = argv[argv.index("--args") + 1:]
    else:
        clang_tidy_args = argv
        argv = []

    args = parser.parse_args(argv)

    if not clang_tidy_args:
        stats(args.export_stats_input, args.export_stats)
        return

    yaml_file = None
    for arg in clang_tidy_args:
        if arg.find("-export-details=") != -1:
            yaml_file = arg[arg.find("-export-details=") + len("-export-details="):]
            break

    command = [args.clang_tidy_binary]
    if not yaml_file:
        (handle, yaml_file) = tempfile.mkstemp(suffix=".yaml")
        os.close(handle)
        command.append("--export-details=" + yaml_file)
    command.extend(clang_tidy_args)

    try:
        proc = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        stdout, stderr = proc.communicate()
                
        sys.stdout.write(stdout + "\n")
        sys.stdout.flush()
        if stderr:
            sys.stderr.write(stderr + "\n")
            sys.stderr.flush()
    except Exception as e:
        sys.stderr.write("Failed: " + str(e) + ": ".join(command) + "\n")

    stats(yaml_file, args.export_stats)


if __name__ == "__main__":
    main()
