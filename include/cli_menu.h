#ifndef CLI_MENU_H
#define CLI_MENU_H

#include <stddef.h>

typedef struct Menu Menu;
typedef struct MenuFlow MenuFlow;

typedef void (*MenuCallback)(Menu* menu);
typedef void (*InputCallback)(Menu* menu, const char* input);
typedef void (*FlowStepCallback)(MenuFlow* flow);

typedef enum {
    MENU_ITEM_REGULAR,
    MENU_ITEM_SUBMENU,
    MENU_ITEM_INPUT,
    MENU_ITEM_FLOW
} MenuItemType;

typedef struct {
    char* title;
    MenuItemType type;
    union {
        MenuCallback callback;
        Menu* submenu;
        InputCallback input_callback;
        MenuFlow* flow;
    };
    int selected;
} MenuItem;

struct Menu {
    char* title;
    Menu* parent;
    MenuItem* items;
    int should_quit;
    void* user_data;
    int selected_index;
};

struct MenuFlow {
    char* title;
    Menu** steps;
    int current_step;
    void* data;
    FlowStepCallback on_complete;
    FlowStepCallback on_cancel;
};

// Menu functions
Menu* menu_create(const char* title, Menu* parent);
void menu_add_item(Menu* menu, const char* title, MenuCallback callback);
void menu_add_submenu(Menu* menu, const char* title, Menu* submenu);
void menu_add_input(Menu* menu, const char* title, InputCallback input_callback);
void menu_add_flow(Menu* menu, const char* title, MenuFlow* flow);
void menu_run(Menu* menu);
void menu_free(Menu* menu);
void menu_back(Menu* menu);
void menu_quit(Menu* menu);
char* menu_get_input(const char* prompt, int visible);

// Flow functions
MenuFlow* flow_create(const char* title,
                      FlowStepCallback on_complete,
                      FlowStepCallback on_cancel);
void flow_add_step(MenuFlow* flow, Menu* step);
void flow_start(MenuFlow* flow);
void flow_free(MenuFlow* flow);
void flow_next_step(MenuFlow* flow);
void flow_previous_step(MenuFlow* flow);
void flow_complete(MenuFlow* flow);
void flow_cancel(MenuFlow* flow);

#endif