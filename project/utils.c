#include "kernel/types.h"
#include "user/user.h"
#include "utils.h"

void fmt_path(char* buf, const char* path, const char* name) {
  char *p;
  strcpy(buf, path);
  p = buf + strlen(buf);
  if(p > buf && *(p-1) != '/'){
    *p++ = '/';
  }
  strcpy(p, name);
}
