#ifndef HACKRF_UI_H
#define HACKRF_UI_H

#include <rf_path.h>
#include <stdint.h>

void hackrf_ui_init(void);
void hackrf_ui_setFrequency(uint64_t _freq);
void hackrf_ui_setSampleRate(uint32_t _sample_rate);
void hackrf_ui_setDirection(const rf_path_direction_t _direction);
void hackrf_ui_setFilterBW(const uint32_t _filter_bw);
void hackrf_ui_setLNAPower(bool _lna_on);
void hackrf_ui_setBBLNAGain(const uint32_t gain_db);
void hackrf_ui_setBBVGAGain(const uint32_t gain_db);
void hackrf_ui_setBBTXVGAGain(const uint32_t gain_db);

#endif
