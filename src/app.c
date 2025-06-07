#include "app.h"

static bool is_mounted = false;

bool app_is_mounted(void) { return is_mounted; }

void app_mount_callback(Menu *m) {
    if (is_mounted) {
        printf("Imagem ja esta montada.\n");
    } else {
        switch (m->selected_index) {
            case 0:
                printf("Montando \"fat12.img\"...\n");
                break;
            case 1:
                printf("Montando \"fat12subdir.img\"...\n");
                break;
            default:
                printf("Montando fat12.img...\n");
                break;
        }
        is_mounted = true;
        printf("Imagem montada com sucesso.\n");
    }
    menu_wait_for_any_key();
    menu_back(m);  // Permite que o menu seja trocado
}

void app_unmount_callback(Menu *m) {
    if (!is_mounted) {
        printf("Nenhuma imagem montada.\n");
    } else {
        is_mounted = false;
        printf("Imagem desmontada com sucesso.\n");
    }
    menu_wait_for_any_key();
    menu_back(m);  // Permite que o menu seja trocado
}

void app_ls1_callback(Menu *m) {
    UNUSED(m);
    printf("Listando arquivos no diretorio raiz...\n");
}
void app_ls_callback(Menu *m) {
    UNUSED(m);
    printf("Listando todos os arquivos e diretorios...\n");
}
void app_rm_callback(Menu *m) {
    UNUSED(m);
    printf("Removendo arquivo ou diretorio...\n");
}

void app_copy_complete(int copy_type, const char *src, const char *dst) {
    printf("\nOperacao completa:\n");
    printf("  Tipo: %s\n", (copy_type ? "Disco -> Sistema" : "Sistema -> Disco"));
    printf("  Origem: %s\n", src);
    printf("  Destino: %s\n", dst);
    menu_wait_for_any_key();
}
