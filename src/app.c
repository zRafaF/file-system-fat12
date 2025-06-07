#include "app.h"

static FILE *disk = NULL;

bool app_is_mounted(void) { return disk != NULL; }

void app_mount_callback(Menu *m) {
    if (app_is_mounted()) {
        printf("Imagem ja esta montada.\n");
    } else {
        switch (m->selected_index) {
            case 0:
                disk = fopen(PATH_FAT12_IMG, "rb");
                if (disk == NULL) {
                    perror("Failed to open disk image");
                    exit(EXIT_FAILURE);
                }
                printf("Imagem montada com sucesso.\n");
                break;
            case 1:
                disk = fopen(PATH_FAT12SUBDIR_IMG, "rb");
                if (disk == NULL) {
                    perror("Failed to open disk image");
                    exit(EXIT_FAILURE);
                }
                printf("Imagem montada com sucesso.\n");
                break;
            default:
                printf("Opção inválida...\n");
                break;
        }
    }
    menu_wait_for_any_key();
    menu_back(m);  // Permite que o menu seja trocado
}

void app_unmount_callback(Menu *m) {
    if (!app_is_mounted()) {
        printf("Nenhuma imagem montada.\n");
    } else {
        fclose(disk);
        disk = NULL;  // Desmonta a imagem
        printf("Imagem desmontada com sucesso.\n");
    }
    menu_wait_for_any_key();
    menu_back(m);  // Permite que o menu seja trocado
}

void app_boot_sector_callback(Menu *m) {
    UNUSED(m);
    if (!app_is_mounted()) {
        printf("Nenhuma imagem montada.\n");
    } else {
        fat12_print_boot_sector_info(fat12_read_boot_sector(disk));
    }
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
