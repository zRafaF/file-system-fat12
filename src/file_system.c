#include "file_system.h"

#include "stb_ds.h"

// Format the file name from a fat12_file_subdir_s structure, removes trailing spaces and constructs the full name with extension.
static char *fs_get_file_name(fat12_file_subdir_s dir, char *buffer) {
    if (buffer == NULL) {
        return NULL;  // Error: buffer is null
    }

    size_t current_buffer_pos = 0;

    // Copy filename characters, trimming trailing spaces
    for (size_t i = 0; i < sizeof(dir.filename); ++i) {
        if (dir.filename[i] != 0x20) {  // If not a space
            buffer[current_buffer_pos++] = dir.filename[i];
        } else {
            break;
        }
    }

    size_t extension_length = 0;
    for (size_t i = 0; i < sizeof(dir.extension); ++i) {
        if (dir.extension[i] != 0x20) {
            extension_length = i + 1;  // Update length if it's a non-space character
        }
    }

    // Add a dot and the extension if an extension exists
    if (extension_length > 0) {
        buffer[current_buffer_pos++] = '.';
        memcpy(buffer + current_buffer_pos, dir.extension, extension_length);
        current_buffer_pos += extension_length;
    }

    // Null-terminate the full string
    buffer[current_buffer_pos] = '\0';

    return buffer;
}

void fs_print_file_leaf(fat12_file_subdir_s dir, uint8_t depth) {
    for (uint8_t i = 0; i < depth; i++) {
        printf("  ");
    }

    static const size_t max_filename_length = sizeof(dir.filename) + sizeof(dir.extension) + 1;
    char file_name[max_filename_length];
    fs_get_file_name(dir, file_name);

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

    fat12_date_s creation_date = fat12_extract_date(dir.creation_date);
    fat12_time_s creation_time = fat12_extract_time(dir.creation_time);

    printf("%u/%u/%u\t\t", creation_date.day, creation_date.month, creation_date.year);
    printf("%u:%u:%u\n", creation_time.hours, creation_time.minutes, creation_time.seconds);
}

void fs_print_ls_directory_header() {
    printf("Nome\t\tAtributos\tTamanho (bytes)\tData de Criacao\tHora de Criacao\n");
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

void fs_free_directory(fs_directory_t dir) {
    arrfree(dir.files);
    arrfree(dir.subdirs);
}