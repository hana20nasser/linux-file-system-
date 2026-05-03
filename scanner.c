#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "scanner.h"


void scan_recursive(const char *basepath,FileInfo files[],int *count){
   struct dirent *dp;
   DIR *dir = opendir(basepath);
   if(!dir) return;
   while ((dp = readdir(dir)) !=NULL){

     if (strcmp(dp->d_name, ".") ==0  ||strcmp(dp->d_name,"..") ==0)
        continue;
      char path[512];

      sprintf(path, "%s/%s", basepath, dp->d_name);
      
      struct stat st;
      stat(path, &st);

      if (stat(path, &st)== -1){
         perror("stat error");
         continue;
      }


//if  file normal

   if (S_ISREG(st.st_mode)){
     strcpy(files[*count].path,path);
     files[*count].size =st.st_size;
     files[*count].modified_time =st.st_mtime;
     (*count)++;
   }
//if folder recursion

  if (S_ISDIR(st.st_mode)){
    scan_recursive(path, files, count);
   }
 }
  closedir(dir);
}

int scan_directory(const char *path, FileInfo files[], int *count){

  *count =0;
  scan_recursive(path, files, count);
  return 0;
}
