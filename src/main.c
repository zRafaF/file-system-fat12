#define STB_DS_IMPLEMENTATION

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_operation.h"
#include "cli_menu.h"
#include "defines.h"
#include "fat12.h"
#include "stb_ds.h"

typedef struct {
    int move_type;  // 0 = Copy, 1 = Cut
    char* origin_path;
    char* output_path;
} MoveContext;

//----------------------------------------------------------------
// Callbacks for “Back” and “Quit” in non‐flow menus:
void back_callback(Menu* menu) {
    menu_back(menu);
}

void quit_callback(Menu* menu) {
    menu_quit(menu);
}

//----------------------------------------------------------------
// Flow‐step callbacks:

// Step 1: user picks “Copy” (index=0) or “Cut” (index=1)
void handle_move_type(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    // menu->selected_index was set by menu_run()
    ctx->move_type = menu->selected_index;  // 0 = Copy, 1 = Cut
    flow_next_step(flow);
}

// Step 2: read origin path from input string
void handle_origin_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    if (ctx->origin_path) {
        free(ctx->origin_path);
    }
    ctx->origin_path = strdup(input);
    flow_next_step(flow);
}

// Step 3: read destination path from input string
void handle_output_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    MoveContext* ctx = (MoveContext*)flow->data;

    if (ctx->output_path) {
        free(ctx->output_path);
    }
    ctx->output_path = strdup(input);
    flow_next_step(flow);
}

// Step 4: user confirms the operation
void handle_confirm(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_complete(flow);
}

// Step 4 alternative: user cancels the operation
void handle_cancel(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_cancel(flow);
}

// “Back” inside steps 2 or 3 should go to the previous step in the flow
void handle_flow_back(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_previous_step(flow);
}

//----------------------------------------------------------------
// Completion handlers (moved‐or‐cut flow):

void move_complete(MenuFlow* flow) {
    MoveContext* ctx = (MoveContext*)flow->data;
    printf("\nOperation completed:\n");
    printf("  Type: %s\n", (ctx->move_type ? "Cut" : "Copy"));
    printf("  Origin: %s\n", ctx->origin_path);
    printf("  Destination: %s\n", ctx->output_path);

    // Instead of freeing the flow or context, we only free the two strings
    // and reset the MoveContext fields. That way, the same flow object can
    // be re‐used if the user picks “Move File” again.
    if (ctx->origin_path) {
        free(ctx->origin_path);
        ctx->origin_path = NULL;
    }
    if (ctx->output_path) {
        free(ctx->output_path);
        ctx->output_path = NULL;
    }
    ctx->move_type = -1;

    printf("\nPress any key to continue...");
    getchar();
}

void move_cancel(MenuFlow* flow) {
    MoveContext* ctx = (MoveContext*)flow->data;
    printf("\nOperation cancelled\n");

    if (ctx->origin_path) {
        free(ctx->origin_path);
        ctx->origin_path = NULL;
    }
    if (ctx->output_path) {
        free(ctx->output_path);
        ctx->output_path = NULL;
    }
    ctx->move_type = -1;

    printf("\nPress any key to continue...");
    getchar();
}

void handle_option(Menu* menu) {
    // This function is a placeholder for handling other menu options.
    // It can be expanded to handle specific actions for other menu items.
    printf("Option selected: %s\n", menu->items[menu->selected_index].title);
}

//----------------------------------------------------------------
// Build the “Move File” flow and attach it (once) to the main menu:
void setup_move_flow(Menu* parent_menu) {
    // Allocate a single MoveContext that we will re‐use each time the flow runs.
    MoveContext* ctx = malloc(sizeof(MoveContext));
    ctx->move_type = -1;
    ctx->origin_path = NULL;
    ctx->output_path = NULL;

    // Create the flow (only once)
    MenuFlow* flow = flow_create("Move File", move_complete, move_cancel);
    flow->data = ctx;

    // Step 1: Select “Copy” or “Cut”
    Menu* step1 = menu_create("Select Move Type", NULL);
    menu_add_item(step1, "Copy", handle_move_type);
    menu_add_item(step1, "Cut", handle_move_type);
    // If user presses Back on step 1, cancel the flow
    menu_add_item(step1, "Back", handle_cancel);

    // Step 2: Get origin path
    Menu* step2 = menu_create("Enter Origin Path", NULL);
    menu_add_input(step2, "Path: ", handle_origin_path);
    // Back goes to step 1
    menu_add_item(step2, "Back", handle_flow_back);

    // Step 3: Get destination path
    Menu* step3 = menu_create("Enter Output Path", NULL);
    menu_add_input(step3, "Path: ", handle_output_path);
    // Back goes to step 2
    menu_add_item(step3, "Back", handle_flow_back);

    // Step 4: Confirm vs Cancel
    Menu* step4 = menu_create("Confirm Operation", NULL);
    menu_add_item(step4, "Confirm", handle_confirm);
    menu_add_item(step4, "Cancel", handle_cancel);

    // Link steps into the flow in order
    flow_add_step(flow, step1);
    flow_add_step(flow, step2);
    flow_add_step(flow, step3);
    flow_add_step(flow, step4);

    // Finally, put the flow entry into the parent (“Main”) menu
    menu_add_flow(parent_menu, "Move File", flow);
}

int main() {
    Menu* main_menu = menu_create("Main Menu", NULL);

    // Two dummy options
    menu_add_item(main_menu, "Option 1", handle_option);
    menu_add_item(main_menu, "Option 2", NULL);

    // Attach “Move File” flow into the main menu (only once)
    setup_move_flow(main_menu);

    // A “Quit” item on the main menu
    menu_add_item(main_menu, "Quit", quit_callback);

    // Run the top‐level menu loop
    menu_run(main_menu);

    // ── Cleanup on exit ──
    // We only allocated one MoveContext inside setup_move_flow, plus one MenuFlow + its steps.
    // Free everything now. (In practice you could loop over submenus to free them; here’s a minimal approach.)

    // 1) Free the MoveContext we allocated:
    MenuItem* flow_item = NULL;
    for (int i = 0; i < arrlen(main_menu->items); i++) {
        if (main_menu->items[i].type == MENU_ITEM_FLOW) {
            flow_item = &main_menu->items[i];
            break;
        }
    }
    if (flow_item) {
        MenuFlow* flow = (MenuFlow*)flow_item->flow;
        MoveContext* ctx = (MoveContext*)flow->data;
        if (ctx) {
            if (ctx->origin_path) free(ctx->origin_path);
            if (ctx->output_path) free(ctx->output_path);
            free(ctx);
        }
        // 2) Free each step‐menu
        for (int s = 0; s < arrlen(flow->steps); s++) {
            menu_free(flow->steps[s]);
        }
        // 3) Finally free the flow itself
        flow_free(flow);
    }

    // 4) Free the main menu
    menu_free(main_menu);
    return 0;
}
