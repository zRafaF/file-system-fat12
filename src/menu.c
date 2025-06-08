#include "menu.h"

#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "stb_ds.h"

static bool exit_program = false;

static void quit_callback(Menu* menu) {
    exit_program = true;
    menu_back(menu);
}

static void handle_move_type(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    ctx->copy_type = menu->selected_index;
    flow_next_step(flow);
}

static void handle_origin_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    free(ctx->origin_path);
    ctx->origin_path = strdup(input);
    flow_next_step(flow);
}

static void handle_output_path(Menu* menu, const char* input) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    CopyContext* ctx = (CopyContext*)flow->data;
    free(ctx->output_path);
    ctx->output_path = strdup(input);
    flow_next_step(flow);
}

static void handle_confirm(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_complete(flow);
}

static void handle_cancel(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_cancel(flow);
}

static void handle_flow_back(Menu* menu) {
    MenuFlow* flow = (MenuFlow*)menu->user_data;
    flow_previous_step(flow);
}

static void copy_complete(MenuFlow* flow) {
    CopyContext* ctx = (CopyContext*)flow->data;
    app_copy_complete(ctx->copy_type, ctx->origin_path, ctx->output_path);

    free(ctx->origin_path);
    free(ctx->output_path);
    ctx->origin_path = ctx->output_path = NULL;
    ctx->copy_type = -1;
}

static void copy_cancel(MenuFlow* flow) {
    CopyContext* ctx = (CopyContext*)flow->data;
    printf("\nOperacao cancelada.\n");
    free(ctx->origin_path);
    free(ctx->output_path);
    ctx->origin_path = ctx->output_path = NULL;
    ctx->copy_type = -1;
}

static void setup_copy_flow(Menu* parent_menu) {
    CopyContext* ctx = malloc(sizeof(CopyContext));
    ctx->copy_type = -1;
    ctx->origin_path = ctx->output_path = NULL;

    MenuFlow* flow = flow_create("Copiar Arquivo", copy_complete, copy_cancel);
    flow->data = ctx;

    // Step 1
    Menu* step1 = menu_create("Selecione o tipo de operacao", NULL);
    menu_add_item(step1, "Disco -> Sistema", handle_move_type);
    menu_add_item(step1, "Sistema -> Disco", handle_move_type);
    menu_add_item(step1, "Voltar", handle_cancel);

    // Step 2
    Menu* step2 = menu_create("Insira o caminho de origem", NULL);
    menu_add_input(step2, "Caminho Origem: ", handle_origin_path);
    menu_add_item(step2, "Voltar", handle_flow_back);

    // Step 3
    Menu* step3 = menu_create("Insira o caminho de destino", NULL);
    menu_add_input(step3, "Caminho Destino: ", handle_output_path);
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

void init_menus(Menu* unmounted_menu, Menu* mounted_menu) {
    // Unmounted menu setup
    menu_add_item(unmounted_menu, "Montar \"fat12.img\"", app_mount_callback);
    menu_add_item(unmounted_menu, "Montar \"fat12subdir.img\"", app_mount_callback);
    menu_add_item(unmounted_menu, "Sair", quit_callback);

    // Mounted menu setup
    menu_add_item(mounted_menu, "Debug", app_debug_callback);
    menu_add_item(mounted_menu, "Info Boot Sector", app_boot_sector_callback);
    menu_add_item(mounted_menu, "ls-1 (Listar diretorio raiz)", app_ls1_callback);
    menu_add_item(mounted_menu, "ls   (Listar todos arquivos e diretorios)", app_ls_callback);
    menu_add_input(mounted_menu, "rm   (Remover arquivo ou diretorio) ", app_rm_callback);

    setup_copy_flow(mounted_menu);
    menu_add_item(mounted_menu, "Desmontar Imagem", app_unmount_callback);
    menu_add_item(mounted_menu, "Sair", quit_callback);
}

void free_menus(Menu* unmounted_menu, Menu* mounted_menu) {
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
}

bool should_exit() {
    return exit_program;
}