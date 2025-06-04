#include "cli_menu.h"

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

Menu* menu_create(const char* title) {
    Menu* menu = malloc(sizeof(Menu));
    menu->title = strdup(title);
    menu->items = NULL;
    return menu;
}

void menu_add_item(Menu* menu, const char* title, MenuCallback callback) {
    MenuItem item = {
        .title = strdup(title),
        .callback = callback};
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

#ifdef _WIN32
static void enable_raw_mode() {
    // No special handling needed for Windows in this implementation
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
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0;
    if (seq[0] == '\033') {
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\033';
        if (seq[1] == '[') {
            switch (seq[2]) {
                case 'A':
                    return 'A';  // Up arrow
                case 'B':
                    return 'B';  // Down arrow
            }
        }
        return '\033';
    }
    return seq[0];
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

void menu_run(Menu* menu) {
    if (arrlen(menu->items) == 0) return;

    int selected = 0;
    enable_raw_mode();

    while (1) {
        print_menu(menu, selected);
        int key = read_key();

        if (key == 'A') {  // Up
            selected = (selected > 0) ? selected - 1 : arrlen(menu->items) - 1;
        } else if (key == 'B') {  // Down
            selected = (selected < arrlen(menu->items) - 1) ? selected + 1 : 0;
        } else if (key == '\r' || key == '\n') {  // Enter
            clear_screen();
            menu->items[selected].callback();
            printf("\n\nPress any key to continue...");
            read_key();
        } else if (key == 'q' || key == 'Q') {
            break;
        }
    }
}