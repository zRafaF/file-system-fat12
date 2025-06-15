#include "app.h"

#include <time.h>

#include "stb_ds.h"

static FILE *disk = NULL;

bool app_is_mounted(void) { return disk != NULL; }

void app_mount_callback(Menu *m) {
    if (app_is_mounted()) {
        printf("Imagem ja esta montada.\n");
    } else {
        switch (m->selected_index) {
            case 0:
                disk = fopen(PATH_FAT12_IMG, "r+b");
                if (disk == NULL) {
                    perror("Failed to open disk image");
                    exit(EXIT_FAILURE);
                }
                fat12_load_full_fat_table(disk);
                printf("Imagem montada com sucesso em \'/\'.\n");
                break;
            case 1:
                disk = fopen(PATH_FAT12SUBDIR_IMG, "r+b");
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

    fs_directory_tree_node_t *disk_tree = fs_create_disk_tree(disk);
    if (disk_tree == NULL) {
        printf("Erro ao criar a arvore de diretorios do disco.\n");
        return;
    }

    fs_directory_tree_node_t *target_node = fs_get_node_by_path(disk_tree, input);
    if (target_node == NULL) {
        printf("Caminho '%s' nao encontrado no disco.\n", input);
        fs_free_disk_tree(disk_tree);
        return;
    }

    if (!fs_remove_file_or_directory(disk, target_node)) {
        fprintf(stderr, "Erro ao remover o arquivo ou diretorio '%s'.\n", input);
        fs_free_disk_tree(disk_tree);
        return;
    }
    printf("Arquivo ou diretorio '%s' removido com sucesso.\n", input);
    fs_free_disk_tree(disk_tree);
}

bool _app_copy_sys_to_disk(const char *src, const char *dst) {
    printf("Copiando do sistema para o disco...\n");
    fs_directory_tree_node_t *disk_tree = fs_create_disk_tree(disk);
    if (disk_tree == NULL) {
        printf("Erro ao criar a arvore de diretorios do disco.\n");
        return false;
    }

    fs_directory_tree_node_t *target_node = fs_get_node_by_path(disk_tree, src);
    if (target_node == NULL) {
        printf("Caminho '%s' nao encontrado no disco.\n", src);
        fs_free_disk_tree(disk_tree);
        return false;
    }

    FILE *target_file = fopen(dst, "w");
    if (target_file == NULL) {
        perror("Erro ao abrir o arquivo de destino");
        return false;
    }

    uint16_t *cluster_list = NULL;
    if (!fat12_get_table_entry_chain(target_node->metadata.first_cluster, &cluster_list)) {
        fprintf(stderr, "Nao foi possivel encontrar a lista de clusters de %s\n", target_node->metadata.filename);
        arrfree(cluster_list);
        return false;
    }

    uint32_t remaining_size = target_node->metadata.file_size;
    for (int i = 0; i < arrlen(cluster_list); i++) {
        printf("Escrevendo %i/%llu...\n", i + 1, arrlen(cluster_list));

        uint8_t buffer[SECTOR_SIZE];
        if (!fat12_read_data_sector(disk, buffer, cluster_list[i])) {
            fprintf(stderr, "Erro ao ler o setor de dados do cluster %d\n", cluster_list[i]);
            arrfree(cluster_list);
            fs_free_disk_tree(disk_tree);
            return false;
        }

        size_t to_write = (remaining_size > SECTOR_SIZE) ? SECTOR_SIZE : remaining_size;
        fwrite(buffer, 1, to_write, target_file);
        remaining_size -= to_write;
        if (remaining_size == 0) break;  // no more data to write
    }

    fclose(target_file);
    fs_free_disk_tree(disk_tree);
    arrfree(cluster_list);
    fflush(disk);
    return true;
}

bool _app_copy_disk_to_sys(const char *src, const char *dst) {
    printf("Copiando do disco para o sistema...\n");

    fs_fat_compatible_filename_t filename = fs_get_filename_from_path(dst);
    if (filename.file[0] == 0) {
        fprintf(stderr, "Nome de arquivo invalido: '%.*s'\n", FAT12_FILE_NAME_LENGTH, src);
        return false;
    }

    fs_directory_tree_node_t *disk_tree = fs_create_disk_tree(disk);
    if (disk_tree == NULL) {
        printf("Erro ao criar a arvore de diretorios do disco.\n");
        return false;
    }

    fs_directory_tree_node_t *target_node = fs_get_directory_node_by_path(disk_tree, dst);
    if (target_node == NULL) {
        printf("Caminho '%s' nao encontrado no disco.\n", dst);
        fs_free_disk_tree(disk_tree);
        return false;
    }

    printf("Escrevendo no diretorio: \'%s\'\n", target_node->metadata.filename);
    printf("Profundidade do diretorio: %u\n\n", target_node->depth);

    FILE *source_file = fopen(src, "r");
    if (source_file == NULL) {
        perror("Erro ao abrir o arquivo de origem");
        return false;
    }

    uint16_t *cluster_list = NULL;

    uint32_t file_size = fs_write_file_to_data_area(source_file, disk, &cluster_list);

    if (!file_size) {
        fprintf(stderr, "Erro ao escrever o arquivo na area de dados.\n");
        fclose(source_file);
        arrfree(cluster_list);
        return false;
    }

    if (!fs_write_cluster_chain_to_fat_table(disk, cluster_list)) {
        fprintf(stderr, "Erro ao escrever a cadeia de clusters na tabela FAT.\n");
        fclose(source_file);
        arrfree(cluster_list);
        return false;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fat12_time_s current_time = {.seconds = tm.tm_sec, .minutes = tm.tm_min, .hours = tm.tm_hour};
    fat12_date_s current_date = {.day = tm.tm_mday, .month = tm.tm_mon + 1, .year = tm.tm_year + 1900};

    printf("Name: %.*s\n", FAT12_FILE_NAME_LENGTH, filename.file);
    printf("Extension: %s\n", filename.extension);

    fat12_file_subdir_s file_entry = fat12_format_file_entry(
        filename.file,                 // File name
        filename.extension,            // extension
        FAT12_ATTR_NONE,               // 0x00 - No attributes
        f12h_pack_time(current_time),  // Creation time
        f12h_pack_date(current_date),  // Creation date
        f12h_pack_date(current_date),  // Last access date
        f12h_pack_time(current_time),  // Last write time
        f12h_pack_date(current_date),  // Last write date
        cluster_list[0],               // First cluster from the list
        file_size                      // File size
    );

    if (!fs_add_file_to_directory(disk, target_node, file_entry)) {
        fprintf(stderr, "Erro ao adicionar o arquivo ao diretorio.\n");
        fclose(source_file);
        arrfree(cluster_list);
        fs_free_disk_tree(disk_tree);
        return false;
    }

    fclose(source_file);
    arrfree(cluster_list);
    fs_free_disk_tree(disk_tree);
    fflush(disk);  // Ensure all changes are written to the disk image
    return true;
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
            if (_app_copy_sys_to_disk(src, dst))
                printf("Arquivo copiado com sucesso para o disco.\n");
            else
                printf("Erro ao copiar o arquivo para o disco.\n");
            break;
        case 1:
            if (_app_copy_disk_to_sys(src, dst))
                printf("Arquivo copiado com sucesso do disco.\n");
            else
                printf("Erro ao copiar o arquivo do disco.\n");
            break;
        default:
            printf("Erro: Tipo de copia desconhecido.\n");
            break;
    }
    fflush(disk);
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
    // app_copy_complete(0, "/ARQ.TXT", "./teste.txt");
    app_copy_complete(1, "teste.txt", "/SUBDIR/t.txt");
}
#endif

// Copies and replace the images at /imgs/backup to /imgs/
void app_quick_actions_reset_images_callback(Menu *m) {
    UNUSED(m);
    printf("Recuperando imagens para o estado original...\n");

#ifdef _WIN32
    // Ensure destination directory exists
    system("if not exist \"imgs\\\" mkdir imgs");
    // Copy with overwrite
    if (system("copy /Y \"imgs\\backups\\*.img\" \"imgs\\\"") != 0) {
        perror("Erro ao restaurar imagens no Windows");
        return;
    }
#else
    // Ensure destination directory exists
    system("mkdir -p imgs");
    // Copy with overwrite
    if (system("cp -f imgs/backups/*.img imgs/") != 0) {
        perror("Erro ao restaurar imagens no Linux");
        return;
    }
#endif

    printf("Imagens restauradas com sucesso.\n");
    exit(0);  // Exit the program after restoring images
}

void app_quick_actions_copy_to_disk_callback(Menu *m) {
    UNUSED(m);
    printf("Copiando /AQR.TXT do sistema para o disco\n");
    app_copy_complete(0, "/ARQ.TXT", "./ARQ.TXT");
}
void app_quick_actions_copy_to_system_callback(Menu *m) {
    UNUSED(m);
    printf("Copiando shrek.md para o sistema em /SUBDIR/script.md\n");
    printf("\nFalha na imagem sem subdiretorios\n");
    app_copy_complete(1, "shrek.md", "/SUBDIR/script.txt");
}

void app_quick_actions_list_fat12_table_callback(Menu *m) {
    UNUSED(m);
    printf("Listando 100 itens da tabela FAT12:\n");

    for (int i = 0; i < 100; i++) {
        uint16_t entry = fat12_get_table_entry(i);
        printf("FAT entry %d: %x\n", i, entry);
    }
}