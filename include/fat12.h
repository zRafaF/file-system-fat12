#ifndef FAT12_H
#define FAT12_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

#define FAT12_FAT_TABLES_START 1               // FAT12 starts at sector 1
#define FAT12_NUM_OF_FAT_TABLES_SECTORS 9      // FAT12 can have up to 9 sectors for the FAT table
#define FAT12_FAT_TABLES_RESERVED_ENTRIES 2    // FAT12 reserves the first two entries in the FAT table
#define FAT12_DIRECTORY_ENTRIES_PER_SECTOR 16  // Maximum number of entries 512 / 32 = 16 entries per sector

#define FAT12_NUM_OF_FAT_TABLES_ENTRIES (((SECTOR_SIZE * FAT12_NUM_OF_FAT_TABLES_SECTORS) / 3) + FAT12_FAT_TABLES_RESERVED_ENTRIES)  // Each FAT12 entry is 1.5 bytes, so we divide by 3

#define FAT12_FILE_NAME_LENGTH 8       // Maximum length of a file name in FAT12
#define FAT12_FILE_EXTENSION_LENGTH 3  // Maximum length of a file extension in FAT12

#define FAT12_ROOT_DIRECTORY_START 19                                                                         // Root directory starts at cluster 19 for FAT12
#define FAT12_NUM_OF_ROOT_DIRECTORY_SECTORS 14                                                                // FAT12 root directory can occupy up to 14 sectors
#define FAT12_ROOT_DIRECTORY_ENTRIES FAT12_DIRECTORY_ENTRIES_PER_SECTOR *FAT12_NUM_OF_ROOT_DIRECTORY_SECTORS  // Total number of entries in the root directory

// FAT12 data area
#define FAT12_DATA_AREA_START 33
#define FAT12_DATA_AREA_END 2879
#define FAT12_MAX_CLUSTER_NUMBER FAT12_DATA_AREA_END - FAT12_DATA_AREA_START
#define FAT12_DATA_AREA_NUMBER_OFFSET 2

// FAT12 entries code map
#define FAT12_FREE (0x000)            // Free cluster marker
#define FAT12_RESERVED_BEGIN (0xFF0)  // Reserved cluster marker
#define FAT12_RESERVED_END (0xFF6)    // Reserved cluster marker
#define FAT12_BAD (0xFF7)             // Bad cluster marker
#define FAT12_EOC_BEGIN (0xFF8)       // End of cluster chain marker
#define FAT12_EOC_END (0xFFF)         // End of cluster chain marker

// Packed para que o compilador não adicione padding entre os campos da estrutura
typedef struct __attribute__((__packed__)) {
    uint8_t ignore0[11];               // Ignored bytes
    uint16_t sector_size;              // Size of a sector in bytes
    uint8_t sectors_per_cluster;       // Number of sectors per cluster
    uint16_t num_of_reserved_sectors;  // Number of reserved sectors
    uint8_t num_of_fats;
    uint16_t max_num_of_root_directory_entries;
    uint16_t qnt_of_sectors_on_disk;  // Total number of sectors on the disk
    uint8_t ignore21;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_of_heads;
    uint8_t ignore28[4];
    uint32_t total_sector_count_for_fat32;  // Total sectors for FAT32 (0 for FAT12)
    uint8_t ignore36[2];
    uint8_t boot_signature;  // Boot signature (0x29)
    uint32_t volume_id;      // Volume ID
    char volume_label[11];   // Volume label
    char type_of_file_system[8];
} fat12_boot_sector_s;

typedef enum {
    FAT12_ATTR_NONE = 0x00,
    FAT12_ATTR_READ_ONLY = 0x01,
    FAT12_ATTR_HIDDEN = 0x02,
    FAT12_ATTR_SYSTEM = 0x04,
    FAT12_ATTR_VOLUME_LABEL = 0x08,
    FAT12_ATTR_DIRECTORY = 0x10,
    FAT12_ATTR_ARCHIVE = 0x20,
    FAT12_ATTR_UNUSED1 = 0x40,
    FAT12_ATTR_UNUSED2 = 0x80,
    FAT12_ATTR_LONG_NAME = 0x0F,  // Long file name attribute (Can be ignored)
} fat12_file_subdir_attributes_e;

typedef struct __attribute__((__packed__)) {
    char filename[FAT12_FILE_NAME_LENGTH];
    char extension[FAT12_FILE_EXTENSION_LENGTH];
    uint8_t attributes;         // File attributes
    uint8_t reserved[2];        // Reserved
    uint16_t creation_time;     // Creation time
    uint16_t creation_date;     // Creation date
    uint16_t last_access_date;  // Last access date
    uint16_t ignore_in_fat12;   // Ignored in FAT12
    uint16_t last_write_time;   // Last modification time
    uint16_t last_write_date;   // Last modification date
    uint16_t first_cluster;     // First cluster number
    uint32_t file_size;         // File size in bytes
} fat12_file_subdir_s;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
} fat12_time_s;

typedef struct {
    uint8_t day;
    uint8_t month;
    uint16_t year;
} fat12_date_s;

fat12_time_s fat12_extract_time(uint16_t time);
fat12_date_s fat12_extract_date(uint16_t date);

fat12_boot_sector_s fat12_read_boot_sector(FILE *disk);
fat12_file_subdir_s fat12_read_directory_entry(FILE *disk, uint16_t entry_idx);
fat12_file_subdir_s fat12_read_directory_from_data_area(FILE *disk, uint16_t cluster, uint8_t idx);

void fat12_print_boot_sector_info(fat12_boot_sector_s bs);
void fat12_print_directory_info(fat12_file_subdir_s dir);

uint8_t *fat12_read_data_sector(FILE *disk, uint8_t *buffer, uint16_t sector_number);
bool fat12_write_data_sector(FILE *disk, uint8_t *buffer, uint16_t sector_number);

uint8_t *fat12_load_full_fat_table(FILE *disk);

uint16_t fat12_get_table_entry(uint16_t entry_idx);

uint16_t fat12_find_next_free_entry(uint16_t start_idx);

// Reads a FAT12 table entry and returns the cluster chain starting from the first cluster.
// WARNING: The chain must be freed after use.
bool fat12_get_table_entry_chain(uint16_t first_entry, uint16_t **chain);

char *fat12_attribute_to_string(uint8_t attribute);

#endif  // FAT12_H