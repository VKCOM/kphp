// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/write-files.h"

#include "common/crc32.h"

#include "compiler/compiler-core.h"
#include "compiler/stage.h"

void WriteFilesF::execute(WriterData *data, EmptyStream &) {
  stage::set_name("Write files");

  std::string full_file_name = G->cpp_dir;
  if (!data->subdir.empty()) {
    full_file_name += data->subdir;
    full_file_name += "/";
  }
  full_file_name += data->file_name;

  File *file = G->get_file_info(std::move(full_file_name));
  file->needed = true;
  file->includes = data->flush_includes();
  file->lib_includes = data->flush_lib_includes();

  file->compile_with_debug_info_flag = data->compile_with_debug_info();

  if (file->on_disk && data->compile_with_crc()) {
    if (file->crc64 == (unsigned long long)-1) {
      FILE *old_file = fopen(file->path.c_str(), "r");
      kphp_assert_msg (old_file != nullptr,
                  fmt_format("Failed to open [{}] : {}", file->path, strerror(errno)));
      unsigned long long old_crc = 0;
      unsigned long long old_crc_with_comments = static_cast<unsigned long long>(-1);

      if (fscanf(old_file, "//crc64:%llx", &old_crc) != 1) {
        kphp_warning (fmt_format("can't read crc64 from [{}]\n", file->path));
        old_crc = static_cast<unsigned long long>(-1);
      } else {
        if (fscanf(old_file, " //crc64_with_comments:%llx", &old_crc_with_comments) != 1) {
          old_crc_with_comments = static_cast<unsigned long long>(-1);
        }
      }
      fclose(old_file);

      file->crc64 = old_crc;
      file->crc64_with_comments = old_crc_with_comments;
    }
  }

  bool need_del = false;
  bool need_fix = false;
  bool need_save_time = false;
  const unsigned long long crc = data->calc_crc();
  string code_str;
  data->dump(code_str);
  const unsigned long long crc_with_comments = compute_crc64(code_str.c_str(), code_str.length());
  if (file->on_disk && data->compile_with_crc()) {
    if (file->crc64 != crc) {
      need_fix = true;
      need_del = true;
    } else if (file->crc64_with_comments != crc_with_comments) {
      need_fix = true;
      need_del = true;
      need_save_time = true;
    }
  } else {
    need_fix = true;
  }

  if (need_fix) {
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
      kphp_assert(fprintf(dest_file, "//crc64:%016llx\n", ~crc) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64_with_comments:%016llx\n", ~crc_with_comments) >= 0);
      kphp_assert(fprintf(dest_file, "%s", code_str.c_str()) >= 0);
      kphp_assert(fflush(dest_file) >= 0);
      kphp_assert(fseek(dest_file, 0, SEEK_SET) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64:%016llx\n", crc) >= 0);
      kphp_assert(fprintf(dest_file, "//crc64_with_comments:%016llx\n", crc_with_comments) >= 0);
    }
    else {
      kphp_assert(fprintf(dest_file, "%s", code_str.c_str()) >= 0);
    }

    kphp_assert(fflush(dest_file) >= 0);
    kphp_assert(fclose(dest_file) >= 0);

    file->crc64 = crc;
    file->crc64_with_comments = crc_with_comments;
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
