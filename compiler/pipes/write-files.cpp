#include "compiler/pipes/write-files.h"

#include "common/crc32.h"

#include "compiler/compiler-core.h"

void WriteFilesF::execute(WriterData *data, EmptyStream &) {
  AUTO_PROF (end_write);
  stage::set_name("Write files");
  string dir = G->cpp_dir;

  string cur_file_name = data->file_name;
  string cur_subdir = data->subdir;

  string full_file_name = dir;
  if (!cur_subdir.empty()) {
    full_file_name += cur_subdir;
    full_file_name += "/";
  }
  full_file_name += cur_file_name;

  File *file = G->get_file_info(full_file_name);
  file->needed = true;
  file->includes = data->get_includes();
  file->compile_with_debug_info_flag = data->compile_with_debug_info();

  if (file->on_disk && data->compile_with_crc()) {
    if (file->crc64 == (unsigned long long)-1) {
      FILE *old_file = fopen(full_file_name.c_str(), "r");
      dl_passert (old_file != nullptr,
                  format("Failed to open [%s]", full_file_name.c_str()));
      unsigned long long old_crc = 0;
      unsigned long long old_crc_with_comments = static_cast<unsigned long long>(-1);

      if (fscanf(old_file, "//crc64:%Lx", &old_crc) != 1) {
        kphp_warning (format("can't read crc64 from [%s]\n", full_file_name.c_str()));
        old_crc = static_cast<unsigned long long>(-1);
      } else {
        if (fscanf(old_file, " //crc64_with_comments:%Lx", &old_crc_with_comments) != 1) {
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
      int upd_res = file->upd_mtime();
      mtime_before = file->mtime;
      if (upd_res <= 0) {
        need_save_time = false;
      }
    }
    if (!need_save_time && G->env().get_verbosity() > 0) {
      fprintf(stderr, "File [%s] changed\n", full_file_name.c_str());
    }
    if (need_del) {
      int err = unlink(full_file_name.c_str());
      dl_passert (err == 0, format("Failed to unlink [%s]", full_file_name.c_str()));
    }
    FILE *dest_file = fopen(full_file_name.c_str(), "w");
    dl_passert (dest_file != nullptr,
                format("Failed to open [%s] for write\n", full_file_name.c_str()));

    if (data->compile_with_crc()) {
      dl_pcheck (fprintf(dest_file, "//crc64:%016Lx\n", ~crc));
      dl_pcheck (fprintf(dest_file, "//crc64_with_comments:%016Lx\n", ~crc_with_comments));
      dl_pcheck (fprintf(dest_file, "%s", code_str.c_str()));
      dl_pcheck (fflush(dest_file));
      dl_pcheck (fseek(dest_file, 0, SEEK_SET));
      dl_pcheck (fprintf(dest_file, "//crc64:%016Lx\n", crc));
      dl_pcheck (fprintf(dest_file, "//crc64_with_comments:%016Lx\n", crc_with_comments));
    }
    else {
      dl_pcheck (fprintf(dest_file, "%s", code_str.c_str()));
    }

    dl_pcheck (fflush(dest_file));
    dl_pcheck (fclose(dest_file));

    file->crc64 = crc;
    file->crc64_with_comments = crc_with_comments;
    file->on_disk = true;

    if (need_save_time) {
      file->set_mtime(mtime_before);
    }
    long long mtime = file->upd_mtime();
    dl_assert (mtime > 0, "Stat failed");
    kphp_error(!need_save_time || file->mtime == mtime_before, "Failed to set previous mtime\n");
  }
  delete data;
}
