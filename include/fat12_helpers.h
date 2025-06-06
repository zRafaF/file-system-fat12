#ifndef FAT12_HELPERS_H
#define FAT12_HELPERS_H

#include "fat12.h"

#define F12H_FORMATTED_DATE_TIME_BUFFER_SIZE 23

char* f12h_format_filename(fat12_file_subdir_s dir, char* buffer);

char* f12h_format_date_time(fat12_date_s date, fat12_time_s time, char* buffer);

#endif  // FAT12_HELPERS_H