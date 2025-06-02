#include "fat12.h"

fat12_boot_sector_s fat12_read_boot_sector(FILE *disk) {
    assert(disk != NULL);

    fat12_boot_sector_s boot_sector;

    // Read the boot sector into the structure
    size_t bytes_read = fread(&boot_sector, sizeof(fat12_boot_sector_s), 1, disk);
    if (bytes_read != 1) {
        perror("Failed to read boot sector");
        exit(EXIT_FAILURE);
    }

    return boot_sector;
}

void fat12_print_boot_sector_info(fat12_boot_sector_s bs) {
    printf("Boot Sector Information:\n");
    printf("Sector Size: %u bytes\n", bs.sector_size);
    printf("Sectors per Cluster: %u\n", bs.sectors_per_cluster);
    printf("Number of Reserved Sectors: %u\n", bs.num_of_reserved_sectors);
    printf("Number of FATs: %u\n", bs.num_of_fats);
    printf("Max Number of Root Directory Entries: %u\n", bs.max_num_of_root_directory_entries);
    printf("Total Number of Sectors on Disk: %u\n", bs.qnt_of_sectors_on_disk);
    printf("Sectors per FAT: %u\n", bs.sectors_per_fat);
    printf("Sectors per Track: %u\n", bs.setctors_per_track);
    printf("Number of Heads: %u\n", bs.num_of_heads);
    printf("Total Sector Count for FAT32: %u\n", bs.total_sector_count_for_fat32);
    printf("Boot Signature: 0x%02X\n", bs.boot_signature);
    printf("Volume ID: 0x%08X\n", bs.volume_id);
    printf("Volume Label: %.11s\n", bs.volume_label);
    printf("File System Type: %.8s\n", bs.type_of_file_system);
}

// Reads a cluster from a FAT12 disk image and overwrites the provided buffer with the cluster data.
uint8_t *fat12_read_cluster(FILE *disk, uint8_t *buffer, uint16_t cluster_number) {
    assert(disk != NULL);
    assert(buffer != NULL);

    // Calculate the offset for the cluster
    uint64_t offset = cluster_number * SECTOR_SIZE;

    // Move the file pointer to the correct position
    if (fseek(disk, offset, SEEK_SET) != 0) {
        perror("Failed to seek to cluster position");
        return NULL;
    }

    // Read the cluster data into the buffer
    size_t bytes_read = fread(buffer, sizeof(uint8_t), SECTOR_SIZE, disk);
    if (bytes_read != SECTOR_SIZE) {
        perror("Failed to read cluster data");
        return NULL;
    }

    return buffer;
}