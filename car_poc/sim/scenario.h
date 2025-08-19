#ifndef SCENARIO_H
#define SCENARIO_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_VOICE_CMD_LEN (64U)

typedef struct {
    uint32_t ms;
    uint16_t distance_mm;
    uint8_t rain_pct;
    uint16_t speed_kph;
    uint16_t sign_event;
    bool gap_found;
    uint16_t gap_width_mm;
    int16_t cabin_tc_x10;
    int16_t ambient_tc_x10;
    uint8_t humid_pct;
    int16_t setpoint_x10;
    char voice_cmd[MAX_VOICE_CMD_LEN];
} scenario_row_t;

bool scenario_init(const char* filename);
bool scenario_get_next_row(scenario_row_t* row);
void scenario_close(void);

#endif /* SCENARIO_H */