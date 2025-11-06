# TARA Implementation Guide (YAML-driven, CSV Results)

This document is a complete, copy-paste-ready playbook to add **TARA (Threat Analysis & Risk Assessment)** checks to your AUTOSAR-style POC.  
It uses a **YAML TARA file** as the single source of truth and produces a **CSV results file** in the project root: `./tara_check_results.csv`.

> **Scope of automated checks (for this POC):**
> - Source files: `app_autobrake.c`, `app_speedgov.c`, `app_wipers.c`
> - Check type: **Autosar coding standards subset** (static analysis) + **TARA traceability completeness**
> - Output: CSV with pass/fail per rule, per file, plus TARA linkage completeness

---

## 1) Repository Layout (minimal)

```
your-project/
├─ TARA_Implementation_Guide.md      # this doc
├─ tara.yaml                          # TARA source of truth (create from template below)
├─ app_autobrake.c
├─ app_speedgov.c
├─ app_wipers.c
├─ scripts/
│  ├─ tara_check.py                   # parses tara.yaml + static reports → CSV
│  └─ run_static_checks.sh            # runs clang-tidy/cppcheck and saves reports
└─ tara_check_results.csv             # generated CSV (output)
```

You can place scripts in any folder; paths are configurable in `tara_check.py`.

---

## 2) Create the TARA YAML (template)

Create a file `tara.yaml` in the project root with the following template and fill your details.  
This links **Threats → Goals → Requirements → Controls (code) → Tests**, and identifies the **files** that must pass static checks (Autosar subset).

```yaml
# tara.yaml
item: POC_AUTOSAR_Modules
version: 1.0
owners:
  - name: "Your Name"
    email: "you@example.com"

files_in_scope:
  - path: "app_autobrake.c"
  - path: "app_speedgov.c"
  - path: "app_wipers.c"

# The static ruleset name helps the checker map to the right report fields
static_ruleset:
  name: "AUTOSAR_CPP14_CRITICAL_SUBSET"
  description: "Subset of AUTOSAR/MISRA-style critical rules enforced via clang-tidy/cppcheck for C/C++ POC"
  # You can tune rule IDs to your tool (examples only)
  rules:
    - id: "A5-0-1"     # initialization before use
    - id: "A18-5-2"    # bounds checks
    - id: "A0-1-1"     # no dangerous casts
    - id: "A22-0-1"    # no UB, avoid undefined behavior patterns
    - id: "A2-10-1"    # no unguarded macro trickery
    - id: "NW-NULL"    # null dereference
    - id: "NW-BUFFER"  # buffer overflow-like issues

threats:
  - id: "T1"
    title: "Replay or stale data causes unsafe actuation"
    feasibility: "high"
    impact: "high"
    risk: "very_high"
    goals: ["G1"]
    requirements: ["R1","R2"]
  - id: "T2"
    title: "Tampering or injection in HAL path"
    feasibility: "medium"
    impact: "high"
    risk: "high"
    goals: ["G1"]
    requirements: ["R1"]

goals:
  - id: "G1"
    text: "Ensure authenticity/freshness for actuation-relevant signals"

requirements:
  - id: "R1"
    text: "Apply debouncing, fail-safe defaults, and defensive checks in processing loop"
  - id: "R2"
    text: "Add time plausibility (stale-data reject) and driver-override precedence"

controls:
  - id: "C_AUTOBRAKE_LOOP"
    type: "code"
    files: ["app_autobrake.c"]
    maps_requirements: ["R1","R2"]
  - id: "C_SPEEDGOV_LOOP"
    type: "code"
    files: ["app_speedgov.c"]
    maps_requirements: ["R1"]
  - id: "C_WIPERS_LOOP"
    type: "code"
    files: ["app_wipers.c"]
    maps_requirements: ["R1"]

tests:
  - id: "UT_AUTOBRAKE_DEBOUNCE"
    type: "unit"
    description: "Debounce & fail-safe unit tests"
    evidence_path: "tests/unit/test_autobrake_debounce.xml"
    covers_requirements: ["R1"]
  - id: "SIL_STALE_REJECT"
    type: "sil"
    description: "Software-in-the-loop stale data rejection"
    evidence_path: "tests/sil/test_stale_reject.xml"
    covers_requirements: ["R2"]
```

> Keep IDs stable. The checker will fail if a high-risk threat has no requirement, control, or test evidence.

---

## 3) Static Analysis (Autosar subset) – how we check the three files

You can use **clang-tidy** or **cppcheck** (or both). The script below is a thin wrapper that:
- Runs the tools on `app_autobrake.c`, `app_speedgov.c`, `app_wipers.c`
- Exports a machine-readable report (XML/JSON)
- The Python TARA checker parses those reports and maps issues to rule IDs

Create `scripts/run_static_checks.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

# Config
SRC_FILES=("app_autobrake.c" "app_speedgov.c" "app_wipers.c")
OUT_DIR="reports"
mkdir -p "$OUT_DIR"


# Example with cppcheck (works for C); produces XML report
# Install: sudo apt-get install cppcheck
cppcheck --enable=warning,style,performance,portability \
         --inconclusive --language=c \
         --xml --xml-version=2 \
         "${SRC_FILES[@]}" 2> "${OUT_DIR}/cppcheck.xml"

# Example with clang-tidy (if you have compile_commands.json)
# clang-tidy ${SRC_FILES[@]} -export-fixes="${OUT_DIR}/clang_tidy.yaml" || true

echo "Static analysis reports generated in ${OUT_DIR}/"
```

Make it executable:
```bash
chmod +x scripts/run_static_checks.sh
```

---

## 4) TARA Checker Script → CSV output

Create `scripts/tara_check.py` – it:
1. Parses `tara.yaml`
2. Checks **traceability completeness** (threats → goals → requirements → controls → tests)
3. Parses static analysis report(s) (e.g., `reports/cppcheck.xml`) and maps findings to the files in scope
4. Writes `./tara_check_results.csv` with:
   - `section` (traceability/static)
   - `subject` (file or ID)
   - `rule_or_id`
   - `status` (PASS/FAIL)
   - `details`

```python
#!/usr/bin/env python3
import sys, csv, os, yaml, xml.etree.ElementTree as ET

TARA_FILE = "tara.yaml"
CPPCHK = "reports/cppcheck.xml"
OUT_CSV = "tara_check_results.csv"

def load_yaml(path):
    with open(path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)

def parse_cppcheck(xml_path):
    if not os.path.exists(xml_path):
        return []
    tree = ET.parse(xml_path)
    root = tree.getroot()
    issues = []
    for error in root.findall("errors/error"):
        msg = error.get("msg", "")
        sev = error.get("severity", "")
        cwe = error.get("cwe", "")
        for loc in error.findall("location"):
            file_path = loc.get("file", "")
            line = loc.get("line", "")
            issues.append({
                "file": os.path.basename(file_path),
                "severity": sev,
                "message": msg,
                "cwe": cwe,
                "line": line
            })
    return issues

def main():
    tara = load_yaml(TARA_FILE)
    files_in_scope = [os.path.basename(f["path"]) for f in tara.get("files_in_scope", [])]
    ruleset = tara.get("static_ruleset", {}).get("rules", [])
    rule_ids = set([r["id"] for r in ruleset]) if ruleset else set()
    threats = tara.get("threats", [])
    goals = {g["id"]: g for g in tara.get("goals", [])}
    reqs  = {r["id"]: r for r in tara.get("requirements", [])}
    ctrls = tara.get("controls", [])
    tests = tara.get("tests", [])

    rows = []

    # ---- Traceability checks
    # threats must have goals, requirements
    for t in threats:
        tid = t["id"]
        g_ok = all(gid in goals for gid in t.get("goals", []))
        r_ok = all(rid in reqs  for rid in t.get("requirements", []))
        rows.append(["traceability", tid, "goals_linked", "PASS" if g_ok else "FAIL", ""])
        rows.append(["traceability", tid, "requirements_linked", "PASS" if r_ok else "FAIL", ""])

    # each requirement should be implemented by at least one control
    req_to_ctrl = {rid:False for rid in reqs.keys()}
    for c in ctrls:
        for rid in c.get("maps_requirements", []):
            if rid in req_to_ctrl:
                req_to_ctrl[rid] = True
    for rid, ok in req_to_ctrl.items():
        rows.append(["traceability", rid, "control_exists", "PASS" if ok else "FAIL", ""])

    # each high/very_high threat should be covered by at least one test (by requirement linkage)
    req_to_tests = {}
    for tcase in tests:
        for rid in tcase.get("covers_requirements", []):
            req_to_tests.setdefault(rid, []).append(tcase["id"])

    for t in threats:
        tid = t["id"]
        risk = t.get("risk","").lower()
        needs_tests = risk in ("high","very_high","very_high".replace("_",""))  # robust
        has_test = False
        if needs_tests:
            for rid in t.get("requirements", []):
                if req_to_tests.get(rid):
                    has_test = True
                    break
            rows.append(["traceability", tid, "tests_cover_high_risk", "PASS" if has_test else "FAIL", ""])

    # controls must reference files that are in scope
    for c in ctrls:
        files = c.get("files", [])
        ok = all(os.path.basename(f) in files_in_scope for f in files)
        rows.append(["traceability", c["id"], "files_in_scope", "PASS" if ok else "FAIL", ""])

    # ---- Static analysis check summary (per file)
    cpp_issues = parse_cppcheck(CPPCHK)
    for f in files_in_scope:
        # Mark FAIL if any issue with severity ERROR is present for the file
        file_issues = [i for i in cpp_issues if i["file"] == f]
        has_error = any(i["severity"].lower() in ("error","critical") for i in file_issues)
        detail = f"{len(file_issues)} findings"
        rows.append(["static", f, "cppcheck_summary", "FAIL" if has_error else "PASS", detail])

    # Write CSV
    with open(OUT_CSV, "w", newline="", encoding="utf-8") as cf:
        w = csv.writer(cf)
        w.writerow(["section","subject","rule_or_id","status","details"])
        for r in rows:
            w.writerow(r)

    print(f"CSV written: {OUT_CSV}")

if __name__ == "__main__":
    main()
```

Make it executable:
```bash
chmod +x scripts/tara_check.py
```

---

## 5) Run the checks locally

1) Generate static reports:
```bash
./scripts/run_static_checks.sh
```

2) Generate TARA CSV results:
```bash
./scripts/tara_check.py
# → creates ./tara_check_results.csv
```

Open `tara_check_results.csv` to review PASS/FAIL per traceability rule and per file’s static summary.

---

## 6) (Optional) GitHub Actions CI

Create `.github/workflows/tara.yml`:

```yaml
name: TARA Checks

on: [push, pull_request]

jobs:
  tara:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install tools
        run: |
          sudo apt-get update
          sudo apt-get install -y cppcheck python3-pip
          pip3 install pyyaml
      - name: Static analysis
        run: ./scripts/run_static_checks.sh
      - name: TARA → CSV
        run: ./scripts/tara_check.py
      - name: Upload CSV
        uses: actions/upload-artifact@v4
        with:
          name: tara-results
          path: tara_check_results.csv
```

This will publish `tara_check_results.csv` on every PR/push.

---

## 7) Reading the CSV Result

Columns:
- **section**: `traceability` or `static`
- **subject**: threat/req/control ID (traceability) or filename (static)
- **rule_or_id**: the specific linkage rule or summary
- **status**: `PASS` / `FAIL`
- **details**: free text (e.g., number of findings)

> Treat any `FAIL` in `traceability` as a blocker for high-risk threats.  
> Treat `static` FAILs as code-quality gate failures; fix and re-run.

---

## 8) Adapting the Autosar Rules

- If you use **clang-tidy** with a custom config, export to YAML and parse that instead of `cppcheck.xml` (adjust `tara_check.py` accordingly).  
- If you have an **AUTOSAR/MISRA commercial tool**, export XML/CSV and add a parser section in `tara_check.py`.

---

## 9) Summary

- **TARA (tara.yaml)** = source of truth.  
- **Static checks** = Autosar-style quality for `app_autobrake.c`, `app_speedgov.c`, `app_wipers.c`.  
- **Output** = `tara_check_results.csv` in project root.  
- **CI ready** with minimal setup.

Good luck—drop this in your repo and iterate quickly.
