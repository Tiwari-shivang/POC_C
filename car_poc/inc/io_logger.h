#ifndef IO_LOGGER_H
#define IO_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

bool io_logger_init(const char* filename);
void io_logger_log_outputs(uint32_t timestamp_ms, bool brake, uint8_t wiper_mode, 
                          bool alarm, uint16_t limit_req, uint8_t fan_stage, 
                          bool ac_on, uint8_t blend, uint8_t park_step);
void io_logger_close(void);

#endif /* IO_LOGGER_H */