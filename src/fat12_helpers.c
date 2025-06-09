#include "fat12_helpers.h"

char* f12h_format_filename(fat12_file_subdir_s dir, char* buffer) {
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
char* f12h_format_date_time(fat12_date_s date, fat12_time_s time, char* buffer) {
    if (buffer == NULL) {
        return NULL;  // Error: buffer is null
    }

    // Format the date and time into the buffer
    snprintf(buffer, F12H_FORMATTED_DATE_TIME_BUFFER_SIZE, "%02u/%02u/%04u %02uh:%02um:%02us",
             date.day, date.month, date.year,
             time.hours, time.minutes, time.seconds);

    return buffer;
}

uint16_t f12h_pack_date(fat12_date_s date) {
    // Pack the date into a 16-bit value
    uint16_t packed_date = 0;
    packed_date |= (date.year - 1980) << 9;  // Year since 1980
    packed_date |= date.month << 5;          // Month (1-12)
    packed_date |= date.day;                 // Day (1-31)
    return packed_date;
}

uint16_t f12h_pack_time(fat12_time_s time) {
    // Pack the time into a 16-bit value
    uint16_t packed_time = 0;
    packed_time |= (time.hours & 0x1F) << 11;   // Hours (0-23)
    packed_time |= (time.minutes & 0x3F) << 5;  // Minutes (0-59)
    packed_time |= (time.seconds & 0x1F) * 2;   // Seconds (0-29, multiplied by 2)
    return packed_time;
}