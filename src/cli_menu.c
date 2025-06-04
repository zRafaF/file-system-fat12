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

// Internal functions
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
    return menu;
}

void menu_add_item(Menu* menu, const char* title, MenuCallback callback) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_REGULAR,
        .callback = callback};
    arrput(menu->items, item);
}

void menu_add_submenu(Menu* menu, const char* title, Menu* submenu) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_SUBMENU,
        .submenu = submenu};
    arrput(menu->items, item);
}

void menu_add_input(Menu* menu, const char* title, InputCallback input_callback) {
    MenuItem item = {
        .title = strdup(title),
        .type = MENU_ITEM_INPUT,
        .input_callback = input_callback};
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
    // Propagate quit to top-level menu
    Menu* top = menu;
    while (top->parent) {
        top = top->parent;
    }
    top->should_quit = 1;
}

#ifdef _WIN32
static void enable_raw_mode() {
    // No special handling needed for Windows in this implementation
}

static void disable_raw_mode() {
    // No special handling
}

static int read_key() {
    int ch = _getch();
    if (ch == 0 || ch == 224) {  // Arrow keys
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
    printf("===== %s =====\n\n", menu->title);

    for (int i = 0; i < arrlen(menu->items); i++) {
        if (i == selected) {
            printf("> [ %s ] <\n", menu->items[i].title);
        } else {
            printf("  %s\n", menu->items[i].title);
        }
    }
    printf("\nUse arrow keys to navigate, Enter to select");
}

char* menu_get_input(const char* prompt) {
    disable_raw_mode();  // Return to normal terminal mode for input

    printf("%s", prompt);
    fflush(stdout);

    // Read input line
    char* input = NULL;
    int capacity = 0;
    int len = 0;
    int c;

    while ((c = getchar()) != '\n' && c != EOF) {
        if (len + 1 >= capacity) {
            capacity = capacity == 0 ? 32 : capacity * 2;
            input = realloc(input, capacity);
        }
        input[len++] = c;
    }

    if (input) {
        input[len] = '\0';
    } else {
        input = strdup("");
    }

    enable_raw_mode();  // Return to raw mode for menu navigation
    return input;
}

void menu_run(Menu* menu) {
    if (arrlen(menu->items) == 0) return;

    int selected = 0;
    enable_raw_mode();

    while (!menu->should_quit) {
        print_menu(menu, selected);
        int key = read_key();

        if (key == 'A') {  // Up
            selected = (selected > 0) ? selected - 1 : arrlen(menu->items) - 1;
        } else if (key == 'B') {  // Down
            selected = (selected < arrlen(menu->items) - 1) ? selected + 1 : 0;
        } else if (key == '\r' || key == '\n') {  // Enter
            MenuItem* item = &menu->items[selected];

            switch (item->type) {
                case MENU_ITEM_REGULAR:
                    clear_screen();
                    item->callback(menu);
                    if (!menu->should_quit) {
                        printf("\n\nPress any key to continue...");
                        read_key();
                    }
                    break;

                case MENU_ITEM_SUBMENU:
                    clear_screen();
                    menu_run(item->submenu);
                    // Reset should_quit after returning from submenu
                    menu->should_quit = 0;
                    selected = 0;
                    break;

                case MENU_ITEM_INPUT:
                    clear_screen();
                    char* input = item->input_callback("Enter text: ");
                    printf("\nYou entered: %s\n", input);
                    free(input);
                    printf("\nPress any key to continue...");
                    read_key();
                    break;
            }
        }
    }

    // Reset quit flag when returning to parent
    menu->should_quit = 0;
}