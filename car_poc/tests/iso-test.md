# ISO-Aligned Test Case Generation Prompt

You are given a C project: **Car POC (Proof of Concept)**  
It simulates automotive features in **Embedded C**, with no real hardware, using an SDL2 visual simulator and HAL mocks.  

## Project Modules
- `app_autobrake.c` → Auto braking logic (uses distance sensor input, brake request output).
- `app_autopark.c` → Auto parking logic.
- `app_climate.c` → Auto climate control (adjusts cabin temperature).
- `app_speedgov.c` → Speed governor (reads sign speed, alarms if exceeded).
- `app_wipers.c` → Rain sensing wipers (20°–120° sweep).
- `app_voice.c` → Voice assistant trigger.
- `hal_*.c` → Hardware abstraction layer (mocked for sensors/actuators).
- `main.c` → Main entry and simulation loop.

## ISO Standards to Align With
1. **ISO 26262 (Functional Safety)**  
   - Safety requirements (latency, fail-safe, bounds).
   - Example: Brake must activate within 100 ms if obstacle detected.

2. **ISO/SAE 21434 (Cybersecurity)**  
   - Security requirements (bounds checks, fuzzing, input validation).
   - Example: Voice input buffer must not overflow.

3. **ISO 14229 (UDS Diagnostics)**  
   - Diagnostic services & DTC checks.
   - Example: Implausible sensor values set a diagnostic trouble code (DTC).

## Testing Framework
- Use **Unity** test framework (`unity.c`, `unity.h` in `tests/unity/`).
- Each module has a test file in `tests/` (e.g., `test_autobrake.c`).
- Each test file already has `UNITY_BEGIN()` / `UNITY_END()` in `main()`.

## Requirements for Generated Tests
- Write **C unit tests** using Unity (`TEST_ASSERT_*`).
- Group tests by ISO area:
  - Safety tests → prefix with `test_SAF_*`
  - Security tests → prefix with `test_SEC_*`
  - Diagnostics tests → prefix with `test_UDS_*`
- Each test should check one requirement.
- Provide comments explaining **which ISO requirement** it maps to.

### Example Test (Safety, ISO 26262)
```c
void test_SAF_BrakeWithin100ms(void) {
    // ISO 26262: braking latency requirement
    mock_distance_mm = 700U; // obstacle < threshold
    mock_timestamp_ms = 0U;
    mock_current_time = 0U;

    app_autobrake_step();
    mock_current_time = 50U;
    app_autobrake_step();

    TEST_ASSERT_TRUE(mock_brake_request); // must brake within 100 ms
}
