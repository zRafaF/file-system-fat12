#include "fat12.h"

static bool has_loaded_fat_table = false;

static uint8_t fat_table[SECTOR_SIZE * 9];  // FAT12 can have up to 9 sectors for the FAT table

static inline fat12_time_s fat12_extract_time(uint16_t time) {
    // Extraído de https://fileadmin.cs.lth.se/cs/Education/EDA385/HT09/student_doc/FinalReports/FAT12_overview.pdf

    fat12_time_s t;
    t.seconds = (time & 0x1F) / 2;
    t.minutes = (time >> 5) & 0x3F;
    t.hours = (time >> 11) & 0x1F;
    return t;
}

static inline fat12_date_s fat12_extract_date(uint16_t date) {
    // Extraído de https://fileadmin.cs.lth.se/cs/Education/EDA385/HT09/student_doc/FinalReports/FAT12_overview.pdf

    fat12_date_s d;
    d.day = (date & 0x1F);
    d.month = (date >> 5) & 0x0F;
    d.year = ((date >> 9) & 0b1111111) + 1980;  // Year since 1980
    return d;
}

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

fat12_directory_s fat12_read_directory_entry(FILE *disk, uint16_t entry_idx) {
    assert(disk != NULL);
    assert(entry_idx < (FAT12_NUM_OF_ROOT_DIRECTORY_SECTORS * FAT12_ROOT_DIRECTORY_ENTRIES_PER_SECTOR));

    fat12_directory_s dir_entry;

    // Calculate the sector where the directory entry is located
    uint16_t sector_idx = FAT12_ROOT_DIRECTORY_START + (entry_idx / FAT12_ROOT_DIRECTORY_ENTRIES_PER_SECTOR);
    uint16_t entry_offset = entry_idx % FAT12_ROOT_DIRECTORY_ENTRIES_PER_SECTOR;

    // Move to the position of the directory entry
    if (fseek(disk, sector_idx * SECTOR_SIZE + entry_offset * sizeof(fat12_directory_s), SEEK_SET) != 0) {
        perror("Failed to seek to directory entry position");
        exit(EXIT_FAILURE);
    }

    // Read the directory entry into the structure
    size_t bytes_read = fread(&dir_entry, sizeof(fat12_directory_s), 1, disk);
    if (bytes_read != 1) {
        perror("Failed to read directory entry");
        exit(EXIT_FAILURE);
    }

    return dir_entry;
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
    printf("Sectors per Track: %u\n", bs.sectors_per_track);
    printf("Number of Heads: %u\n", bs.num_of_heads);
    printf("Total Sector Count for FAT32: %u\n", bs.total_sector_count_for_fat32);
    printf("Boot Signature: 0x%02X\n", bs.boot_signature);
    printf("Volume ID: 0x%08X\n", bs.volume_id);
    printf("Volume Label: %.11s\n", bs.volume_label);
    printf("File System Type: %.8s\n", bs.type_of_file_system);
}

void fat12_print_directory_info(fat12_directory_s dir) {
    printf("Directory Entry Information:\n");
    printf("Filename: %.8s\n", dir.filename);
    printf("Extension: %.3s\n", dir.extension);
    printf("Attributes: 0x%02X\n", dir.attributes);
    fat12_time_s creation_time = fat12_extract_time(dir.creation_time);
    fat12_date_s creation_date = fat12_extract_date(dir.creation_date);

    printf("Creation Time: %02uh %02um %02us\n", creation_time.hours, creation_time.minutes, creation_time.seconds);
    printf("Creation Date: %02u/%02u/%04u\n", creation_date.day, creation_date.month, creation_date.year);

    fat12_date_s last_access_date = fat12_extract_date(dir.last_access_date);
    printf("Last Access Date: %02u/%02u/%04u\n", last_access_date.day, last_access_date.month, last_access_date.year);

    fat12_time_s last_write_time = fat12_extract_time(dir.last_write_time);
    fat12_date_s last_write_date = fat12_extract_date(dir.last_write_date);
    printf("Last Write Time: %02uh %02um %02us\n", last_write_time.hours, last_write_time.minutes, last_write_time.seconds);
    printf("Last Write Date: %02u/%02u/%04u\n", last_write_date.day, last_write_date.month, last_write_date.year);

    printf("First Cluster: %u\n", dir.first_cluster);
    printf("File Size: %u bytes\n", dir.file_size);
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

uint8_t *fat12_load_full_fat_table(FILE *disk) {
    assert(disk != NULL);

    // Move to the start of the FAT table
    if (fseek(disk, SECTOR_SIZE * FAT12_FAT_TABLES_START, SEEK_SET) != 0) {
        perror("Failed to seek to FAT table position");
        return NULL;
    }

    // Read the FAT table into the buffer
    size_t bytes_read = fread(fat_table, sizeof(uint8_t), SECTOR_SIZE * 9, disk);
    if (bytes_read != SECTOR_SIZE * FAT12_NUM_OF_FAT_TABLES_SECTORS) {
        perror("Failed to read FAT table data");
        return NULL;
    }

    has_loaded_fat_table = true;  // Mark that the FAT table has been loaded

    return fat_table;
}

// Reads a FAT12 table entry.
uint16_t fat12_get_table_entry(uint16_t entry_idx) {
    assert(has_loaded_fat_table);
    assert(entry_idx < 0xFFF);
    assert(fat_table != NULL);

    // Compute byte offset = floor(entry_idx * 1.5)
    uint32_t byte_offset = (entry_idx * 3) / 2;

    uint16_t value = 0;
    if ((entry_idx % 2) == 0) {
        // If entry_idx is even, the first byte contains the low 8 bits
        // and the second byte contains the high 4 bits.
        uint16_t lo = fat_table[byte_offset];
        uint16_t hi = fat_table[byte_offset + 1] & 0x0F;
        value = lo | (hi << 8);
    } else {
        // If entry_idx is odd, the first byte contains the low 4 bits
        // and the second byte contains the high 8 bits.
        uint16_t lo = (fat_table[byte_offset] >> 4) & 0x0F;
        uint16_t hi = fat_table[byte_offset + 1];
        value = (lo) | (hi << 4);
    }

    return value;
}
