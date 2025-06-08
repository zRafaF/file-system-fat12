#include "fat12.h"

#include "stb_ds.h"

static bool has_loaded_fat_table = false;

static uint8_t fat_table[SECTOR_SIZE * 9];  // FAT12 can have up to 9 sectors for the FAT table

static void fat12_reset_file_seek(FILE *disk) {
    assert(disk != NULL);
    if (fseek(disk, 0, SEEK_SET) != 0) {
        perror("Failed to reset file seek");
        exit(EXIT_FAILURE);
    }
}

fat12_time_s fat12_extract_time(uint16_t time) {
    // Extraído de https://fileadmin.cs.lth.se/cs/Education/EDA385/HT09/student_doc/FinalReports/FAT12_overview.pdf

    fat12_time_s t;
    t.seconds = (time & 0x1F) / 2;
    t.minutes = (time >> 5) & 0x3F;
    t.hours = (time >> 11) & 0x1F;
    return t;
}

fat12_date_s fat12_extract_date(uint16_t date) {
    // Extraído de https://fileadmin.cs.lth.se/cs/Education/EDA385/HT09/student_doc/FinalReports/FAT12_overview.pdf

    fat12_date_s d;
    d.day = (date & 0x1F);
    d.month = (date >> 5) & 0x0F;
    d.year = ((date >> 9) & 0b1111111) + 1980;  // Year since 1980
    return d;
}

fat12_boot_sector_s fat12_read_boot_sector(FILE *disk) {
    assert(disk != NULL);
    fat12_reset_file_seek(disk);

    fat12_boot_sector_s boot_sector;

    // Read the boot sector into the structure
    size_t bytes_read = fread(&boot_sector, sizeof(fat12_boot_sector_s), 1, disk);
    if (bytes_read != 1) {
        perror("Failed to read boot sector");
        exit(EXIT_FAILURE);
    }

    return boot_sector;
}

fat12_file_subdir_s fat12_read_directory_entry(FILE *disk, uint16_t entry_idx) {
    assert(disk != NULL);
    assert(entry_idx < (FAT12_NUM_OF_ROOT_DIRECTORY_SECTORS * FAT12_DIRECTORY_ENTRIES_PER_SECTOR));
    fat12_reset_file_seek(disk);

    fat12_file_subdir_s dir_entry;

    // Calculate the sector where the directory entry is located
    uint16_t sector_idx = FAT12_ROOT_DIRECTORY_START + (entry_idx / FAT12_DIRECTORY_ENTRIES_PER_SECTOR);
    uint16_t entry_offset = entry_idx % FAT12_DIRECTORY_ENTRIES_PER_SECTOR;

    // Move to the position of the directory entry
    if (fseek(disk, sector_idx * SECTOR_SIZE + entry_offset * sizeof(fat12_file_subdir_s), SEEK_SET) != 0) {
        perror("Failed to seek to directory entry position");
        exit(EXIT_FAILURE);
    }

    // Read the directory entry into the structure
    size_t bytes_read = fread(&dir_entry, sizeof(fat12_file_subdir_s), 1, disk);
    if (bytes_read != 1) {
        perror("Failed to read directory entry");
        exit(EXIT_FAILURE);
    }

    return dir_entry;
}

fat12_file_subdir_s fat12_read_directory_from_data_area(FILE *disk, uint16_t cluster, uint8_t idx) {
    assert(disk != NULL);
    assert(cluster < FAT12_MAX_CLUSTER_NUMBER);
    assert(idx < FAT12_DIRECTORY_ENTRIES_PER_SECTOR);
    fat12_reset_file_seek(disk);

    fat12_file_subdir_s dir_entry;

    const uint64_t offset = ((FAT12_DATA_AREA_START + (cluster - FAT12_DATA_AREA_NUMBER_OFFSET)) * SECTOR_SIZE) + (idx * sizeof(fat12_file_subdir_s));

    // Move to the start of the directory sector
    if (fseek(disk, offset, SEEK_SET) != 0) {
        perror("Failed to seek to directory sector position");
        exit(EXIT_FAILURE);
    }

    // Read the directory entry into the structure
    size_t bytes_read = fread(&dir_entry, sizeof(fat12_file_subdir_s), 1, disk);
    if (bytes_read != 1) {
        perror("Failed to read directory entry from sector");
        exit(EXIT_FAILURE);
    }

    return dir_entry;
}

char *fat12_attribute_to_string(uint8_t attribute) {
    switch (attribute) {
        case FAT12_ATTR_NONE:
            return "None";
        case FAT12_ATTR_READ_ONLY:
            return "Read Only";
        case FAT12_ATTR_HIDDEN:
            return "Hidden";
        case FAT12_ATTR_SYSTEM:
            return "System";
        case FAT12_ATTR_VOLUME_LABEL:
            return "Volume Label";
        case FAT12_ATTR_DIRECTORY:
            return "Directory";
        case FAT12_ATTR_ARCHIVE:
            return "Archive";
        case FAT12_ATTR_UNUSED1:
            return "Unused1";
        case FAT12_ATTR_UNUSED2:
            return "Unused2";
        case FAT12_ATTR_LONG_NAME:
            return "Long Name";
        default:
            return "Unknown";
    }
}

void fat12_print_boot_sector_info(fat12_boot_sector_s bs) {
    printf("\n===== BOOT SECTOR INFO =====\n\n");

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

void fat12_print_directory_info(fat12_file_subdir_s dir) {
    printf("\n===== DIR ENTRY INFO =====\n\n");

    printf("Filename: %.8s\n", dir.filename);
    printf("Extension: %.3s\n", dir.extension);

    printf("Attributes: (0x%02X) %s\n", dir.attributes, fat12_attribute_to_string(dir.attributes));

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
uint8_t *fat12_read_data_sector(FILE *disk, uint8_t *buffer, uint16_t sector_number) {
    assert(disk != NULL);
    assert(buffer != NULL);
    assert(sector_number < FAT12_MAX_CLUSTER_NUMBER);
    fat12_reset_file_seek(disk);

    // Calculate the offset for the cluster
    uint64_t offset = (FAT12_DATA_AREA_START + (sector_number - FAT12_DATA_AREA_NUMBER_OFFSET)) * SECTOR_SIZE;

    if (fseek(disk, offset, SEEK_SET) != 0) {
        perror("Failed to seek to cluster position");
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(uint8_t), SECTOR_SIZE, disk);
    if (bytes_read != SECTOR_SIZE) {
        perror("Failed to read cluster data");
        return NULL;
    }

    return buffer;
}

bool fat12_write_data_sector(FILE *disk, uint8_t *buffer, uint16_t sector_number) {
    assert(disk != NULL);
    assert(buffer != NULL);
    assert(sector_number < FAT12_MAX_CLUSTER_NUMBER);
    fat12_reset_file_seek(disk);

    // Calculate the offset for the cluster
    uint64_t offset = (FAT12_DATA_AREA_START + (sector_number - FAT12_DATA_AREA_NUMBER_OFFSET)) * SECTOR_SIZE;

    if (fseek(disk, offset, SEEK_SET) != 0) {
        perror("Failed to seek to cluster position");
        return false;
    }

    size_t bytes_written = fwrite(buffer, sizeof(uint8_t), SECTOR_SIZE, disk);
    if (bytes_written != SECTOR_SIZE) {
        perror("Failed to write cluster data");
        return false;
    }

    return true;
}

uint8_t *fat12_load_full_fat_table(FILE *disk) {
    assert(disk != NULL);
    fat12_reset_file_seek(disk);

    // Move to the start of the FAT table
    if (fseek(disk, SECTOR_SIZE * FAT12_FAT_TABLES_START, SEEK_SET) != 0) {
        perror("Failed to seek to FAT table position");
        return NULL;
    }

    // Clear the FAT table buffer
    memset(fat_table, 0, sizeof(fat_table));

    // Read the FAT table into the buffer
    size_t bytes_read = fread(fat_table, sizeof(uint8_t), SECTOR_SIZE * FAT12_NUM_OF_FAT_TABLES_SECTORS, disk);
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

uint16_t fat12_find_next_free_entry(uint16_t start_idx) {
    assert(has_loaded_fat_table);
    assert(fat_table != NULL);
    assert(start_idx < FAT12_NUM_OF_FAT_TABLES_ENTRIES);

    for (uint16_t i = start_idx; i < FAT12_NUM_OF_FAT_TABLES_ENTRIES; i++) {
        if (fat12_get_table_entry(i) == FAT12_FREE) {
            return i;
        }
    }

    fprintf(stderr, "No free entries found in the FAT table.\n");
    return 0;  // < 2 indicates no free entries found
}

bool fat12_get_table_entry_chain(uint16_t first_entry, uint16_t **chain) {
    assert(first_entry < FAT12_NUM_OF_FAT_TABLES_ENTRIES);

    arrpush(*chain, first_entry);
    size_t number_of_reads = 0;

    uint16_t current_entry = *chain[arrlen(*chain) - 1];
    while (true) {
        uint16_t next_entry = fat12_get_table_entry(current_entry);
        number_of_reads++;

        if (number_of_reads > FAT12_NUM_OF_FAT_TABLES_ENTRIES) {
            fprintf(stderr, "Too many reads from FAT table, possible infinite loop detected.\n");
            return false;  // Prevent infinite loop
        }

        if (next_entry >= FAT12_RESERVED_BEGIN && next_entry <= FAT12_RESERVED_END) {
            fprintf(stderr, "Invalid cluster encountered: %x\n", next_entry);
            return false;  // Stop on invalid cluster
        }

        if (next_entry == FAT12_BAD) {
            fprintf(stderr, "Bad cluster encountered: %x\n", next_entry);
            return false;  // Stop on bad cluster
        }
        if (next_entry == FAT12_FREE) {
            fprintf(stderr, "Pointed to free cluster: %x\n", next_entry);
            return false;  // Stop on bad cluster
        }

        if (next_entry >= FAT12_EOC_BEGIN && next_entry <= FAT12_EOC_END) {
            break;  // End of cluster
        }

        current_entry = next_entry;
        arrpush(*chain, next_entry);
    }

    return true;
}