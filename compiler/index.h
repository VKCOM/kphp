#pragma once
#include "../common.h"

/*** Index ***/
void mkpath (const string &s);
class Target;

class File {
  private:
    File (const File &from);
    File &operator = (const File &from);
  public:
    string path;
    string name;
    string ext;
    string name_without_ext;
    string subdir;
    long long mtime;
    unsigned long long crc64;
    bool on_disk;
    bool needed;
    Target *target;
    //Don't know where else I can save it
    vector <string> includes;
    File();
    explicit File (const string &path);
    long long upd_mtime() __attribute__ ((warn_unused_result));
    void unlink();
};

class Index {
  private:
    map <string, File *> files;
    string dir;
    set <string> subdirs;
    void remove_file (const string &path);
    void create_subdir (const string &subdir);

    static Index *current_index;
    static int scan_dir_callback (const char *fpath, const struct stat *sb, 
        int typeflag, struct FTW *ftwbuf);
  public:

    void set_dir (const string &dir);
    const string &get_dir() const;
    void sync_with_dir (const string &dir);
    void del_extra_files();
    File *get_file (string path, bool force = false);
    vector <File *> get_files();

    //stupid text version. to be improved
    void save (FILE *f);
    void load (FILE *f);
};
