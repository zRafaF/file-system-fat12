#define STB_DS_IMPLEMENTATION

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer_operation.h"
#include "cli_menu.h"
#include "defines.h"
#include "fat12.h"
#include "stb_ds.h"

// Menu callbacks
void option1(Menu* menu) {
    UNUSED(menu);
    printf("Option 1 selected!\n");
}
void option2(Menu* menu) {
    UNUSED(menu);
    printf("Option 2 chosen!\n");
}

// Back callback
void back_callback(Menu* menu) {
    menu_back(menu);
}

// Quit callback
void quit_callback(Menu* menu) {
    menu_quit(menu);
}

// Input processing callback
void process_input(Menu* menu, const char* input) {
    UNUSED(menu);

    // Create a copy to process
    char* processed = strdup(input);
    char* src = processed;
    char* dst = processed;

    // Remove all 'a' characters (case-insensitive)
    while (*src) {
        if (*src != 'a') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';

    printf("Processed input: %s\n", processed);
    free(processed);
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

    Menu* main_menu = menu_create("MENU", NULL);

    Menu* copy_disk_sys_menu = menu_create("COPIAR DISCO -> SISTEMA", main_menu);
    menu_add_input(copy_disk_sys_menu, "Caminho de origem", process_input);
    menu_add_input(copy_disk_sys_menu, "Caminho de destino", process_input);
    menu_add_item(copy_disk_sys_menu, "Copiar", option1);
    menu_add_item(copy_disk_sys_menu, "Voltar", back_callback);

    Menu* copy_sys_disk_menu = menu_create("COPIAR SISTEMA -> DISCO", main_menu);
    menu_add_input(copy_sys_disk_menu, "Caminho de origem", process_input);
    menu_add_input(copy_sys_disk_menu, "Caminho de destino", process_input);
    menu_add_item(copy_sys_disk_menu, "Copiar", option1);
    menu_add_item(copy_sys_disk_menu, "Voltar", back_callback);

    // Add items to main menu
    menu_add_item(main_menu, "Montar imagem", option1);
    menu_add_item(main_menu, "ls-1", option2);
    menu_add_item(main_menu, "ls", option2);
    menu_add_submenu(main_menu, "Copiar disco -> sistema", copy_disk_sys_menu);
    menu_add_submenu(main_menu, "Copiar sistema -> disco", copy_sys_disk_menu);
    menu_add_item(main_menu, "Sair", quit_callback);

    // Run the menu system
    menu_run(main_menu);

    // Cleanup
    menu_free(copy_disk_sys_menu);
    menu_free(main_menu);

    printf("\nObrigado por usar!\n");
    return 0;
}
