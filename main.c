#include <stdio.h>
#include "scanner.h"
#include <time.h>

int main(){
   FileInfo files[1000];
   int count;

   scan_directory("/home/vboxuser/file_project", files, &count);
 for (int i=0;i< count; i++){

    char timebuf[100];
    struct tm *tm_info = localtime(&files[i].modified_time);
    strftime(timebuf, 100, "%Y-%m-%d %H:%M", tm_info);
    printf("%s  | %ld bytes | %s\n",
      files[i].path,
      files[i].size,
      timebuf);
    }
    return 0;
}
