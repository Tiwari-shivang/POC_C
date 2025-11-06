OFF Mode → If rain level ≤ 15% for 2 consecutive samples → Wipers stay OFF.

INT Mode Entry → If rain level ≥ 20% for 2 consecutive samples → Transition to INT within 200 ms.

INT → LOW → If rain level ≥ 40% for 2 consecutive samples → Transition to LOW speed within 200 ms.

LOW → HIGH → If rain level ≥ 70% for 2 consecutive samples → Transition to HIGH speed within 200 ms.

HIGH → LOW Hysteresis → If rain level ≤ 60% for 2 consecutive samples → Drop from HIGH to LOW.

INT → OFF Hysteresis → If rain level ≤ 15% for 2 consecutive samples → Transition back to OFF.

Stale Data → If sample age > 100 ms → Ignore sample; mode does not change.

Sensor Failure → If sensor read fails → Hold last mode (no up-transition).

Parking Behavior → When mode = OFF → Wipers must reach parked position within 500 ms.