// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/write-files.h"

#include "common/crc32.h"

#include "compiler/compiler-core.h"
#include "compiler/stage.h"

void WriteFilesF::execute(WriterData *data, EmptyStream &) {
  stage::set_name("Write files");

  File *file = data->get_file();

  bool need_del = false;
  bool need_fix = false;
  bool need_save_time = false;
  const unsigned long long hash_of_cpp = data->get_hash_of_cpp();
  const unsigned long long hash_of_comments = data->get_hash_of_comments();
  if (file->on_disk && data->compile_with_crc()) {
    if (file->crc64 != hash_of_cpp) {
      need_fix = true;
      need_del = true;
    } else if (file->crc64_with_comments != hash_of_comments) {
      need_fix = true;
      need_del = true;
      need_save_time = true;
    }
  } else {
    need_fix = true;
  }

  if (need_fix) {
    string code_str;
    data->dump(code_str);
//    printf("overwrite file %s need_fix=%d need_del=%d need_save_time=%d\n", file->path.c_str(), need_fix, need_del, need_save_time);

    long long mtime_before = 0;
    if (need_save_time) {
      int res = file->read_stat();
      mtime_before = file->mtime;
      if (res <= 0) {
        need_save_time = false;
      }
    }
    if (need_del) {
      int err = unlink(file->path.c_str());
      kphp_assert_msg(err == 0, fmt_format("Failed to unlink [{}] : {}", file->path, strerror(errno)));
    }
    FILE *dest_file = fopen(file->path.c_str(), "w");
    kphp_assert_msg(dest_file != nullptr,
                fmt_format("Failed to open [{}] for write : {}\n", file->path, strerror(errno)));

    if (data->compile_with_crc()) {
      // the first two lines of every .cpp/.h are hashes
      // they are named "crc64", but actually are not crc of contents, just some hashes calculated on the fly
      // also, "crc64_with_comments" is actually "hash OF comments", not "with"
      // such naming at the top of every file is left for backwards compatibility reasons
      kphp_assert(fprintf(dest_file, "//crc64:%016llx\n", ~hash_of_cpp) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64_with_comments:%016llx\n", ~hash_of_comments) >= 0);
      kphp_assert(fprintf(dest_file, "%s", code_str.c_str()) >= 0);
      kphp_assert(fflush(dest_file) >= 0);
      kphp_assert(fseek(dest_file, 0, SEEK_SET) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64:%016llx\n", hash_of_cpp) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64_with_comments:%016llx\n", hash_of_comments) >= 0);
    }
    else {
      kphp_assert(fprintf(dest_file, "%s", code_str.c_str()) >= 0);
    }

    kphp_assert(fflush(dest_file) >= 0);
    kphp_assert(fclose(dest_file) >= 0);

    file->crc64 = hash_of_cpp;
    file->crc64_with_comments = hash_of_comments;
    file->on_disk = true;

    if (need_save_time) {
      file->set_mtime(mtime_before);
    } else {
      file->is_changed = true;
    }
    long long res = file->read_stat();
    kphp_assert_msg(res > 0, "Stat failed");
    kphp_error(!need_save_time || file->mtime == mtime_before, "Failed to set previous mtime\n");
  }

  delete data;
}
