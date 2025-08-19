#ifndef CALIB_H
#define CALIB_H

#define SENSOR_STALE_MS            (100U)

#define AB_THRESHOLD_MM            (1220U)
#define AB_DEBOUNCE_HITS          (3U)

#define WIPER_T_RAIN_INT          (20U)
#define WIPER_T_RAIN_LOW          (45U)
#define WIPER_T_RAIN_HIGH         (75U)

#define SPEED_ALARM_TOL_KPH       (3U)
#define SPEED_ALARM_DEBOUNCE      (2U)
#define SPEED_HYSTERESIS_KPH      (5U)

#define CLIMATE_KP                (8)
#define CLIMATE_KI                (1)
#define CLIMATE_DT_MS             (1000U)

#define PARK_MIN_GAP_MM           (5000U)
#define PARK_SCAN_LEN_SAMPLES     (50U)

#endif /* CALIB_H */