#include "streams.h"
#include "files_popen.h"

#include <stdio.h>
#include "runtime/string_functions.h"//php_buf, TODO

const string popen_wrapper_name("popen://", 8);

//2022-04-19 av
Stream f$popen(const string &command, const string &mode)
{
  if (dl::query_num != opened_files_last_query_num) {
    new(opened_files_storage) array<FILE *>();

    opened_files_last_query_num = dl::query_num;
  }
  string fullname = popen_wrapper_name;
  fullname.append( command );
  
  printf("open: %s\n", fullname.c_str());
  dl::enter_critical_section();//NOT OK: opened_files
  FILE *file = popen(command.c_str(), mode.c_str());
  if (file == nullptr) {
    dl::leave_critical_section();
    return false;
  }

  opened_files->set_value(fullname, file);
  dl::leave_critical_section();

  return fullname;
}

bool f$pclose(const Stream &stream) {

  string filename = stream.to_string();
  if (dl::query_num == opened_files_last_query_num && opened_files->has_key(filename)) {
    dl::enter_critical_section();//NOT OK: opened_files
    int result = pclose(opened_files->get_value(filename));
    opened_files->unset(filename);
    dl::leave_critical_section();
    return (result == 0);
  }
  return false;
}

void global_init_popen_lib() {
  static stream_functions file_stream_functions;

  file_stream_functions.name = string("popen", 5);
  file_stream_functions.fopen = f$popen;
  file_stream_functions.fwrite = file_fwrite;
  file_stream_functions.fseek = nullptr;;
  file_stream_functions.ftell = nullptr;
  file_stream_functions.fread = file_fread;
  file_stream_functions.fgetc = file_fgetc;
  file_stream_functions.fgets = file_fgets;
  file_stream_functions.fpassthru = file_fpassthru;
  file_stream_functions.fflush = file_fflush;
  file_stream_functions.feof = file_feof;
  file_stream_functions.fclose = f$pclose;

  file_stream_functions.file_get_contents = file_file_get_contents;
  file_stream_functions.file_put_contents = file_file_put_contents;

  file_stream_functions.stream_socket_client = nullptr;
  file_stream_functions.context_set_option = nullptr;
  file_stream_functions.stream_set_option = nullptr;
  file_stream_functions.get_fd = nullptr;

  register_stream_functions(&file_stream_functions, false);
}


void free_popen_lib() {
  dl::enter_critical_section();//OK
  if (dl::query_num == opened_files_last_query_num) {
    const array<FILE *> *const_opened_files = opened_files;
    for (array<FILE *>::const_iterator p = const_opened_files->begin(); p != const_opened_files->end(); ++p) {
      fclose(p.get_value());
    }
    opened_files_last_query_num--;
  }

  if (opened_fd != -1) {
    close_safe(opened_fd);
  }
  opened_fd = -1;
  dl::leave_critical_section();
}
