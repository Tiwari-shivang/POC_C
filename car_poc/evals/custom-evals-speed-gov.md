No Alarm Mode → If vehicle speed ≤ 50 km/h (limit) for 2 consecutive samples → Alarm stays OFF.

Overspeed Alarm Entry → If vehicle speed ≥ 51 km/h for 2 consecutive samples → Alarm must turn ON within 200 ms.

Alarm Clear with Hysteresis → Once alarm is ON, it clears only when vehicle speed ≤ 47 km/h (limit − 3 km/h) for 2 consecutive samples.

Stale Data → If speed sample age > 100 ms → Ignore sample; alarm state does not change.

Sensor Failure → If speed read fails → Hold last alarm state (no new alarm assertions).

Limit Update → When new speed limit = 80 km/h is polled, system must request update (hal_set_speed_limit_request(80)) within 100 ms.

Ignore Invalid Limit → If new speed limit = 0 km/h (invalid), ignore and keep previous limit (no update request).

No Duplicate Requests → If same limit value is polled repeatedly, request is sent at most once within 1 s.

Latency Budget → From first valid overspeed detection, alarm must assert within 200 ms total (including debounce).

Diagnostic Behavior → If sensor fails 3 times within 1 s, raise DTC_SPEED_SENSOR_FAIL; clear it after 5 consecutive valid reads.