#ifndef FAT12_H
#define FAT12_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"

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
    uint16_t setctors_per_track;
    uint16_t num_of_heads;
    uint8_t ignore28[4];
    uint32_t total_sector_count_for_fat32;  // Total sectors for FAT32 (0 for FAT12)
    uint8_t ignore36[2];
    uint8_t boot_signature;  // Boot signature (0x29)
    uint32_t volume_id;      // Volume ID
    char volume_label[11];   // Volume label
    char type_of_file_system[8];
} fat12_boot_sector_s;

fat12_boot_sector_s fat12_read_boot_sector(FILE *disk);
void fat12_print_boot_sector_info(fat12_boot_sector_s bs);
uint8_t *fat12_read_cluster(FILE *disk, uint8_t *buffer, uint16_t cluster_number);

#endif  // FAT12_H