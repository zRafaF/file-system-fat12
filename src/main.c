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

// ----------------------------------------------------------------
// Globals to track state

static bool is_mounted = false;
static bool exit_program = false;

// ----------------------------------------------------------------
// “Back” helper: returns from the current menu_run()

void back_callback(Menu* menu) {
    menu_back(menu);
}

// ----------------------------------------------------------------
// Quit helper: marks for exit and returns from the current menu_run()

void quit_callback(Menu* menu) {
    exit_program = true;
    menu_back(menu);
}

// ----------------------------------------------------------------
// Mount / Unmount callbacks

void mount_callback(Menu* menu) {
    if (is_mounted) {
        printf("Imagem já está montada.\n");
    } else {
        // Aqui você chamaria sua rotina real de montagem...
        is_mounted = true;
        printf("Imagem montada com sucesso.\n");
    }
    printf("\nPressione qualquer tecla para continuar...");
    getchar();
    menu_back(menu);
}

void unmount_callback(Menu* menu) {
    if (!is_mounted) {
        printf("Nenhuma imagem montada.\n");
    } else {
        // Aqui você chamaria sua rotina real de desmontagem...
        is_mounted = false;
        printf("Imagem desmontada com sucesso.\n");
    }
    printf("\nPressione qualquer tecla para continuar...");
    getchar();
    menu_back(menu);
}

// ----------------------------------------------------------------
// Your existing “Copy File” flow setup
// (unchanged from o seu código original)

typedef struct {
    int copy_type;  // 0 = Disco -> Sistema, 1 = Sistema -> Disco
    char* origin_path;
    char* output_path;
} CopyContext;

void handle_move_type(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    ctx->copy_type = menu->selected_index;
    flow_next_step(flow);
}

void handle_origin_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    free(ctx->origin_path);
    ctx->origin_path = strdup(input);
    flow_next_step(flow);
}

void handle_output_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    free(ctx->output_path);
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

void handle_flow_back(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_previous_step(flow);
}

void copy_complete(MenuFlow* flow) {
    CopyContext* ctx = (CopyContext*)flow->data;
    printf("\nOperação completa:\n");
    printf("  Tipo: %s\n", (ctx->copy_type ? "Disco -> Sistema" : "Sistema -> Disco"));
    printf("  Origem: %s\n", ctx->origin_path);
    printf("  Destino: %s\n", ctx->output_path);
    free(ctx->origin_path);
    free(ctx->output_path);
    ctx->origin_path = ctx->output_path = NULL;
    ctx->copy_type = -1;
}

void copy_cancel(MenuFlow* flow) {
    CopyContext* ctx = (CopyContext*)flow->data;
    printf("\nOperação cancelada.\n");
    free(ctx->origin_path);
    free(ctx->output_path);
    ctx->origin_path = ctx->output_path = NULL;
    ctx->copy_type = -1;
}

void handle_option(Menu* menu) {
    printf("Option selected: %s\n", menu->items[menu->selected_index].title);
}

void setup_copy_flow(Menu* parent_menu) {
    CopyContext* ctx = malloc(sizeof(CopyContext));
    ctx->copy_type = -1;
    ctx->origin_path = ctx->output_path = NULL;

    MenuFlow* flow = flow_create("Copiar Arquivo", copy_complete, copy_cancel);
    flow->data = ctx;

    // Step 1
    Menu* step1 = menu_create("Selecione o tipo de operação", NULL);
    menu_add_item(step1, "Disco -> Sistema", handle_move_type);
    menu_add_item(step1, "Sistema -> Disco", handle_move_type);
    menu_add_item(step1, "Voltar", handle_cancel);

    // Step 2
    Menu* step2 = menu_create("Insira o caminho de origem", NULL);
    menu_add_input(step2, "Caminho: ", handle_origin_path);
    menu_add_item(step2, "Voltar", handle_flow_back);

    // Step 3
    Menu* step3 = menu_create("Insira o caminho de destino", NULL);
    menu_add_input(step3, "Caminho: ", handle_output_path);
    menu_add_item(step3, "Voltar", handle_flow_back);

    // Step 4
    Menu* step4 = menu_create("Deseja continuar?", NULL);
    menu_add_item(step4, "Confirmar", handle_confirm);
    menu_add_item(step4, "Cancelar", handle_cancel);

    flow_add_step(flow, step1);
    flow_add_step(flow, step2);
    flow_add_step(flow, step3);
    flow_add_step(flow, step4);

    menu_add_flow(parent_menu, "Copiar Arquivo", flow);
}

// ----------------------------------------------------------------
// main()

int main(void) {
    // Pre‐build both menus:

    // 1) Menu de “não montado”
    Menu* unmounted_menu = menu_create("MENU PRINCIPAL", NULL);
    menu_add_item(unmounted_menu, "Montar Imagem", mount_callback);
    menu_add_item(unmounted_menu, "Quit", quit_callback);

    // 2) Menu de “já montado”
    Menu* mounted_menu = menu_create("DISK OPERATIONS", NULL);
    menu_add_item(mounted_menu, "ls-1 (Listar diretório raiz)", handle_option);
    menu_add_item(mounted_menu, "ls   (Listar todos arquivos e diretórios)", handle_option);
    setup_copy_flow(mounted_menu);
    menu_add_item(mounted_menu, "rm   (Remover arquivo ou diretório)", handle_option);
    menu_add_item(mounted_menu, "Desmontar Imagem", unmount_callback);
    menu_add_item(mounted_menu, "Quit", quit_callback);

    // Loop, alternando entre os dois menus até o usuário escolher “Quit”
    while (!exit_program) {
        printf("\n%s\n", is_mounted ? "DISK OPERATIONS" : "MENU PRINCIPAL");
        if (!is_mounted) {
            menu_run(unmounted_menu);
        } else {
            menu_run(mounted_menu);
        }
    }

    // ── Cleanup ──
    // Free CopyContext inside the mounted flow:
    for (int i = 0; i < arrlen(mounted_menu->items); i++) {
        if (mounted_menu->items[i].type == MENU_ITEM_FLOW) {
            MenuFlow* flow = (MenuFlow*)mounted_menu->items[i].flow;
            CopyContext* ctx = (CopyContext*)flow->data;
            if (ctx) free(ctx);
            for (int s = 0; s < arrlen(flow->steps); s++) {
                menu_free(flow->steps[s]);
            }
            flow_free(flow);
            break;
        }
    }

    // Free both menus
    menu_free(unmounted_menu);
    menu_free(mounted_menu);

    return 0;
}
