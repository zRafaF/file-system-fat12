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

    printf("----------------------------------------------------------------------------------------------------------------\n");

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
    printf("\n=======  REALIZANDO COPIA DE ARQUIVOS  =======\n");
    printf("----------------------------------------------\n");
    printf("- Tipo: %s\n", (copy_type ? "Disco -> Sistema" : "Sistema -> Disco"));
    printf("- Origem: %s\n", src);
    printf("- Destino: %s\n", dst);
    printf("----------------------------------------------\n");

    switch (copy_type) {
        case 0:
            printf("Copiando do sistema de arquivos para o disco.\n");

            fs_directory_tree_node_t *disk_tree = fs_create_disk_tree(disk);
            if (disk_tree == NULL) {
                printf("Erro ao criar a arvore de diretorios do disco.\n");
                menu_wait_for_any_key();
                return;
            }

            fs_directory_tree_node_t *target_node = fs_get_node_by_path(disk_tree, src);
            if (target_node == NULL) {
                printf("Caminho '%s' nao encontrado no disco.\n", src);
                fs_free_disk_tree(disk_tree);
                menu_wait_for_any_key();
                return;
            }

            FILE *target_file = fopen(dst, "w");
            if (target_file == NULL) {
                perror("Erro ao abrir o arquivo de destino");
                menu_wait_for_any_key();
                return;
            }

            uint16_t *cluster_list = NULL;
            if (!fat12_get_table_entry_chain(target_node->metadata.first_cluster, &cluster_list)) {
                fprintf(stderr, "Nao foi possivel encontrar a lista de clusters de %s\n", target_node->metadata.filename);
                arrfree(cluster_list);
                menu_wait_for_any_key();
                return;
            }

            uint32_t remaining_size = target_node->metadata.file_size;
            for (int i = 0; i < arrlen(cluster_list); i++) {
                printf("Escrevendo %i/%lu...\n", i + 1, arrlen(cluster_list));

                uint8_t buffer[512];
                if (!fat12_read_data_sector(disk, buffer, cluster_list[i])) {
                    fprintf(stderr, "Erro ao ler o setor de dados do cluster %d\n", cluster_list[i]);
                    arrfree(cluster_list);
                    fs_free_disk_tree(disk_tree);
                    menu_wait_for_any_key();
                    return;
                }

                size_t to_write = (remaining_size > 512) ? 512 : remaining_size;
                fwrite(buffer, 1, to_write, target_file);
                remaining_size -= to_write;
                if (remaining_size == 0) break;  // no more data to write
            }

            fclose(target_file);
            fs_free_disk_tree(disk_tree);
            arrfree(cluster_list);

            printf("Arquivo copiado com sucesso para o disco.\n");
            break;
        case 1:
            printf("Copiado do disco para o sistema de arquivos.\n");
            break;
        default:
            printf("Tipo de copia desconhecido.\n");
            break;
    }

    menu_wait_for_any_key();
}

#ifdef DEBUG
void app_debug1_callback(Menu *m) {
    UNUSED(m);
    for (int i = 0; i < 20; i++) {
        uint16_t entry = fat12_get_table_entry(i);
        printf("FAT entry %d: %x\n", i, entry);
    }
}

void app_debug2_callback(Menu *m) {
    UNUSED(m);
    app_copy_complete(0, "/ARQ.TXT", "./teste.txt");
}
#endif
