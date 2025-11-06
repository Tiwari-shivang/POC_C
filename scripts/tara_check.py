#!/usr/bin/env python3
"""
TARA (Threat Analysis & Risk Assessment) Checker for Mercedes POC
Parses tara.yaml and static analysis reports to generate CSV results
"""

import sys
import csv
import os
import yaml
import xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime

# Configuration paths
TARA_FILE = "tara.yaml"
CPPCHK = "autosar_full_analysis.xml"  # AUTOSAR addon analysis (corrected)
OUT_CSV = "new_result.csv"  # Output to new_result.csv

def load_yaml(path):
    """Load and parse YAML configuration file"""
    try:
        with open(path, "r", encoding="utf-8") as f:
            return yaml.safe_load(f)
    except FileNotFoundError:
        print(f"ERROR: {path} not found. Please create the TARA configuration file.")
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"ERROR: Invalid YAML in {path}: {e}")
        sys.exit(1)

def parse_cppcheck(xml_path):
    """Parse cppcheck XML report for issues"""
    if not os.path.exists(xml_path):
        print(f"WARNING: {xml_path} not found. Running without static analysis results.")
        return []

    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()
        issues = []

        # Handle both cppcheck v2 and v1 XML formats
        errors_path = "errors/error" if root.find("errors") is not None else "error"

        for error in root.findall(errors_path):
            msg = error.get("msg", "")
            sev = error.get("severity", "")
            error_id = error.get("id", "")
            cwe = error.get("cwe", "")

            for loc in error.findall("location"):
                file_path = loc.get("file", "")
                line = loc.get("line", "")
                # Normalize path to just filename for comparison
                file_name = os.path.basename(file_path)

                issues.append({
                    "file": file_name,
                    "severity": sev,
                    "message": msg,
                    "error_id": error_id,
                    "cwe": cwe,
                    "line": line
                })

        return issues
    except ET.ParseError as e:
        print(f"WARNING: Could not parse {xml_path}: {e}")
        return []

def check_traceability(tara):
    """Check TARA traceability completeness"""
    rows = []

    threats = tara.get("threats", [])
    goals = {g["id"]: g for g in tara.get("goals", [])}
    reqs = {r["id"]: r for r in tara.get("requirements", [])}
    ctrls = tara.get("controls", [])
    tests = tara.get("tests", [])

    # Check 1: All threats must have linked goals and requirements
    for t in threats:
        tid = t["id"]
        title = t.get("title", "")[:50]  # Truncate for readability

        # Check goals linkage
        linked_goals = t.get("goals", [])
        missing_goals = [gid for gid in linked_goals if gid not in goals]
        if missing_goals:
            rows.append(["traceability", tid, "goals_linked", "FAIL",
                        f"Missing goals: {', '.join(missing_goals)}"])
        else:
            rows.append(["traceability", tid, "goals_linked", "PASS",
                        f"{len(linked_goals)} goals linked"])

        # Check requirements linkage
        linked_reqs = t.get("requirements", [])
        missing_reqs = [rid for rid in linked_reqs if rid not in reqs]
        if missing_reqs:
            rows.append(["traceability", tid, "requirements_linked", "FAIL",
                        f"Missing reqs: {', '.join(missing_reqs)}"])
        else:
            rows.append(["traceability", tid, "requirements_linked", "PASS",
                        f"{len(linked_reqs)} requirements linked"])

    # Check 2: Each requirement should be implemented by at least one control
    req_to_ctrl = {rid: [] for rid in reqs.keys()}
    for c in ctrls:
        for rid in c.get("maps_requirements", []):
            if rid in req_to_ctrl:
                req_to_ctrl[rid].append(c["id"])

    for rid, ctrl_ids in req_to_ctrl.items():
        if not ctrl_ids:
            rows.append(["traceability", rid, "control_exists", "FAIL",
                        "No control implements this requirement"])
        else:
            rows.append(["traceability", rid, "control_exists", "PASS",
                        f"Implemented by: {', '.join(ctrl_ids)}"])

    # Check 3: High/very_high risk threats must have test coverage
    req_to_tests = {}
    for tcase in tests:
        for rid in tcase.get("covers_requirements", []):
            req_to_tests.setdefault(rid, []).append(tcase["id"])

    for t in threats:
        tid = t["id"]
        risk = t.get("risk", "").lower().replace("_", "")

        if risk in ("high", "veryhigh"):
            has_test = False
            test_ids = []

            for rid in t.get("requirements", []):
                if req_to_tests.get(rid):
                    has_test = True
                    test_ids.extend(req_to_tests[rid])

            if has_test:
                rows.append(["traceability", tid, "tests_cover_high_risk", "PASS",
                            f"Covered by: {', '.join(set(test_ids))}"])
            else:
                rows.append(["traceability", tid, "tests_cover_high_risk", "FAIL",
                            f"HIGH RISK threat without test coverage!"])

    # Check 4: Controls must reference files that are in scope
    files_in_scope = [os.path.basename(f["path"]) for f in tara.get("files_in_scope", [])]
    for c in ctrls:
        cid = c["id"]
        files = c.get("files", [])
        normalized_files = [os.path.basename(f) for f in files]

        out_of_scope = [f for f in normalized_files if f not in files_in_scope]
        if out_of_scope:
            rows.append(["traceability", cid, "files_in_scope", "FAIL",
                        f"Out of scope: {', '.join(out_of_scope)}"])
        else:
            rows.append(["traceability", cid, "files_in_scope", "PASS",
                        f"{len(files)} files referenced"])

    return rows

def check_static_analysis(tara, cpp_issues):
    """Check static analysis results against AUTOSAR/MISRA rules"""
    rows = []

    files_in_scope = [os.path.basename(f["path"]) for f in tara.get("files_in_scope", [])]
    ruleset = tara.get("static_ruleset", {})
    rule_ids = {r["id"] for r in ruleset.get("rules", [])}

    # Analyze issues per file
    for fname in files_in_scope:
        file_issues = [i for i in cpp_issues if i["file"] == fname]

        # Count issues by severity
        severity_counts = {}
        for issue in file_issues:
            sev = issue["severity"].lower()
            severity_counts[sev] = severity_counts.get(sev, 0) + 1

        # Determine pass/fail based on critical issues
        critical_severities = {"error", "critical", "warning"}
        has_critical = any(issue["severity"].lower() in critical_severities
                          for issue in file_issues)

        # Build details string
        if file_issues:
            detail_parts = []
            for sev, count in sorted(severity_counts.items()):
                detail_parts.append(f"{count} {sev}")
            details = f"Found: {', '.join(detail_parts)}"
        else:
            details = "No issues found"

        status = "FAIL" if has_critical else "PASS"
        rows.append(["static", fname, "cppcheck_summary", status, details])

        # Check specific AUTOSAR/MISRA violations
        for issue in file_issues:
            if issue["severity"].lower() in critical_severities:
                rows.append(["static", fname, f"line_{issue['line']}", "FAIL",
                            f"{issue['error_id']}: {issue['message'][:60]}..."])

    # Summary row for overall static analysis
    total_issues = len(cpp_issues)
    critical_count = sum(1 for i in cpp_issues
                        if i["severity"].lower() in {"error", "critical"})

    if critical_count > 0:
        rows.append(["static", "OVERALL", "static_analysis", "FAIL",
                    f"{critical_count} critical issues out of {total_issues} total"])
    else:
        rows.append(["static", "OVERALL", "static_analysis", "PASS",
                    f"{total_issues} non-critical findings"])

    return rows

def generate_summary(rows):
    """Generate a summary section for the CSV"""
    summary_rows = []

    # Count pass/fail by section
    trace_pass = sum(1 for r in rows if r[0] == "traceability" and r[3] == "PASS")
    trace_fail = sum(1 for r in rows if r[0] == "traceability" and r[3] == "FAIL")
    static_pass = sum(1 for r in rows if r[0] == "static" and r[3] == "PASS")
    static_fail = sum(1 for r in rows if r[0] == "static" and r[3] == "FAIL")

    summary_rows.append(["summary", "traceability", "total",
                        f"{trace_pass} PASS / {trace_fail} FAIL",
                        f"Pass rate: {100*trace_pass/(trace_pass+trace_fail+0.1):.1f}%"])

    summary_rows.append(["summary", "static", "total",
                        f"{static_pass} PASS / {static_fail} FAIL",
                        f"Pass rate: {100*static_pass/(static_pass+static_fail+0.1):.1f}%"])

    # Overall verdict
    if trace_fail == 0 and static_fail == 0:
        verdict = "PASS"
        details = "All TARA checks passed successfully"
    elif trace_fail > 0:
        verdict = "FAIL"
        details = f"Traceability issues found ({trace_fail} failures)"
    else:
        verdict = "FAIL"
        details = f"Static analysis issues found ({static_fail} failures)"

    summary_rows.append(["summary", "OVERALL", "verdict", verdict, details])

    return summary_rows

def main():
    """Main execution function"""
    print("=" * 60)
    print("Mercedes POC TARA Checker")
    print("=" * 60)

    # Load TARA configuration
    print(f"Loading TARA configuration from {TARA_FILE}...")
    tara = load_yaml(TARA_FILE)

    # Parse static analysis results
    print(f"Parsing static analysis results from {CPPCHK}...")
    cpp_issues = parse_cppcheck(CPPCHK)
    print(f"  Found {len(cpp_issues)} static analysis findings")

    # Run checks
    print("\nRunning TARA checks...")
    rows = []

    # Add timestamp and version info
    rows.append(["metadata", "timestamp", "generated",
                datetime.now().strftime("%Y-%m-%d %H:%M:%S"), ""])
    rows.append(["metadata", "tara_version", "version",
                tara.get("version", "unknown"), tara.get("item", "")])

    # Traceability checks
    print("  - Checking traceability completeness...")
    trace_rows = check_traceability(tara)
    rows.extend(trace_rows)

    # Static analysis checks
    print("  - Checking static analysis results...")
    static_rows = check_static_analysis(tara, cpp_issues)
    rows.extend(static_rows)

    # Generate summary
    print("  - Generating summary...")
    summary_rows = generate_summary(trace_rows + static_rows)
    rows.extend(summary_rows)

    # Write CSV output
    print(f"\nWriting results to {OUT_CSV}...")
    with open(OUT_CSV, "w", newline="", encoding="utf-8") as cf:
        writer = csv.writer(cf)
        # Write header
        writer.writerow(["section", "subject", "rule_or_id", "status", "details"])
        # Write all rows
        for row in rows:
            writer.writerow(row)

    print(f"[OK] CSV results written to: {OUT_CSV}")

    # Print summary to console
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)

    for row in summary_rows:
        if row[2] == "verdict":
            symbol = "[OK]" if row[3] == "PASS" else "[FAIL]"
            print(f"{symbol} Overall: {row[3]} - {row[4]}")
        else:
            print(f"  {row[1]}: {row[3]}")

    print("=" * 60)

    # Exit with appropriate code
    overall_verdict = next((r[3] for r in summary_rows if r[2] == "verdict"), "FAIL")
    sys.exit(0 if overall_verdict == "PASS" else 1)

if __name__ == "__main__":
    main()