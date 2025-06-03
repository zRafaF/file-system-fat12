#ifndef BUFFER_OPERATION_H
#define BUFFER_OPERATION_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "defines.h"

uint16_t bo_read_2_bytes(uint8_t *buffer, int offset);

void bo_print_buffer(uint8_t *buffer, int size);

#endif  // BUFFER_OPERATION_H