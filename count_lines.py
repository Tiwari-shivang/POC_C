#!/usr/bin/env python3
"""
Lines of Code Counter for Mercedes POC
Analyzes and counts lines of code across different categories
"""

import os
import glob
from pathlib import Path

def count_lines_in_file(filepath):
    """Count total lines, code lines, comment lines, and blank lines in a file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        total_lines = len(lines)
        blank_lines = 0
        comment_lines = 0
        code_lines = 0
        in_multiline_comment = False
        
        for line in lines:
            stripped = line.strip()
            
            # Check for blank lines
            if not stripped:
                blank_lines += 1
                continue
            
            # Check for multi-line comments (C-style)
            if '/*' in stripped:
                in_multiline_comment = True
            
            if in_multiline_comment:
                comment_lines += 1
                if '*/' in stripped:
                    in_multiline_comment = False
            # Check for single-line comments
            elif stripped.startswith('//') or stripped.startswith('#'):
                comment_lines += 1
            else:
                code_lines += 1
        
        return {
            'total': total_lines,
            'code': code_lines,
            'comments': comment_lines,
            'blank': blank_lines
        }
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}

def analyze_directory(base_path):
    """Analyze all source code files in the project"""
    
    categories = {
        'Source Files (.c)': {
            'pattern': 'car_poc/src/*.c',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Header Files (.h)': {
            'pattern': 'car_poc/inc/*.h',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Test Files': {
            'pattern': 'car_poc/tests/*.c',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Simulation Files': {
            'pattern': 'car_poc/sim/*.c',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Simulation Headers': {
            'pattern': 'car_poc/sim/*.h',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Configuration Headers': {
            'pattern': 'car_poc/cfg/*.h',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Evaluation Files': {
            'pattern': 'car_poc/eval/**/*.c',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        },
        'Unity Test Framework': {
            'pattern': 'car_poc/tests/unity/*.[ch]',
            'files': [],
            'stats': {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
        }
    }
    
    # Process each category
    for category, info in categories.items():
        pattern_path = os.path.join(base_path, info['pattern'])
        files = glob.glob(pattern_path, recursive=True)
        
        for filepath in files:
            filename = os.path.basename(filepath)
            stats = count_lines_in_file(filepath)
            
            info['files'].append({
                'name': filename,
                'path': filepath,
                'stats': stats
            })
            
            # Aggregate stats
            for key in stats:
                info['stats'][key] += stats[key]
    
    return categories

def print_detailed_report(categories):
    """Print a detailed report of lines of code"""
    
    print("\n" + "=" * 80)
    print("                   MERCEDES POC - LINES OF CODE ANALYSIS")
    print("=" * 80)
    
    # Overall totals
    overall = {'total': 0, 'code': 0, 'comments': 0, 'blank': 0}
    
    for category, info in categories.items():
        if info['files']:  # Only show categories with files
            print(f"\n{category}")
            print("-" * 40)
            
            # Sort files by code lines (descending)
            sorted_files = sorted(info['files'], key=lambda x: x['stats']['code'], reverse=True)
            
            for file_info in sorted_files:
                stats = file_info['stats']
                name = file_info['name']
                print(f"  {name:30} | Total: {stats['total']:5} | Code: {stats['code']:5} | Comments: {stats['comments']:4} | Blank: {stats['blank']:4}")
            
            print(f"  {'Subtotal':30} | Total: {info['stats']['total']:5} | Code: {info['stats']['code']:5} | Comments: {info['stats']['comments']:4} | Blank: {info['stats']['blank']:4}")
            
            # Add to overall totals
            for key in overall:
                overall[key] += info['stats'][key]
    
    # Print summary
    print("\n" + "=" * 80)
    print("SUMMARY BY CATEGORY")
    print("=" * 80)
    
    # Sort categories by code lines
    sorted_categories = sorted(
        [(cat, info) for cat, info in categories.items() if info['files']], 
        key=lambda x: x[1]['stats']['code'], 
        reverse=True
    )
    
    for category, info in sorted_categories:
        stats = info['stats']
        percentage = (stats['code'] / overall['code'] * 100) if overall['code'] > 0 else 0
        print(f"{category:30} | Code Lines: {stats['code']:5} ({percentage:5.1f}%) | Files: {len(info['files']):3}")
    
    print("\n" + "=" * 80)
    print("OVERALL STATISTICS")
    print("=" * 80)
    
    print(f"Total Lines of Code (excluding blank/comments): {overall['code']:,}")
    print(f"Total Lines (including everything):             {overall['total']:,}")
    print(f"Comment Lines:                                   {overall['comments']:,}")
    print(f"Blank Lines:                                     {overall['blank']:,}")
    
    if overall['total'] > 0:
        code_percentage = (overall['code'] / overall['total']) * 100
        comment_percentage = (overall['comments'] / overall['total']) * 100
        blank_percentage = (overall['blank'] / overall['total']) * 100
        
        print(f"\nCode Density:     {code_percentage:.1f}%")
        print(f"Comment Density:  {comment_percentage:.1f}%")
        print(f"Blank Lines:      {blank_percentage:.1f}%")
    
    # Module breakdown
    print("\n" + "=" * 80)
    print("MODULE BREAKDOWN (Core Application Modules)")
    print("=" * 80)
    
    modules = [
        ('Auto Brake System', 'app_autobrake'),
        ('Windshield Wipers', 'app_wipers'),
        ('Speed Governor', 'app_speedgov'),
        ('Auto Parking', 'app_autopark'),
        ('Climate Control', 'app_climate'),
        ('Voice Assistant', 'app_voice')
    ]
    
    module_total = 0
    for module_name, module_prefix in modules:
        module_lines = 0
        # Count lines in source and header files for this module
        for category, info in categories.items():
            for file_info in info['files']:
                if module_prefix in file_info['name'].lower():
                    module_lines += file_info['stats']['code']
        
        if module_lines > 0:
            module_total += module_lines
            print(f"  {module_name:25} | {module_lines:5} lines")
    
    print(f"  {'Total Core Modules':25} | {module_total:5} lines")
    
    # Test coverage
    test_lines = categories.get('Test Files', {}).get('stats', {}).get('code', 0)
    if overall['code'] > 0:
        test_ratio = test_lines / (overall['code'] - test_lines) * 100
        print(f"\nTest Code Ratio: {test_ratio:.1f}% (Test LOC / Production LOC)")
    
    print("\n" + "=" * 80)
    print("                            END OF REPORT")
    print("=" * 80)

def main():
    base_path = os.path.dirname(os.path.abspath(__file__))
    
    print("Analyzing Mercedes POC codebase...")
    categories = analyze_directory(base_path)
    print_detailed_report(categories)
    
    # Save report to file
    report_file = os.path.join(base_path, 'loc_report.txt')
    import sys
    original_stdout = sys.stdout
    with open(report_file, 'w') as f:
        sys.stdout = f
        print_detailed_report(categories)
        sys.stdout = original_stdout
    
    print(f"\nReport also saved to: {report_file}")

if __name__ == "__main__":
    main()