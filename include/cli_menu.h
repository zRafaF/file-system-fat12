#ifndef CLI_MENU_H
#define CLI_MENU_H

#include <stddef.h>

typedef struct Menu Menu;

typedef void (*MenuCallback)(Menu* menu);
typedef char* (*InputCallback)(const char* prompt);

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

// Creates a new menu with title
Menu* menu_create(const char* title, Menu* parent);

// Adds regular item to a menu
void menu_add_item(Menu* menu, const char* title, MenuCallback callback);

// Adds submenu item
void menu_add_submenu(Menu* menu, const char* title, Menu* submenu);

// Adds input item
void menu_add_input(Menu* menu, const char* title, InputCallback input_callback);

// Displays and runs menu interaction
void menu_run(Menu* menu);

// Cleans up menu resources
void menu_free(Menu* menu);

// Helper function to go back to parent menu
void menu_back(Menu* menu);

// Function to get user input
char* menu_get_input(const char* prompt);

// Mark menu to quit
void menu_quit(Menu* menu);

#endif