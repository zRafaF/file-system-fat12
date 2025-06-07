#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

#include "cli_menu.h"

typedef struct {
    int copy_type;  // 0 = Disco -> Sistema, 1 = Sistema -> Disco
    char *origin_path;
    char *output_path;
} CopyContext;

void init_menus(Menu *unmounted_menu, Menu *mounted_menu);
void free_menus(Menu *unmounted_menu, Menu *mounted_menu);

bool should_exit();

#endif  // MENU_H
