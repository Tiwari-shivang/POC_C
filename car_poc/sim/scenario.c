#include "scenario.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static FILE* scenario_file = NULL;
static bool header_read = false;

bool scenario_init(const char* filename) {
    if (filename == NULL) {
        return false;
    }
    
    scenario_file = fopen(filename, "r");
    if (scenario_file == NULL) {
        return false;
    }
    
    header_read = false;
    return true;
}

bool scenario_get_next_row(scenario_row_t* row) {
    char line[512];
    char* token;
    uint8_t field_idx = 0U;
    
    if ((scenario_file == NULL) || (row == NULL)) {
        return false;
    }
    
    if (!header_read) {
        if (fgets(line, sizeof(line), scenario_file) == NULL) {
            return false;
        }
        header_read = true;
    }
    
    if (fgets(line, sizeof(line), scenario_file) == NULL) {
        return false;
    }
    
    memset(row, 0, sizeof(scenario_row_t));
    
    token = strtok(line, ",");
    while ((token != NULL) && (field_idx < 11U)) {
        switch (field_idx) {
            case 0U:
                row->ms = (uint32_t)strtoul(token, NULL, 10);
                break;
            case 1U:
                row->distance_mm = (uint16_t)strtoul(token, NULL, 10);
                break;
            case 2U:
                row->rain_pct = (uint8_t)strtoul(token, NULL, 10);
                break;
            case 3U:
                row->speed_kph = (uint16_t)strtoul(token, NULL, 10);
                break;
            case 4U:
                row->sign_event = (uint16_t)strtoul(token, NULL, 10);
                break;
            case 5U:
                row->gap_found = (strtoul(token, NULL, 10) != 0U);
                break;
            case 6U:
                row->gap_width_mm = (uint16_t)strtoul(token, NULL, 10);
                break;
            case 7U:
                row->cabin_tc_x10 = (int16_t)strtol(token, NULL, 10);
                break;
            case 8U:
                row->ambient_tc_x10 = (int16_t)strtol(token, NULL, 10);
                break;
            case 9U:
                row->humid_pct = (uint8_t)strtoul(token, NULL, 10);
                break;
            case 10U:
                row->setpoint_x10 = (int16_t)strtol(token, NULL, 10);
                break;
            default:
                break;
        }
        field_idx++;
        token = strtok(NULL, ",");
    }
    
    if (token != NULL) {
        char* newline = strchr(token, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }
        strncpy(row->voice_cmd, token, MAX_VOICE_CMD_LEN - 1U);
        row->voice_cmd[MAX_VOICE_CMD_LEN - 1U] = '\0';
    }
    
    return true;
}

void scenario_close(void) {
    if (scenario_file != NULL) {
        fclose(scenario_file);
        scenario_file = NULL;
        header_read = false;
    }
}