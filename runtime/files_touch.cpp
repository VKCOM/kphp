// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC Sigmalab
// Distributed under the GPL v3 License, see LICENSE.notice.txt
#include <ctime>
#include <sys/types.h>
#include <utime.h>

bool f$touch(const string filename, const int64_t mtime, const int64_t atime)
{
  if (FILE *file = fopen(filename.c_str(), "a+")) {
     fclose(file);
  }
  else {
    return false;
  }

  time_t now = std::time(NULL);
  time_t val_mtime = !mtime ? now : mtime;
  time_t val_atime = !atime ? now : atime;
  const utimbuf   timebuf = {val_atime, val_mtime};
  return utime(filename.c_str(), &timebuf ) == 0;
}
