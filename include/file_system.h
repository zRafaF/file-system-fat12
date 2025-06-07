#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>

#include "fat12.h"

typedef struct {
    fat12_file_subdir_s *files;    // Array of files in the directory
    fat12_file_subdir_s *subdirs;  // Array of subdirectories in the directory
} fs_directory_t;

void fs_print_ls_directory_header();

void fs_print_file_leaf(fat12_file_subdir_s dir, uint8_t depth);

// Reads the root directory of the FAT12 file system and returns a pointer to a fs_directory_t structure
// WARNING: The returned pointer must be freed after use to avoid memory leaks (arrfree()).
fs_directory_t fs_read_root_directory(FILE *disk);

// Frees the memory allocated for a fs_directory_t structure
void fs_free_directory(fs_directory_t dir);

#endif  // FILE_SYSTEM_H