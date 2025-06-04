#ifndef CLI_MENU_H
#define CLI_MENU_H

#include <stddef.h>

typedef struct Menu Menu;

typedef void (*MenuCallback)(Menu* menu);
typedef void (*InputCallback)(Menu* menu, const char* input);

typedef enum {
    MENU_ITEM_REGULAR,
    MENU_ITEM_SUBMENU,
    MENU_ITEM_INPUT
} MenuItemType;

typedef struct {
    char* title;
    MenuItemType type;
    union {
        MenuCallback callback;
        Menu* submenu;
        InputCallback input_callback;
    };
} MenuItem;

struct Menu {
    char* title;
    Menu* parent;
    MenuItem* items;
    int should_quit;
};

Menu* menu_create(const char* title, Menu* parent);
void menu_add_item(Menu* menu, const char* title, MenuCallback callback);
void menu_add_submenu(Menu* menu, const char* title, Menu* submenu);
void menu_add_input(Menu* menu, const char* title, InputCallback input_callback);
void menu_run(Menu* menu);
void menu_free(Menu* menu);
void menu_back(Menu* menu);
void menu_quit(Menu* menu);
char* menu_get_input(const char* prompt);

#endif  // CLI_MENU_H