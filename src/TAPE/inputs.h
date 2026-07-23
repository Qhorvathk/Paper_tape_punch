#ifndef INPUTS_H
#define INPUTS_H

#include <stdbool.h>

extern volatile bool Ready;
extern volatile bool PaperOut;

void inputs_setup(void);

#endif
void Bits_out(uint8_t value);