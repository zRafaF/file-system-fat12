#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer_operation.h"
#include "defines.h"
#include "fat12.h"

#define STB_DS_IMPLEMENTATION
#include "cli_menu.h"
#include "stb_ds.h"

void option1() { printf("Option 1 selected!\n"); }
void option2() { printf("Option 2 chosen!\n"); }
void option3() { printf("Option 3 picked!\n"); }
void quit() {
    exit(0);
}

int main() {
    FILE *disk = fopen(IMG_PATH, "rb");
    if (disk == NULL) {
        perror("Failed to open disk image");
        return 1;
    }

    fat12_boot_sector_s boot_sector = fat12_read_boot_sector(disk);
    fat12_print_boot_sector_info(boot_sector);

    fat12_load_full_fat_table(disk);  // Read the boot sector again to ensure it's loaded

    for (int i = 0; i < 8; i++) {
        uint16_t entry = fat12_get_table_entry(i);
        printf("Entry %2u: 0x%03X\n", i, entry);
    }

    for (int i = 0; i < 3; i++) {
        fat12_directory_s dir = fat12_read_directory_entry(disk, i);  // Read the root directory entry

        fat12_print_directory_info(dir);
    }
    // Sector Buffer
    uint8_t buffer[SECTOR_SIZE];

    fat12_read_cluster(disk, buffer, 19);  // Read first fat12 table cluster

    // printf("Cluster 1 Data:\n");
    // bo_print_buffer(buffer, SECTOR_SIZE);

    int *array = NULL;
    arrput(array, 2);
    arrput(array, 3);
    arrput(array, 5);
    for (int i = 0; i < arrlen(array); ++i)
        printf("%d ", array[i]);

    Menu *main_menu = menu_create("MAIN MENU");

    menu_add_item(main_menu, "Start", option1);
    menu_add_item(main_menu, "Settings", option2);
    menu_add_item(main_menu, "Help", option3);
    menu_add_item(main_menu, "Quit", quit);

    menu_run(main_menu);
    menu_free(main_menu);

    return 0;
}
