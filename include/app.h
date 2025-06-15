#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer_operation.h"
#include "cli_menu.h"
#include "defines.h"
#include "fat12.h"
#include "file_system.h"

// Returns true if a disk image is currently mounted
bool app_is_mounted(void);

// menu callbacks
void app_mount_callback(Menu *m);
void app_unmount_callback(Menu *m);
void app_boot_sector_callback(Menu *m);
void app_ls1_callback(Menu *m);
void app_ls_callback(Menu *m);
void app_rm_callback(Menu *m, const char *input);
void app_debug1_callback(Menu *m);
void app_debug2_callback(Menu *m);

void app_quick_actions_reset_images_callback(Menu *m);
void app_quick_actions_copy_to_disk_callback(Menu *m);
void app_quick_actions_copy_to_system_callback(Menu *m);
void app_quick_actions_list_fat12_table_callback(Menu *m);

void app_copy_complete(int copy_type, const char *src, const char *dst);

#endif  // APP_H
