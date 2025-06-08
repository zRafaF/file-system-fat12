#include "app.h"

#include "stb_ds.h"

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
                fat12_load_full_fat_table(disk);
                printf("Imagem montada com sucesso em \'/\'.\n");
                break;
            case 1:
                disk = fopen(PATH_FAT12SUBDIR_IMG, "rb");
                if (disk == NULL) {
                    perror("Failed to open disk image");
                    exit(EXIT_FAILURE);
                }
                fat12_load_full_fat_table(disk);
                printf("Imagem montada com sucesso em \'/\'.\n");
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

    fs_directory_t root_dir = fs_read_root_directory(disk);

    printf("\n=======  LISTANDO DIRETORIO RAIZ  =======\n");
    printf("-----------------------------------------\n");

    fs_print_ls_directory_header();

    printf("----------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < arrlen(root_dir.subdirs); i++) {
        fat12_file_subdir_s subdir = root_dir.subdirs[i];
        fs_print_file_leaf(subdir, 0);
    }

    for (int i = 0; i < arrlen(root_dir.files); i++) {
        fat12_file_subdir_s file = root_dir.files[i];
        fs_print_file_leaf(file, 0);
    }

    fs_free_directory(root_dir);
}

// List all files and directories
void app_ls_callback(Menu *m) {
    UNUSED(m);
    fs_directory_tree_node_t *disk_tree = fs_create_disk_tree(disk);
    printf("\n=======  LISTANDO ARVORE DE DIRETORIOS  =======\n");
    fs_print_directory_tree(disk_tree);
    fs_free_disk_tree(disk_tree);
}

void app_rm_callback(Menu *m, const char *input) {
    UNUSED(m);
    printf("Removendo arquivo ou diretorio: %s\n", input);
}

void app_copy_complete(int copy_type, const char *src, const char *dst) {
    printf("\nOperacao completa:\n");
    printf("  Tipo: %s\n", (copy_type ? "Disco -> Sistema" : "Sistema -> Disco"));
    printf("  Origem: %s\n", src);
    printf("  Destino: %s\n", dst);
    menu_wait_for_any_key();
}

#ifdef DEBUG
void app_debug1_callback(Menu *m) {
    UNUSED(m);

    fs_directory_t sub_dir = fs_read_directory(disk, 6);

    printf("Subdirectory contents:\n");
    fs_print_ls_directory_header();
    printf("----------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < arrlen(sub_dir.subdirs); i++) {
        fat12_file_subdir_s subdir = sub_dir.subdirs[i];
        fs_print_file_leaf(subdir, 0);
        fat12_print_directory_info(subdir);
    }

    for (int i = 0; i < arrlen(sub_dir.files); i++) {
        fat12_file_subdir_s file = sub_dir.files[i];
        fs_print_file_leaf(file, 0);
    }

    fs_free_directory(sub_dir);
}

void app_debug2_callback(Menu *m) {
    UNUSED(m);

    // Testing tree memory leaks
    // fs_directory_tree_node_t **trees = NULL;

    // for (size_t i = 0; i < 20000; i++) {
    //     fs_directory_tree_node_t *tree = fs_create_disk_tree(disk);
    //     arrpush(trees, tree);

    //     if (i % 1000 == 0) {
    //         printf("Criada arvore de diretorios %llu\n", i);
    //     }
    // }

    // printf("Criados %llu arvores de diretorios.\n", arrlen(trees));
    // getchar();  // Wait for user input before freeing

    // for (size_t i = 0; i < arrlen(trees); i++) {
    //     fs_free_disk_tree(trees[i]);
    // }

    // arrfree(trees);

    // printf("Todas as arvores de diretorios foram liberadas.\n");

    // fat12_file_subdir_s *dir_entries = NULL;

    // for (uint8_t i = 0; i < 12; i++) {
    //     fat12_file_subdir_s dir_entry = fat12_read_directory_entry(disk, i);

    //     if (dir_entry.filename[0] != 0x00) {
    //         arrpush(dir_entries, dir_entry);
    //     }
    // }

    // for (int i = 0; i < arrlen(dir_entries); i++) {
    //     fat12_file_subdir_s dir_entry = dir_entries[i];
    //     fat12_print_directory_info(dir_entry);
    // }

    // arrfree(dir_entries);

    // printf("Listando todos os arquivos e diretorios...\n");
}
#endif
