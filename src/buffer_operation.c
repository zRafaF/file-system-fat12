#include "buffer_operation.h"

void bo_print_buffer(uint8_t *buffer, int size) {
    assert(buffer != NULL);
    assert(size > 0 && size <= SECTOR_SIZE);

    for (int i = 0; i < size; i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (size % 16 != 0) {
        printf("\n");
    }
}