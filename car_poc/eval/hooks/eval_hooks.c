#include "eval_hooks.h"

void eval_autobrake_sample(uint32_t a, uint16_t b, uint32_t c, uint8_t d, bool e)
{ (void)a; (void)b; (void)c; (void)d; (void)e; }

void eval_autobrake_event(eval_evt_t evt, uint32_t t)
{ (void)evt; (void)t; }

void eval_loop_tick_begin(uint32_t t) { (void)t; }
void eval_loop_tick_end(uint32_t t)   { (void)t; }