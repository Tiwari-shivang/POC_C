# Wipers System Evaluation Framework

## Overview
The wipers evaluation framework validates the rain-responsive windshield wiper system against ISO 26262 requirements for timing, hysteresis, and safety behaviors.

## Test Scenarios

### 1. Rain Progression Test (`wipers_rain_progression.json`)
- **Purpose**: Validates mode transitions and timing requirements
- **Test Flow**: OFF → INT → LOW → HIGH → LOW
- **Rain levels**: 10% → 25% → 45% → 75% → 45%
- **Key Metrics**:
  - OFF to INT latency: ≤ 200ms
  - INT to LOW latency: ≤ 200ms  
  - LOW to HIGH latency: ≤ 200ms
  - HIGH to LOW latency: ≤ 200ms
  - Park time when OFF: ≤ 500ms

### 2. Stale Data Handling (`wipers_stale_data.json`)
- **Purpose**: Validates sensor failure handling
- **Test Flow**: Normal operation with 150ms sensor stale period
- **Key Metrics**:
  - Stale data properly ignored
  - Mode stability maintained during stale data

### 3. Hysteresis Validation (`wipers_hysteresis.json`)
- **Purpose**: Validates hysteresis thresholds prevent oscillation
- **Test Flow**: HIGH → LOW (60% threshold) and INT → OFF (15% threshold)
- **Key Metrics**:
  - HIGH to LOW hysteresis: activates at ≤60% for 2 samples
  - INT to OFF hysteresis: activates at ≤15% for 2 samples
  - Consecutive sample debounce validation

## Evaluation Events

The wipers evaluation system tracks these critical events:
- `EVAL_WIPERS_OFF_TO_INT`: Transition from OFF to intermittent mode
- `EVAL_WIPERS_INT_TO_LOW`: Transition from intermittent to low speed
- `EVAL_WIPERS_LOW_TO_HIGH`: Transition from low to high speed
- `EVAL_WIPERS_HIGH_TO_LOW`: Transition from high to low speed  
- `EVAL_WIPERS_INT_TO_OFF`: Transition from intermittent to OFF
- `EVAL_WIPERS_PARK_START`: Begin parking sequence
- `EVAL_WIPERS_PARK_END`: Complete parking sequence

## Integration Points

To integrate wipers evaluation in your system:

1. Include `eval_hooks.h` in your wipers module
2. Call `eval_wipers_sample()` on each sensor reading
3. Call `eval_wipers_event()` on mode transitions
4. Call `eval_wipers_flush()` at test completion

## Reports Generated

- **CSV Trace**: `eval/reports/wipers_trace.csv` - Complete sample history
- **JSON Summary**: `eval/reports/wipers_summary.json` - Aggregated metrics

## Safety Requirements Validated

- **Timing**: All mode transitions within 200ms
- **Stale Data**: Sensor age >100ms properly ignored
- **Hysteresis**: Prevents mode oscillation 
- **Parking**: Wipers reach park position within 500ms when OFF
- **Debouncing**: 2 consecutive samples required for transitions