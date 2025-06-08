#include "cli_menu.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_ds.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

static void clear_screen();
static void print_menu(Menu* menu, int selected);
static int read_key();
static void enable_raw_mode();
static void disable_raw_mode();

Menu* menu_create(const char* title, Menu* parent) {
    Menu* menu = malloc(sizeof(Menu));
    menu->title = strdup(title);
    menu->parent = parent;
    menu->items = NULL;
    menu->should_quit = 0;
    menu->user_data = NULL;
    menu->selected_index = 0;
    return menu;
}

void menu_add_item(Menu* menu, const char* title, MenuCallback callback) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_REGULAR,
        .callback = callback,
        .selected = 0};
    arrput(menu->items, item);
}

void menu_add_submenu(Menu* menu, const char* title, Menu* submenu) {
    submenu->parent = menu;
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_SUBMENU,
        .submenu = submenu,
        .selected = 0};
    arrput(menu->items, item);
}

void menu_add_input(Menu* menu, const char* title, InputCallback input_callback) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_INPUT,
        .input_callback = input_callback,
        .selected = 0};
    arrput(menu->items, item);
}

void menu_free(Menu* menu) {
    free(menu->title);
    for (int i = 0; i < arrlen(menu->items); i++) {
        free(menu->items[i].title);
    }
    arrfree(menu->items);
    free(menu);
}

void menu_back(Menu* menu) {
    menu->should_quit = 1;
}

void menu_quit(Menu* menu) {
    Menu* top = menu;
    while (top->parent) {
        top = top->parent;
    }
    top->should_quit = 1;
}

#ifdef _WIN32
static void enable_raw_mode() {
    // Nothing needed for Windows
}

static void disable_raw_mode() {
    // Nothing needed for Windows
}

static int read_key() {
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        switch (_getch()) {
            case 72:
                return 'A';  // Up
            case 80:
                return 'B';  // Down
            default:
                return 0;
        }
    }
    return ch;
}
#else
static struct termios orig_termios;

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int read_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;

    if (c == '\033') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                    return 'A';  // Up arrow
                case 'B':
                    return 'B';  // Down arrow
            }
        }
        return '\033';
    }
    return c;
}
#endif

static void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void print_menu(Menu* menu, int selected) {
    clear_screen();
    printf("\n===== %s =====\n\n", menu->title);

    for (int i = 0; i < arrlen(menu->items); i++) {
        if (i == selected) {
            printf("> [ %s ] <\n", menu->items[i].title);
        } else {
            printf("  %s\n", menu->items[i].title);
        }
    }
    printf("\nUse as setas para navegar e Enter para selecionar.");
    fflush(stdout);
}

char* menu_get_input(const char* prompt, int visible) {
    disable_raw_mode();  // Restore canonical mode for compatibility

    printf("%s", prompt);
    fflush(stdout);

    enable_raw_mode();  // Enable raw mode for immediate key reading

    char* input = NULL;
    int capacity = 0;
    int len = 0;
    int c;

    while (1) {
        c = read_key();
        if (c == '\r' || c == '\n') {
            break;
        } else if (c == 127 || c == 8) {  // Backspace or DEL
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (isprint(c)) {
            if (len + 1 >= capacity) {
                capacity = capacity == 0 ? 32 : capacity * 2;
                input = realloc(input, capacity);
            }
            input[len++] = c;
            if (visible) {
                putchar(c);
                fflush(stdout);
            }
        }
    }

    if (input) {
        input[len] = '\0';
    } else {
        input = strdup("");
    }

    putchar('\n');
    fflush(stdout);
    disable_raw_mode();  // Go back to cooked mode
    return input;
}

void menu_wait_for_any_key(void) {
    printf("\nPressione qualquer tecla para continuar...");
    fflush(stdout);
    read_key();
}

void menu_add_flow(Menu* menu, const char* title, MenuFlow* flow) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_FLOW,
        .flow = flow,
        .selected = 0};
    arrput(menu->items, item);
}

// ───────────── Flow Management ─────────────

MenuFlow* flow_create(const char* title,
                      FlowStepCallback on_complete,
                      FlowStepCallback on_cancel) {
    MenuFlow* flow = malloc(sizeof(MenuFlow));
    flow->title = strdup(title);
    flow->steps = NULL;
    flow->current_step = 0;
    flow->data = NULL;
    flow->on_complete = on_complete;
    flow->on_cancel = on_cancel;
    return flow;
}

void flow_add_step(MenuFlow* flow, Menu* step) {
    step->user_data = flow;
    arrput(flow->steps, step);
}

void flow_start(MenuFlow* flow) {
    if (arrlen(flow->steps) > 0) {
        flow->current_step = 0;
        menu_run(flow->steps[0]);
    }
}

void flow_free(MenuFlow* flow) {
    free(flow->title);
    arrfree(flow->steps);
    free(flow);
}

void flow_next_step(MenuFlow* flow) {
    if (flow->current_step + 1 < arrlen(flow->steps)) {
        flow->current_step++;
        menu_run(flow->steps[flow->current_step]);
    } else {
        flow_complete(flow);
    }
}

void flow_previous_step(MenuFlow* flow) {
    if (flow->current_step > 0) {
        flow->current_step--;
        menu_run(flow->steps[flow->current_step]);
    } else {
        flow_cancel(flow);
    }
}

void flow_complete(MenuFlow* flow) {
    if (flow->on_complete) {
        flow->on_complete(flow);
    }
}

void flow_cancel(MenuFlow* flow) {
    if (flow->on_cancel) {
        flow->on_cancel(flow);
    }
}

// ───────────── Main Loop for Each Menu ─────────────

void menu_run(Menu* menu) {
    if (arrlen(menu->items) == 0) return;

    int selected = 0;
    enable_raw_mode();

    while (!menu->should_quit) {
        print_menu(menu, selected);
        int key = read_key();

        if (key == 'A') {  // Up arrow
            selected = (selected > 0) ? selected - 1 : arrlen(menu->items) - 1;
        } else if (key == 'B') {  // Down arrow
            selected = (selected < arrlen(menu->items) - 1) ? selected + 1 : 0;
        } else if (key == '\r' || key == '\n') {  // Enter
            MenuItem* item = &menu->items[selected];
            menu->selected_index = selected;

            switch (item->type) {
                case MENU_ITEM_REGULAR: {
                    clear_screen();
                    if (item->callback) {
                        item->callback(menu);
                    }
                    // If this menu is part of a flow, exit immediately so the flow logic can move on.
                    if (menu->user_data) {
                        menu->should_quit = 1;
                        break;
                    }
                    // Otherwise, wait for any key before redrawing
                    if (!menu->should_quit) {
                        menu_wait_for_any_key();
                    }
                    break;
                }

                case MENU_ITEM_SUBMENU: {
                    clear_screen();
                    menu_run(item->submenu);
                    // After returning from submenu, reset quit/selection so this parent redraws
                    menu->should_quit = 0;
                    selected = 0;
                    break;
                }

                case MENU_ITEM_INPUT: {
                    clear_screen();
                    char* input = menu_get_input(item->title, 1);
                    if (item->input_callback) {
                        item->input_callback(menu, input);
                    }
                    free(input);
                    // If part of a flow, exit so next step can run
                    if (menu->user_data) {
                        menu->should_quit = 1;
                        break;
                    }
                    if (!menu->should_quit) {
                        menu_wait_for_any_key();
                    }
                    break;
                }

                case MENU_ITEM_FLOW: {
                    clear_screen();
                    // Start (or re‐start) the flow. flow_start() will take care of running step 0,1,2...
                    flow_start(item->flow);
                    // Once flow completes or cancels, we come back here—redraw parent
                    menu->should_quit = 0;
                    selected = 0;
                    break;
                }
            }
        }
    }

    // When quitting this menu, reset should_quit so that a parent menu can continue if needed
    menu->should_quit = 0;
}
