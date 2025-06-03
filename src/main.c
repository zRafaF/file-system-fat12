#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "fat12.h"

int main() {
    FILE *disk = fopen(IMG_PATH, "rb");
    if (disk == NULL) {
        perror("Failed to open disk image");
        return 1;
    }

    fat12_boot_sector_s boot_sector = fat12_read_boot_sector(disk);
    fat12_print_boot_sector_info(boot_sector);

    fat12_load_full_fat_table(disk);  // Read the boot sector again to ensure it's loaded

    for (int i = 0; i < 10; i++) {
        uint16_t entry = fat12_get_table_entry(i);
        printf("Entry %2u: 0x%03X\n", i, entry);
    }

    fat12_directory_s dir = fat12_read_directory_entry(disk, 0);  // Read the root directory entry

    fat12_print_directory_info(dir);

    // Sector Buffer
    uint8_t buffer[SECTOR_SIZE];

    fat12_read_cluster(disk, buffer, 19);  // Read first fat12 table cluster

    printf("Cluster 1 Data:\n");
    for (int i = 0; i < SECTOR_SIZE; i++) {
        printf("%02X ", buffer[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    return 0;
}
