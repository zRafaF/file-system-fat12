#include "buffer_operation.h"

uint16_t bo_read_2_bytes(uint8_t *buffer, int offset) {
    assert(buffer != NULL);
    assert(offset >= 0 && offset < SECTOR_SIZE - 1);

    return (buffer[offset] | (buffer[offset + 1] << 8));
}
