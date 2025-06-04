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

// Menu callbacks
void option1(Menu* menu) { printf("Option 1 selected!\n"); }
void option2(Menu* menu) { printf("Option 2 chosen!\n"); }

// Back callback
void back_callback(Menu* menu) {
    menu_back(menu);
}

// Quit callback
void quit_callback(Menu* menu) {
    menu_quit(menu);
}

// Input callback
char* input_callback(const char* prompt) {
    return menu_get_input(prompt);
}
int main() {
    FILE* disk = fopen(IMG_PATH, "rb");
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

    int* array = NULL;
    arrput(array, 2);
    arrput(array, 3);
    arrput(array, 5);
    for (int i = 0; i < arrlen(array); ++i)
        printf("%d ", array[i]);

    Menu* main_menu = menu_create("MAIN MENU", NULL);

    // Create settings submenu
    Menu* settings_menu = menu_create("SETTINGS", main_menu);
    menu_add_item(settings_menu, "Sound Settings", option1);
    menu_add_item(settings_menu, "Display Settings", option2);
    menu_add_item(settings_menu, "Back", back_callback);

    // Create help submenu
    Menu* help_menu = menu_create("HELP", main_menu);
    menu_add_item(help_menu, "View Tutorial", option1);
    menu_add_item(help_menu, "FAQ", option2);
    menu_add_item(help_menu, "Back", back_callback);

    // Add items to main menu
    menu_add_item(main_menu, "Start Game", option1);
    menu_add_submenu(main_menu, "Settings", settings_menu);
    menu_add_submenu(main_menu, "Help", help_menu);
    menu_add_input(main_menu, "Enter Player Name", input_callback);
    menu_add_item(main_menu, "Quit", quit_callback);

    // Run the menu system
    menu_run(main_menu);

    // Cleanup
    menu_free(help_menu);
    menu_free(settings_menu);
    menu_free(main_menu);

    printf("\nGoodbye!\n");
    return 0;

    return 0;
}
