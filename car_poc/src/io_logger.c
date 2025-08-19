#include "hal.h"
#include <stdio.h>

static FILE* log_file = NULL;
static bool log_file_open = false;

bool io_logger_init(const char* filename) {
    if (filename == NULL) {
        return false;
    }
    
    log_file = fopen(filename, "w");
    if (log_file == NULL) {
        return false;
    }
    
    fprintf(log_file, "ms,brake,wiper_mode,alarm,limit_req,fan_stage,ac_on,blend,park_step\n");
    fflush(log_file);
    log_file_open = true;
    
    return true;
}

void io_logger_log_outputs(uint32_t timestamp_ms, bool brake, uint8_t wiper_mode, 
                          bool alarm, uint16_t limit_req, uint8_t fan_stage, 
                          bool ac_on, uint8_t blend, uint8_t park_step) {
    if (!log_file_open || (log_file == NULL)) {
        return;
    }
    
    fprintf(log_file, "%u,%d,%u,%d,%u,%u,%d,%u,%u\n",
            timestamp_ms,
            brake ? 1 : 0,
            wiper_mode,
            alarm ? 1 : 0,
            limit_req,
            fan_stage,
            ac_on ? 1 : 0,
            blend,
            park_step);
    
    fflush(log_file);
}

void io_logger_close(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
        log_file_open = false;
    }
}