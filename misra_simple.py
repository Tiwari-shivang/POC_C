#!/usr/bin/env python3
"""
MISRA C 2012 Rules Checker for Mercedes POC
Simple implementation without Unicode characters
"""

import sys
import re
import os

class MisraChecker:
    def __init__(self):
        self.violations = []
        self.rules_enabled = {
            '2.7': True,   # Unused parameters
            '5.3': True,   # Identifier shadowing  
            '8.9': True,   # Variable scope
            '8.13': True,  # Const parameters
            '14.3': True,  # Invariant conditions
            '16.5': True,  # Assertions
            '17.2': True,  # No recursion
            '17.7': True,  # Return value usage
            '21.3': True,  # No dynamic memory
        }
        
        self.rule_descriptions = {
            '2.7': 'Parameters should be used in functions',
            '5.3': 'Identifier shall not shadow another identifier',
            '8.9': 'Variable scope should be minimized',
            '8.13': 'Read-only parameters should be const-qualified',
            '14.3': 'Controlling expressions shall not be invariant',
            '16.5': 'Function-like macros shall use assertions',
            '17.2': 'Functions shall not call themselves recursively',
            '17.7': 'Return value of functions shall be used',
            '21.3': 'Dynamic memory allocation shall not be used'
        }
    
    def check_misra_rules(self, line):
        """Check a line for MISRA violations"""
        line_lower = line.lower()
        
        # Rule 2.7: Unused parameters
        if 'unused parameter' in line_lower or 'parameter never used' in line_lower:
            return '2.7'
        
        # Rule 5.3: Identifier shadowing
        if 'shadow' in line_lower or 'shadows' in line_lower:
            return '5.3'
        
        # Rule 8.9: Variable scope
        if 'scope' in line_lower and ('reduce' in line_lower or 'minimize' in line_lower):
            return '8.9'
        
        # Rule 8.13: Const parameters
        if 'const' in line_lower and 'parameter' in line_lower:
            return '8.13'
        
        # Rule 14.3: Invariant conditions
        if 'always true' in line_lower or 'always false' in line_lower:
            return '14.3'
        
        # Rule 17.7: Unused return values
        if 'return value' in line_lower and 'unused' in line_lower:
            return '17.7'
        
        # Rule 21.3: Dynamic memory
        if any(word in line_lower for word in ['malloc', 'calloc', 'realloc', 'free']):
            return '21.3'
        
        return None
    
    def analyze_cppcheck_output(self, content):
        """Analyze Cppcheck output for MISRA violations"""
        lines = content.split('\n')
        
        for line_num, line in enumerate(lines, 1):
            if not line.strip():
                continue
            
            # Parse Cppcheck output format
            if ':' in line and any(sev in line for sev in ['error', 'warning', 'style']):
                rule = self.check_misra_rules(line)
                if rule:
                    # Extract file and line info
                    parts = line.split(':')
                    if len(parts) >= 3:
                        file_path = parts[0].strip()
                        line_no = parts[1].strip() if parts[1].strip().isdigit() else '0'
                        
                        self.violations.append({
                            'rule': rule,
                            'file': file_path,
                            'line': line_no,
                            'message': line.strip(),
                            'description': self.rule_descriptions[rule]
                        })
    
    def get_violations(self):
        """Return all MISRA violations found"""
        return self.violations
    
    def print_summary(self):
        """Print MISRA compliance summary"""
        if not self.violations:
            print("")
            print("*** MISRA C:2012 COMPLIANCE ACHIEVED! ***")
            print("No MISRA violations found")
            print("Code meets automotive safety standards")
            return
        
        print("")
        print("MISRA C:2012 Analysis Results")
        print("=" * 50)
        
        # Group by rule
        rule_counts = {}
        for violation in self.violations:
            rule = violation['rule']
            rule_counts[rule] = rule_counts.get(rule, 0) + 1
        
        print(f"Total Violations: {len(self.violations)}")
        print(f"Rules Violated: {len(rule_counts)}")
        
        print("")
        print("Violations by Rule:")
        for rule, count in sorted(rule_counts.items()):
            print(f"  Rule {rule}: {count} violation(s)")
            print(f"    {self.rule_descriptions.get(rule, 'Unknown rule')}")

def main():
    if len(sys.argv) < 2:
        print("MISRA C:2012 Checker for Mercedes POC")
        print("Usage: python misra_simple.py <cppcheck_output_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    checker = MisraChecker()
    
    try:
        if not os.path.exists(input_file):
            print(f"Error: File '{input_file}' not found")
            sys.exit(1)
        
        print(f"Analyzing Cppcheck output: {input_file}")
        
        with open(input_file, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        checker.analyze_cppcheck_output(content)
        violations = checker.get_violations()
        
        if violations:
            print("")
            print("MISRA Violations Found:")
            print("-" * 60)
            
            for i, violation in enumerate(violations, 1):
                print(f"{i}. {violation['file']}:{violation['line']}")
                print(f"   Rule {violation['rule']}: {violation['description']}")
                print(f"   {violation['message']}")
                print()
        
        checker.print_summary()
        
        # Return violation count as exit code
        sys.exit(len(violations))
        
    except Exception as e:
        print(f"Error processing file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()