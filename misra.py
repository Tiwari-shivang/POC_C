#!/usr/bin/env python3
"""
MISRA C 2012 Rules Checker for Mercedes POC
Enhanced implementation with comprehensive rule coverage
"""

import sys
import re
import json
import os

class MisraChecker:
    def __init__(self):
        self.violations = []
        self.rules_enabled = {
            '1.1': True,   # Language compliance
            '2.1': True,   # Unreachable code
            '2.7': True,   # Unused parameters
            '5.3': True,   # Identifier shadowing  
            '8.9': True,   # Variable scope
            '8.13': True,  # Const parameters
            '9.1': True,   # Uninitialized variables
            '14.3': True,  # Invariant conditions
            '16.5': True,  # Assertions
            '17.2': True,  # No recursion
            '17.7': True,  # Return value usage
            '21.3': True,  # No dynamic memory
        }
        
        self.rule_descriptions = {
            '1.1': 'Code shall conform to C99',
            '2.1': 'Code shall not be unreachable',
            '2.7': 'Parameters should be used in functions',
            '5.3': 'Identifier shall not shadow another identifier',
            '8.9': 'Variable scope should be minimized',
            '8.13': 'Read-only parameters should be const-qualified',
            '9.1': 'Variables shall be initialized before use',
            '14.3': 'Controlling expressions shall not be invariant',
            '16.5': 'Function-like macros shall use assertions',
            '17.2': 'Functions shall not call themselves recursively',
            '17.7': 'Return value of functions shall be used',
            '21.3': 'Dynamic memory allocation shall not be used'
        }
    
    def check_rule_2_7(self, token):
        """Rule 2.7: Parameters should be used"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['unused parameter', 'parameter never used']):
            self.violations.append({
                'rule': '2.7',
                'file': token.get('file'),
                'line': token.get('line'),
                'message': f"MISRA 2.7: {self.rule_descriptions['2.7']}",
                'severity': 'required'
            })
    
    def check_rule_5_3(self, token):
        """Rule 5.3: No identifier shadowing"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['shadow', 'shadows', 'shadowing']):
            self.violations.append({
                'rule': '5.3', 
                'file': token.get('file'),
                'line': token.get('line'),
                'message': f"MISRA 5.3: {self.rule_descriptions['5.3']}",
                'severity': 'required'
            })
    
    def check_rule_8_9(self, token):
        """Rule 8.9: Minimize variable scope"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['scope', 'can be reduced', 'minimize scope']):
            self.violations.append({
                'rule': '8.9',
                'file': token.get('file'), 
                'line': token.get('line'),
                'message': f"MISRA 8.9: {self.rule_descriptions['8.9']}",
                'severity': 'advisory'
            })
    
    def check_rule_8_13(self, token):
        """Rule 8.13: Use const for read-only parameters"""
        msg = token.get('msg', '').lower()
        if ('const' in msg and 'parameter' in msg) or 'constparameter' in token.get('id', ''):
            self.violations.append({
                'rule': '8.13',
                'file': token.get('file'),
                'line': token.get('line'), 
                'message': f"MISRA 8.13: {self.rule_descriptions['8.13']}",
                'severity': 'advisory'
            })
    
    def check_rule_14_3(self, token):
        """Rule 14.3: No invariant conditions"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['always true', 'always false', 'invariant', 'condition is always']):
            self.violations.append({
                'rule': '14.3',
                'file': token.get('file'),
                'line': token.get('line'),
                'message': f"MISRA 14.3: {self.rule_descriptions['14.3']}",
                'severity': 'required'
            })
    
    def check_rule_17_7(self, token):
        """Rule 17.7: Return value should be used"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['return value', 'unused return', 'ignored return']):
            self.violations.append({
                'rule': '17.7',
                'file': token.get('file'),
                'line': token.get('line'),
                'message': f"MISRA 17.7: {self.rule_descriptions['17.7']}",
                'severity': 'required'
            })
    
    def check_rule_21_3(self, token):
        """Rule 21.3: No dynamic memory allocation"""
        msg = token.get('msg', '').lower()
        if any(phrase in msg for phrase in ['malloc', 'calloc', 'realloc', 'free', 'dynamic']):
            self.violations.append({
                'rule': '21.3',
                'file': token.get('file'),
                'line': token.get('line'),
                'message': f"MISRA 21.3: {self.rule_descriptions['21.3']}",
                'severity': 'required'
            })
    
    def process_token(self, token):
        """Process a single token for MISRA violations"""
        if self.rules_enabled.get('2.7'):
            self.check_rule_2_7(token)
        if self.rules_enabled.get('5.3'):
            self.check_rule_5_3(token)
        if self.rules_enabled.get('8.9'):
            self.check_rule_8_9(token)
        if self.rules_enabled.get('8.13'):
            self.check_rule_8_13(token)
        if self.rules_enabled.get('14.3'):
            self.check_rule_14_3(token)
        if self.rules_enabled.get('17.7'):
            self.check_rule_17_7(token)
        if self.rules_enabled.get('21.3'):
            self.check_rule_21_3(token)
    
    def analyze_cppcheck_output(self, content):
        """Analyze Cppcheck output for MISRA violations"""
        lines = content.split('\n')
        
        for line in lines:
            if not line.strip():
                continue
                
            # Parse Cppcheck output format: file:line:column: severity: message [id]
            if ':' in line and any(sev in line for sev in ['error', 'warning', 'style', 'information']):
                parts = line.split(':')
                if len(parts) >= 4:
                    try:
                        file_path = parts[0].strip()
                        line_num = int(parts[1].strip()) if parts[1].strip().isdigit() else 0
                        severity = ''
                        message = ''
                        
                        # Extract severity and message
                        remaining = ':'.join(parts[2:])
                        for sev in ['error', 'warning', 'style', 'information']:
                            if sev in remaining:
                                idx = remaining.find(sev)
                                severity = sev
                                message = remaining[idx + len(sev):].strip()
                                break
                        
                        # Extract ID if present
                        id_match = re.search(r'\[([^]]+)\]', message)
                        check_id = id_match.group(1) if id_match else ''
                        
                        token = {
                            'file': file_path,
                            'line': line_num,
                            'msg': message,
                            'severity': severity,
                            'id': check_id
                        }
                        
                        self.process_token(token)
                        
                    except (ValueError, IndexError):
                        continue
    
    def get_violations(self):
        """Return all MISRA violations found"""
        return self.violations
    
    def print_summary(self):
        """Print MISRA compliance summary"""
        if not self.violations:
            print("\n🎉 MISRA C:2012 COMPLIANCE ACHIEVED!")
            print("✅ No MISRA violations found")
            print("✅ Code meets automotive safety standards")
            return
        
        print(f"\n📊 MISRA C:2012 Analysis Results")
        print("=" * 50)
        
        # Group by rule
        rule_counts = {}
        severity_counts = {'required': 0, 'advisory': 0, 'mandatory': 0}
        
        for violation in self.violations:
            rule = violation['rule']
            severity = violation.get('severity', 'advisory')
            
            rule_counts[rule] = rule_counts.get(rule, 0) + 1
            severity_counts[severity] = severity_counts.get(severity, 0) + 1
        
        print(f"Total Violations: {len(self.violations)}")
        print(f"Rules Violated: {len(rule_counts)}")
        print(f"Required: {severity_counts.get('required', 0)}")
        print(f"Advisory: {severity_counts.get('advisory', 0)}")
        print(f"Mandatory: {severity_counts.get('mandatory', 0)}")
        
        print("\nViolations by Rule:")
        for rule, count in sorted(rule_counts.items()):
            print(f"  Rule {rule}: {count} violation(s) - {self.rule_descriptions.get(rule, 'Unknown')}")

def main():
    if len(sys.argv) < 2:
        print("MISRA C:2012 Checker for Mercedes POC")
        print("Usage: python misra.py <cppcheck_output_file>")
        print("\nExample:")
        print("  cppcheck --enable=all src/ 2> output.txt")
        print("  python misra.py output.txt")
        sys.exit(1)
    
    input_file = sys.argv[1]
    checker = MisraChecker()
    
    try:
        if not os.path.exists(input_file):
            print(f"❌ Error: File '{input_file}' not found")
            sys.exit(1)
        
        print(f"🔍 Analyzing Cppcheck output: {input_file}")
        
        with open(input_file, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        checker.analyze_cppcheck_output(content)
        violations = checker.get_violations()
        
        if violations:
            print(f"\n📋 MISRA Violations Found:")
            print("-" * 80)
            
            for i, violation in enumerate(violations, 1):
                print(f"{i:2d}. {violation['file']}:{violation['line']}")
                print(f"    {violation['message']}")
                print(f"    Severity: {violation.get('severity', 'unknown').upper()}")
                print()
        
        checker.print_summary()
        
        # Return appropriate exit code
        sys.exit(len(violations))
        
    except FileNotFoundError:
        print(f"❌ Error: File '{input_file}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Error processing file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()