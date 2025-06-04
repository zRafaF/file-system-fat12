#ifndef CLI_MENU_H
#define CLI_MENU_H

#include <stddef.h>

typedef void (*MenuCallback)(void);

typedef struct {
    char* title;
    MenuCallback callback;
} MenuItem;

typedef struct {
    char* title;
    MenuItem* items;
} Menu;

// Creates a new menu with title
Menu* menu_create(const char* title);

// Adds item to a menu
void menu_add_item(Menu* menu, const char* title, MenuCallback callback);

// Displays and runs menu interaction
void menu_run(Menu* menu);

// Cleans up menu resources
void menu_free(Menu* menu);

#endif