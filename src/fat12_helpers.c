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
