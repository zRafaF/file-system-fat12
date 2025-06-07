#define STB_DS_IMPLEMENTATION

#include "app.h"
#include "cli_menu.h"
#include "menu.h"
#include "stb_ds.h"  // Precisa ser incluido

int main(void) {
    Menu* unmounted_menu = menu_create("MENU PRINCIPAL", NULL);
    Menu* mounted_menu = menu_create("OPERACOES DE DISCO", NULL);

    init_menus(unmounted_menu, mounted_menu);

    // Loop, alternando entre os dois menus até o usuário escolher “Quit”
    while (!should_exit()) {
        if (!app_is_mounted()) {
            menu_run(unmounted_menu);
        } else {
            menu_run(mounted_menu);
        }
    }

    free_menus(unmounted_menu, mounted_menu);

    return 0;
}
