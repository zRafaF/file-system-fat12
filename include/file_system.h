#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include <string.h>

#include "fat12.h"
#include "fat12_helpers.h"

#define FS_MAX_DIRECTORY_DEPTH 32                                                          // Maximum depth of the directory tree
#define FS_MAX_FILENAME_LENGTH (FAT12_FILE_NAME_LENGTH + FAT12_FILE_EXTENSION_LENGTH + 1)  // Maximum length of a file name

typedef struct {
    fat12_file_subdir_s *files;    // Array of files in the directory
    fat12_file_subdir_s *subdirs;  // Array of subdirectories in the directory
} fs_directory_t;

typedef enum {
    FS_DIRECTORY_TYPE_FILE,    // Represents a file
    FS_DIRECTORY_TYPE_SUBDIR,  // Represents a subdirectory
} fs_directory_type_e;

struct fs_directory_tree_node;

typedef struct fs_directory_tree_node {
    struct fs_directory_tree_node *parent;     // Pointer to the parent node in the directory tree
    struct fs_directory_tree_node **children;  // Array of subdirectory nodes (using stb_ds dynamic arrays)
    fs_directory_type_e type;                  // Type of the directory (file or subdirectory)
    fat12_file_subdir_s metadata;              // Directory or File information
    size_t depth;                              // Depth in the directory tree node
} fs_directory_tree_node_t;

void fs_print_ls_directory_header();

void fs_print_file_leaf(fat12_file_subdir_s dir, uint8_t depth);

// Reads the root directory of the FAT12 file system and returns a pointer to a fs_directory_t structure
// WARNING: The returned pointer must be freed after use to avoid memory leaks (arrfree()).
fs_directory_t fs_read_root_directory(FILE *disk);
fs_directory_t fs_read_directory(FILE *disk, uint16_t cluster);

// Creates a disk tree structure from the FAT12 file system
// This function reads the root directory and builds a tree structure of directories and files.
// WARNING: The returned pointer must be freed after use to avoid memory leaks (fs_free_disk_tree()).
fs_directory_tree_node_t *fs_create_disk_tree(FILE *disk);

void fs_print_directory_tree(fs_directory_tree_node_t *dir_tree);

// Frees the memory allocated for a fs_directory_t structure
void fs_free_directory(fs_directory_t dir);

void fs_free_disk_tree(fs_directory_tree_node_t *dir_tree);

#endif  // FILE_SYSTEM_H