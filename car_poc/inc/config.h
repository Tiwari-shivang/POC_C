#ifndef CONFIG_H
#define CONFIG_H

/* Mercedes POC Configuration Constants */
/* These values align with the existing calib.h definitions */

/* Autobrake Configuration */
#define BRAKE_THRESH_MM                (1220U)
#define AUTOBRAKE_DEBOUNCE_COUNT       (3U)
#define AUTOBRAKE_ACTIVATE_LATENCY_MS  (100U)

/* Climate Configuration */
#define CLIMATE_TARGET_C_X10           (220)    /* 22.0°C */
#define CLIMATE_HUMIDITY_HIGH_PCT      (70U)
#define CLIMATE_LATENCY_MS             (200U)
#define FAN_STAGE_MAX                  (3U)

/* Autopark Configuration */
#define AUTOPARK_MAX_SPEED_KPH         (10U)
#define AUTOPARK_MIN_GAP_MM            (5000U)
#define AUTOPARK_DEBOUNCE_COUNT        (3U)
#define AUTOPARK_ACTIVATE_LATENCY_MS   (200U)
#define AUTOPARK_PROMPT_SCAN           (1U)
#define AUTOPARK_PROMPT_ALIGN          (2U)

/* Common Configuration */
#define STALE_MS                       (100U)
#define CONTROL_DT_MS                  (10U)

#endif /* CONFIG_H */