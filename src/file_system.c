#include "file_system.h"

#include "stb_ds.h"

void fs_print_file_leaf(fat12_file_subdir_s dir, uint8_t depth) {
    for (uint8_t i = 0; i < depth; i++) {
        printf("  ");
    }

    char file_name[FS_MAX_FILENAME_LENGTH];
    f12h_format_filename(dir, file_name);

    printf("%s", file_name);
    if (strlen(file_name) < 8) {
        printf("\t\t");  // Add tab if filename is short
    } else {
        printf("\t");  // Add single tab otherwise
    }

    printf("%s", fat12_attribute_to_string(dir.attributes));
    if (strlen(fat12_attribute_to_string(dir.attributes)) < 8) {
        printf("\t\t");  // Add tab if attributes string is short
    } else {
        printf("\t");  // Add single tab otherwise
    }

    printf("%u\t\t", dir.file_size);
    fat12_date_s edit_date = fat12_extract_date(dir.last_write_date);
    fat12_time_s edit_time = fat12_extract_time(dir.last_write_time);

    char formatted_edit_time[F12H_FORMATTED_DATE_TIME_BUFFER_SIZE];

    f12h_format_date_time(edit_date, edit_time, formatted_edit_time);
    printf("%s\t", formatted_edit_time);

    fat12_date_s creation_date = fat12_extract_date(dir.creation_date);
    fat12_time_s creation_time = fat12_extract_time(dir.creation_time);

    char formatted_date_time[F12H_FORMATTED_DATE_TIME_BUFFER_SIZE];

    f12h_format_date_time(creation_date, creation_time, formatted_date_time);

    printf("%s\t", formatted_date_time);

    printf("%u\n", dir.first_cluster);
}

void fs_print_ls_directory_header() {
    printf("Nome\t\tAtributo\tTamanho (bytes)\tData de Modificacao\tData de Criacao\t\tPrimeiro Cluster\n");
}

fs_directory_t fs_read_root_directory(FILE *disk) {
    fat12_file_subdir_s *dir_entries = NULL;

    for (uint8_t i = 0; i < FAT12_ROOT_DIRECTORY_ENTRIES; i++) {
        fat12_file_subdir_s dir_entry = fat12_read_directory_entry(disk, i);

        if (dir_entry.filename[0] != 0x00) {
            arrpush(dir_entries, dir_entry);
        }
    }

    fat12_file_subdir_s *files = NULL;
    fat12_file_subdir_s *subdirs = NULL;

    for (int i = 0; i < arrlen(dir_entries); i++) {
        if (dir_entries[i].attributes & FAT12_ATTR_DIRECTORY) {
            arrpush(subdirs, dir_entries[i]);
        } else if (dir_entries[i].extension[0] != 0x00 || dir_entries[i].filename[0] != 0x00) {
            arrpush(files, dir_entries[i]);
        }
    }

    arrfree(dir_entries);
    fs_directory_t dir = {.files = files, .subdirs = subdirs};
    return dir;
}

fs_directory_t fs_read_directory(FILE *disk, uint16_t cluster) {
    fat12_file_subdir_s *dir_entries = NULL;

    for (uint8_t i = 0; i < FAT12_DIRECTORY_ENTRIES_PER_SECTOR; i++) {
        fat12_file_subdir_s dir_entry = fat12_read_directory_from_data_area(disk, cluster, i);

        if (dir_entry.filename[0] != 0x00) {
            arrpush(dir_entries, dir_entry);
        }
    }

    fat12_file_subdir_s *files = NULL;
    fat12_file_subdir_s *subdirs = NULL;

    for (int i = 0; i < arrlen(dir_entries); i++) {
        if (dir_entries[i].attributes == FAT12_ATTR_DIRECTORY) {
            arrpush(subdirs, dir_entries[i]);
        } else if (dir_entries[i].extension[0] != 0x00 || dir_entries[i].filename[0] != 0x00) {
            arrpush(files, dir_entries[i]);
        }
    }

    arrfree(dir_entries);
    fs_directory_t dir = {.files = files, .subdirs = subdirs};
    return dir;
}

static void _fs_recursive_create_subdirs_tree(FILE *disk, fs_directory_tree_node_t *dir) {
    if (dir->depth >= FS_MAX_DIRECTORY_DEPTH) {
        fprintf(stderr, "Maximum directory depth reached: %llu\n", dir->depth);
        return;  // Prevent infinite recursion
    }

    if (dir->metadata.first_cluster < FAT12_DATA_AREA_NUMBER_OFFSET ||
        dir->metadata.first_cluster == dir->parent->metadata.first_cluster) {
        return;  // Skip empty or cyclic entries
    }

    uint16_t *cluster_list = NULL;

    if (!fat12_get_table_entry_chain(dir->metadata.first_cluster, &cluster_list)) {
        fprintf(stderr, "Failed to get cluster chain for %s\n", dir->metadata.filename);
        arrfree(cluster_list);
        return;
    }

    for (int i = 0; i < arrlen(cluster_list); i++) {
        fs_directory_t listing = fs_read_directory(disk, cluster_list[i]);

        // subdirectories
        for (int i = 0; i < arrlen(listing.subdirs); i++) {
            fs_directory_tree_node_t *subdir_node = malloc(sizeof(*subdir_node));
            if (!subdir_node) {
                perror("malloc subdir node");
                fs_free_directory(listing);
                arrfree(cluster_list);
                exit(EXIT_FAILURE);
            }
            subdir_node->parent = dir;
            subdir_node->children = NULL;
            subdir_node->type = FS_DIRECTORY_TYPE_SUBDIR;
            subdir_node->metadata = listing.subdirs[i];
            subdir_node->depth = dir->depth + 1;

            // recurse
            _fs_recursive_create_subdirs_tree(disk, subdir_node);

            arrpush(dir->children, subdir_node);
        }

        // files
        for (int i = 0; i < arrlen(listing.files); i++) {
            fs_directory_tree_node_t *file_node = malloc(sizeof(*file_node));
            if (!file_node) {
                perror("malloc file node");
                fs_free_directory(listing);
                arrfree(cluster_list);
                exit(EXIT_FAILURE);
            }
            file_node->parent = dir;
            file_node->children = NULL;
            file_node->type = FS_DIRECTORY_TYPE_FILE;
            file_node->metadata = listing.files[i];
            file_node->depth = dir->depth + 1;

            arrpush(dir->children, file_node);
        }

        fs_free_directory(listing);
    }

    arrfree(cluster_list);
}

fs_directory_tree_node_t *fs_create_disk_tree(FILE *disk) {
    // allocate the root node on the heap
    fs_directory_tree_node_t *root = malloc(sizeof(*root));
    if (!root) {
        perror("malloc root node");
        exit(EXIT_FAILURE);
    }

    // init root
    root->parent = NULL;
    root->children = NULL;
    root->type = FS_DIRECTORY_TYPE_SUBDIR;
    memset(&root->metadata, 0, sizeof(root->metadata));
    root->metadata.filename[0] = '/';  // Root directory name
    root->depth = 0;

    // read the very first (root) directory entries
    fs_directory_t root_dir = fs_read_root_directory(disk);

    // add subdirectories in root
    for (int i = 0; i < arrlen(root_dir.subdirs); i++) {
        fs_directory_tree_node_t *subdir_node = malloc(sizeof(*subdir_node));
        if (!subdir_node) {
            perror("malloc subdir node");
            fs_free_directory(root_dir);
            exit(EXIT_FAILURE);
        }
        subdir_node->parent = root;
        subdir_node->children = NULL;
        subdir_node->type = FS_DIRECTORY_TYPE_SUBDIR;
        subdir_node->metadata = root_dir.subdirs[i];
        subdir_node->depth = 1;

        // recurse into it
        _fs_recursive_create_subdirs_tree(disk, subdir_node);

        arrpush(root->children, subdir_node);
    }

    // add files in root
    for (int i = 0; i < arrlen(root_dir.files); i++) {
        fs_directory_tree_node_t *file_node = malloc(sizeof(*file_node));
        if (!file_node) {
            perror("malloc file node");
            fs_free_directory(root_dir);
            exit(EXIT_FAILURE);
        }
        file_node->parent = root;
        file_node->children = NULL;
        file_node->type = FS_DIRECTORY_TYPE_FILE;
        file_node->metadata = root_dir.files[i];
        file_node->depth = 1;
        arrpush(root->children, file_node);
    }

    fs_free_directory(root_dir);
    return root;
}

static void _fs_print_tree_ascii(
    fs_directory_tree_node_t *node,
    const char *prefix,
    bool is_last) {
    char name[FS_MAX_FILENAME_LENGTH];
    f12h_format_filename(node->metadata, name);

    // Use ASCII-friendly tree characters
    printf("%s", prefix);
    printf("%s", is_last ? "`-- " : "|-- ");
    printf("%s%s\n", name,
           node->type == FS_DIRECTORY_TYPE_SUBDIR ? "/" : "");

    // Build new prefix
    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s",
             prefix,
             is_last ? "    " : "|   ");

    size_t count = arrlen(node->children);
    for (size_t i = 0; i < count; i++) {
        bool last = (i + 1 == count);
        _fs_print_tree_ascii(node->children[i], new_prefix, last);
    }
}

void fs_print_directory_tree(fs_directory_tree_node_t *root) {
    if (!root) return;

    printf("/\n");

    size_t count = arrlen(root->children);
    for (size_t i = 0; i < count; i++) {
        bool last = (i + 1 == count);
        _fs_print_tree_ascii(root->children[i], "", last);
    }
}

void fs_free_directory(fs_directory_t dir) {
    arrfree(dir.files);
    arrfree(dir.subdirs);
}

void fs_free_disk_tree(fs_directory_tree_node_t *dir_tree) {
    if (!dir_tree) return;

    // Recursively free all children
    for (int i = 0, n = arrlen(dir_tree->children); i < n; i++) {
        fs_free_disk_tree(dir_tree->children[i]);
    }

    // Free the dynamic array of children pointers then self
    arrfree(dir_tree->children);
    free(dir_tree);
}
