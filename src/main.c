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

    uint16_t value = fat12_get_table_entry(1);  // Get the FAT entry for cluster 2
    printf("FAT entry for 2: %03X\n", value);

    // Sector Buffer
    uint8_t buffer[SECTOR_SIZE];

    fat12_read_cluster(disk, buffer, 1);  // Read first fat12 table cluster

    printf("Cluster 1 Data:\n");
    for (int i = 0; i < SECTOR_SIZE; i++) {
        printf("%02X ", buffer[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    return 0;
}
