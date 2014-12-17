#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "kphp_core.h"


extern string raw_post_data;

extern const string LETTER_a;


int close_safe (int fd);

int open_safe (const char *pathname, int flags);

int open_safe (const char *pathname, int flags, mode_t mode);

ssize_t read_safe (int fd, void *buf, size_t len);

ssize_t write_safe (int fd, const void *buf, size_t len);


string f$basename (const string &name, const string &suffix = string());

bool f$chmod (const string &s, int mode);

bool f$chmod (const string &s, int mode);

void f$clearstatcache (void);

bool f$copy (const string &from, const string &to);

string f$dirname (const string &name);

OrFalse <array <string> > f$file (const string &name);

OrFalse <string> f$file_get_contents (const string &name);

OrFalse <int> f$file_put_contents (const string &name, const var &content_var);

bool f$file_exists (const string &name);

OrFalse <int> f$filesize (const string &name);

bool f$is_dir (const string &name);

bool f$is_file (const string &name);

bool f$is_readable (const string &name);

bool f$is_writeable (const string &name);

bool f$mkdir (const string &name, int mode = 0777);

string f$php_uname (const string &mode = LETTER_a);

bool f$rename (const string &oldname, const string &newname);

OrFalse <string> f$realpath (const string &path);

OrFalse <string> f$tempnam (const string &dir, const string &prefix);

bool f$unlink (const string &name);


typedef var MyFile;

extern const MyFile STDOUT;
extern const MyFile STDERR;

MyFile f$fopen (const string &filename, const string &mode);

OrFalse <int> f$fwrite (const MyFile &file, const string &text);

int f$fseek (const MyFile &file, int offset, int whence = 0);

bool f$rewind (const MyFile &file);

OrFalse <int> f$ftell (const MyFile &file);

OrFalse <string> f$fread (const MyFile &file, int length);

OrFalse <int> f$fpassthru (const MyFile &file);

bool f$fflush (const MyFile &file);

bool f$fclose (const MyFile &file);


void files_init_static (void);

void files_free_static (void);
