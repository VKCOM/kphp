#pragma once

#include "kphp_core.h"


typedef var Stream;


const int STREAM_SET_BLOCKING_OPTION = 0;
const int STREAM_SET_WRITE_BUFFER_OPTION = 1;
const int STREAM_SET_READ_BUFFER_OPTION = 2;


struct stream_functions {
  string name;

  Stream (*fopen) (const string &stream, const string &mode);

  Stream (*stream_socket_client) (const string &url, int &error_number, string &error_description, double timeout, int flags, const var &context);

  OrFalse <int> (*fwrite) (const Stream &stream, const string &data);

  int (*fseek) (const Stream &stream, int offset, int whence);

  OrFalse <int> (*ftell) (const Stream &stream);

  OrFalse <string> (*fread) (const Stream &stream, int length);

  OrFalse <int> (*fpassthru) (const Stream &stream);

  bool (*fflush) (const Stream &stream);

  bool (*feof) (const Stream &stream);

  bool (*fclose) (const Stream &stream);

  OrFalse <string> (*file_get_contents) (const string &url);

  OrFalse <int> (*file_put_contents) (const string &url, const string &content);
  
  bool (*context_set_option) (var &context, const string &option, const var &value);

  bool (*stream_set_option) (const Stream &stream, int option, int value);

  int (*get_fd) (const Stream &stream);
};


void register_stream_functions (const stream_functions *functions, bool is_default);


Stream f$fopen (const string &url, const string &mode);

OrFalse <int> f$fwrite (const Stream &stream, const string &text);

int f$fseek (const Stream &stream, int offset, int whence = 0);

bool f$rewind (const Stream &stream);

OrFalse <int> f$ftell (const Stream &stream);

OrFalse <string> f$fread (const Stream &stream, int length);

OrFalse <int> f$fpassthru (const Stream &stream);

bool f$fflush (const Stream &stream);

bool f$feof (const Stream &stream);

bool f$fclose (const Stream &stream);


OrFalse <string> f$file_get_contents (const string &url);

OrFalse <int> f$file_put_contents (const string &url, const var &content_var);


var f$stream_context_create (const var &options = array <var>());

bool f$stream_context_set_option (var &context, const var &options_var);
bool f$stream_context_set_option (var &context, const var &, const string &);
bool f$stream_context_set_option (var &context, const var &wrapper, const string &option, const var &value);

extern var error_number_dummy;
extern var error_description_dummy;

const int STREAM_CLIENT_CONNECT = 1;

var f$stream_socket_client (const string &url, var &error_number = error_number_dummy, var &error_description = error_description_dummy, double timeout = -1, int flags = STREAM_CLIENT_CONNECT, const var &context = var());


bool f$stream_set_blocking (const Stream &stream, bool mode);

int f$stream_set_write_buffer (const Stream &stream, int size);

int f$stream_set_read_buffer (const Stream &stream, int size);

OrFalse <int> f$stream_select (var &read, var &write, var &except, const var &tv_sec, int tv_usec = 0);


void streams_init_static (void);

void streams_free_static (void);
