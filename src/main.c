#define STB_DS_IMPLEMENTATION

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer_operation.h"
#include "cli_menu.h"
#include "defines.h"
#include "fat12.h"
#include "stb_ds.h"

typedef struct {
    int move_type;  // 0=copy, 1=cut
    char* origin_path;
    char* output_path;
} MoveContext;

// Callbacks
void back_callback(Menu* menu) { menu_back(menu); }
void quit_callback(Menu* menu) { menu_quit(menu); }

// Flow step handlers
void handle_move_type(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    ctx->move_type = menu->selected_index;  // 0=Copy, 1=Cut
    flow_next_step(flow);
}

void handle_origin_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    if (ctx->origin_path) free(ctx->origin_path);
    ctx->origin_path = strdup(input);
    flow_next_step(flow);
}

void handle_output_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    if (ctx->output_path) free(ctx->output_path);
    ctx->output_path = strdup(input);
    flow_next_step(flow);
}

void handle_confirm(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_complete(flow);
}

void handle_cancel(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_cancel(flow);
}

// Completion handlers
void move_complete(MenuFlow* flow) {
    MoveContext* ctx = (MoveContext*)flow->data;
    printf("\nOperation completed:\n");
    printf("Type: %s\n", ctx->move_type ? "Cut" : "Copy");
    printf("Origin: %s\n", ctx->origin_path);
    printf("Destination: %s\n", ctx->output_path);

    // Cleanup
    free(ctx->origin_path);
    free(ctx->output_path);
    free(ctx);
    flow_free(flow);

    printf("\nPress any key to continue...");
    getchar();
}

void move_cancel(MenuFlow* flow) {
    MoveContext* ctx = (MoveContext*)flow->data;
    printf("\nOperation cancelled\n");

    // Cleanup
    if (ctx->origin_path) free(ctx->origin_path);
    if (ctx->output_path) free(ctx->output_path);
    free(ctx);
    flow_free(flow);

    printf("\nPress any key to continue...");
    getchar();
}

void setup_move_flow(Menu* parent_menu) {
    // Create context
    MoveContext* ctx = malloc(sizeof(MoveContext));
    ctx->move_type = -1;
    ctx->origin_path = NULL;
    ctx->output_path = NULL;

    // Create flow
    MenuFlow* flow = flow_create("Move File", move_complete, move_cancel);
    flow->data = ctx;

    // Create steps
    Menu* step1 = menu_create("Select Move Type", NULL);
    menu_add_item(step1, "Copy", handle_move_type);
    menu_add_item(step1, "Cut", handle_move_type);
    menu_add_item(step1, "Back", back_callback);

    Menu* step2 = menu_create("Enter Origin Path", NULL);
    menu_add_input(step2, "Path: ", handle_origin_path);
    menu_add_item(step2, "Back", back_callback);

    Menu* step3 = menu_create("Enter Output Path", NULL);
    menu_add_input(step3, "Path: ", handle_output_path);
    menu_add_item(step3, "Back", back_callback);

    Menu* step4 = menu_create("Confirm", NULL);
    menu_add_item(step4, "Confirm Operation", handle_confirm);
    menu_add_item(step4, "Cancel", handle_cancel);

    // Add steps to flow
    flow_add_step(flow, step1);
    flow_add_step(flow, step2);
    flow_add_step(flow, step3);
    flow_add_step(flow, step4);

    // Add flow to parent menu
    menu_add_flow(parent_menu, "Move File", flow);
}

int main() {
    Menu* main_menu = menu_create("Main Menu", NULL);

    // Add regular items
    menu_add_item(main_menu, "Option 1", NULL);
    menu_add_item(main_menu, "Option 2", NULL);

    // Add the move file flow
    setup_move_flow(main_menu);

    // Add quit option
    menu_add_item(main_menu, "Quit", quit_callback);

    // Run menu system
    menu_run(main_menu);

    // Cleanup
    menu_free(main_menu);
    return 0;
}