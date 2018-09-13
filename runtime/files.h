#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "runtime/kphp_core.h"

extern const string LETTER_a;


int close_safe(int fd);

int open_safe(const char *pathname, int flags);

int open_safe(const char *pathname, int flags, mode_t mode);

ssize_t read_safe(int fd, void *buf, size_t len);

ssize_t write_safe(int fd, const void *buf, size_t len);


string f$basename(const string &name, const string &suffix = string());

bool f$chmod(const string &s, int mode);

bool f$chmod(const string &s, int mode);

void f$clearstatcache(void);

bool f$copy(const string &from, const string &to);

string f$dirname(const string &name);

OrFalse<array<string>> f$file(const string &name);

bool f$file_exists(const string &name);

OrFalse<int> f$filesize(const string &name);

bool f$is_dir(const string &name);

bool f$is_file(const string &name);

bool f$is_readable(const string &name);

bool f$is_writeable(const string &name);

bool f$mkdir(const string &name, int mode = 0777, bool recursive = false);

string f$php_uname(const string &mode = LETTER_a);

bool f$rename(const string &oldname, const string &newname);

OrFalse<string> f$realpath(const string &path);

OrFalse<string> f$tempnam(const string &dir, const string &prefix);

bool f$unlink(const string &name);

OrFalse<array<string>> f$scandir(const string &directory);

OrFalse<string> file_file_get_contents(const string &name);


void files_init_static_once(void);

void files_init_static(void);

void files_free_static(void);
