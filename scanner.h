#ifndef SCANNER_H
#define SCANNER_H

typedef struct {
   char path[256];
   long size;
   long modified_time;
  } FileInfo;



 int scan_directory(const char *path, FileInfo files[],int *count);

#endif
