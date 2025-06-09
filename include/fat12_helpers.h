#ifndef FAT12_HELPERS_H
#define FAT12_HELPERS_H

#include "fat12.h"

#define F12H_FORMATTED_DATE_TIME_BUFFER_SIZE 29

char* f12h_format_filename(fat12_file_subdir_s dir, char* buffer);

char* f12h_format_date_time(fat12_date_s date, fat12_time_s time, char* buffer);

uint16_t f12h_pack_date(fat12_date_s date);
uint16_t f12h_pack_time(fat12_time_s time);

#endif  // FAT12_HELPERS_H