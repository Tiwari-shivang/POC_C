#!/usr/bin/env python3
"""
Simple threshold checker for autobrake eval KPIs.
Reads eval/reports/autobrake_summary.json and eval/thresholds.json
and exits non-zero if any threshold is breached.
"""

import json
import sys
import os

def main():
    # Paths
    summary_path = "eval/reports/autobrake_summary.json"
    thresholds_path = "eval/thresholds.json"
    
    # Check if files exist
    if not os.path.exists(summary_path):
        print(f"ERROR: Summary file not found: {summary_path}")
        return 1
        
    if not os.path.exists(thresholds_path):
        print(f"ERROR: Thresholds file not found: {thresholds_path}")
        return 1
    
    # Load data
    try:
        with open(summary_path, 'r') as f:
            summary = json.load(f)
        with open(thresholds_path, 'r') as f:
            thresholds = json.load(f)
    except Exception as e:
        print(f"ERROR: Failed to load JSON files: {e}")
        return 1
    
    # Extract metrics
    autobrake_data = summary.get('autobrake', {})
    autobrake_thresholds = thresholds.get('autobrake', {})
    
    detect_latency = autobrake_data.get('detect_latency_ms', 0)
    react_latency = autobrake_data.get('react_latency_ms', 0)
    
    detect_max = autobrake_thresholds.get('detect_latency_ms_max', 9999)
    react_max = autobrake_thresholds.get('react_latency_ms_max', 9999)
    
    # Check thresholds
    failures = []
    
    if detect_latency > detect_max:
        failures.append(f"Detection latency {detect_latency}ms exceeds max {detect_max}ms")
    
    if react_latency > react_max:
        failures.append(f"Reaction latency {react_latency}ms exceeds max {react_max}ms")
    
    # Report results
    if failures:
        print("THRESHOLD VIOLATIONS:")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    else:
        print("ALL THRESHOLDS PASSED:")
        print(f"  - Detection latency: {detect_latency}ms <= {detect_max}ms")
        print(f"  - Reaction latency: {react_latency}ms <= {react_max}ms")
        return 0

if __name__ == "__main__":
    sys.exit(main())