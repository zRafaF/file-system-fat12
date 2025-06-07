#ifndef BUFFER_OPERATION_H
#define BUFFER_OPERATION_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "defines.h"

// Imprime o conteúdo de um buffer de bytes em formato hexadecimal
void bo_print_buffer(uint8_t *buffer, int size);

#endif  // BUFFER_OPERATION_H