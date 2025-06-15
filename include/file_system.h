#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include <string.h>

#include "fat12.h"
#include "fat12_helpers.h"

#define FS_MAX_DIRECTORY_DEPTH 32                                                              // Maximum depth of the directory tree
#define FS_MAX_FILENAME_LENGTH (FAT12_FILE_NAME_LENGTH + 1 + FAT12_FILE_EXTENSION_LENGTH + 1)  // Maximum length of a file name +2 for the dot and null terminator

typedef struct {
    fat12_file_subdir_s *files;    // Array of files in the directory
    fat12_file_subdir_s *subdirs;  // Array of subdirectories in the directory
} fs_directory_t;

typedef enum {
    FS_DIRECTORY_TYPE_FILE,    // Represents a file
    FS_DIRECTORY_TYPE_SUBDIR,  // Represents a subdirectory
} fs_directory_type_e;

typedef struct {
    char file[FAT12_FILE_NAME_LENGTH];
    char extension[FAT12_FILE_EXTENSION_LENGTH];
} fs_fat_compatible_filename_t;

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
// Finds a node in the directory tree by its path.
// Returns a pointer to the node if found, or NULL if not found.
fs_directory_tree_node_t *fs_get_node_by_path(fs_directory_tree_node_t *root, const char *path);
// Traverses the directory tree from the root to locate the node for the parent directory in a given path.
// Returns the node if found, or NULL for invalid or non-existent paths.
fs_directory_tree_node_t *fs_get_directory_node_by_path(fs_directory_tree_node_t *root, const char *path);

void fs_print_directory_tree(fs_directory_tree_node_t *dir_tree);

// Frees the memory allocated for a fs_directory_t structure
void fs_free_directory(fs_directory_t dir);

void fs_free_disk_tree(fs_directory_tree_node_t *dir_tree);

// Extracts the filename from a given path. That is the part after the last '/' or '\' character.
fs_fat_compatible_filename_t fs_get_filename_from_path(const char *path);

// Returns the total size of the file system in bytes. Returns 0 on error.
uint32_t fs_write_file_to_data_area(FILE *source_file, FILE *disk, uint16_t **cluster_list);
bool fs_write_cluster_chain_to_fat_table(FILE *disk, uint16_t *cluster_list);
// Adds a file to the disk, does not update the directory tree.
bool fs_add_file_to_directory(FILE *disk, fs_directory_tree_node_t *dir_node, fat12_file_subdir_s file_entry);

bool fs_remove_file_or_directory(FILE *disk, fs_directory_tree_node_t *dir_node);

#endif  // FILE_SYSTEM_H