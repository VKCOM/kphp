#include <cstdlib>
#include <cstring>

#include "array_functions.h"
#include "streams.h"

static dl::size_type max_wrapper_name_size;

static array <const stream_functions *> wrappers;

static const stream_functions *default_stream_functions;

void register_stream_functions (const stream_functions *functions, bool is_default) {
  string wrapper_name = functions->name;

  php_assert (dl::query_num == 0);
  php_assert (functions != NULL);
  php_assert (!wrappers.isset (wrapper_name));
  php_assert (strlen (wrapper_name.c_str()) == wrapper_name.size());
  
  if (wrapper_name.size() > max_wrapper_name_size) {
    max_wrapper_name_size = wrapper_name.size();
  }

  wrappers.set_value (wrapper_name, functions);

  if (is_default) {
    php_assert (default_stream_functions == NULL);
    default_stream_functions = functions;
  }
}

static const stream_functions *get_stream_functions (const string &name) {
  return wrappers.get_value (name);
}

static const stream_functions *get_stream_functions_from_url (const string &url) {
  if (url.size() == 0) {
    return NULL;
  }

  void *res = memmem (static_cast <const void *> (url.c_str()), url.size(),
                      static_cast <const void *> ("://"), 3);
  if (res != NULL) {
    const char *wrapper_end = static_cast <const char *> (res);
    return get_stream_functions (string (url.c_str(), wrapper_end - url.c_str()));
  }

  return default_stream_functions;
}


var f$stream_context_create (const var &options) {
  var result;
  f$stream_context_set_option (result, options);
  return result;
}

bool f$stream_context_set_option (var &context, const var &wrapper, const string &option, const var &value) {
  if (!context.is_array() && !context.is_null()) {
    php_warning ("Wrong context specified");
    return false;
  }

  if (!wrapper.is_string()) {
    php_warning ("Parameter wrapper must be a string");
    return false;
  }

  string wrapper_string = wrapper.to_string();
  const stream_functions *functions = get_stream_functions (wrapper_string);
  if (functions == NULL) {
    php_warning ("Wrapper \"%s\" is not supported", wrapper_string.c_str());
    return false;
  }
  if (functions->context_set_option == NULL) {
    php_warning ("Wrapper \"%s\" doesn't support function stream_context_set_option", wrapper_string.c_str());
    return false;
  }

  return functions->context_set_option (context[wrapper_string], option, value);
}

bool f$stream_context_set_option (var &context __attribute__((unused)), const var &, const string &) {
  php_warning ("Function stream_context_set_option can't take 3 arguments");
  return false;
}

bool f$stream_context_set_option (var &context, const var &options_var) {
  if (!context.is_array() && !context.is_null()) {
    php_warning ("Wrong context specified");
    return false;
  }

  if (!options_var.is_array()) {
    php_warning ("Parameter options must be an array of arrays");
    return false;
  }

  bool was_error = false;
  const array <var> options_array = options_var.to_array();
  for (array <var>::const_iterator it = options_array.begin(); it != options_array.end(); ++it) {
    const var &wrapper = it.get_key();
    const var &values = it.get_value();

    if (!values.is_array()) {
      php_warning ("Parameter options[%s] must be an array", wrapper.to_string().c_str());
      was_error = true;
      continue;
    }

    const array <var> values_array = values.to_array();
    for (array <var>::const_iterator values_it = values_array.begin(); values_it != values_array.end(); ++values_it) {
      if (!f$stream_context_set_option (context, wrapper, f$strval (values_it.get_key()), values_it.get_value())) {
        was_error = true;
      }
    }
  }
  
  return !was_error;
}


var f$stream_socket_client (const string &url, int &error_number, string &error_description, double timeout, int flags, const var &context) {
  if (flags != STREAM_CLIENT_CONNECT) {
    php_warning ("Wrong parameter flags = %d in function stream_socket_client", flags);
    error_number = -1001;
    error_description = string ("Wrong parameter flags", 21);
    return false;
  }

  const stream_functions *functions = get_stream_functions_from_url (url);
  if (functions == NULL) {
    php_warning ("Can't find appropriate wrapper for \"%s\"", url.c_str());
    error_number = -1002;
    error_description = string ("Wrong wrapper", 13);
    return false;
  }
  if (functions->stream_socket_client == NULL) {
    php_warning ("Wrapper \"%s\" doesn't support function stream_socket_client", functions->name.c_str());
    error_number = -1003;
    error_description = string ("Wrong wrapper", 13);
    return false;
  }

  return functions->stream_socket_client (url, error_number, error_description, timeout, flags, context.get_value (functions->name));
}



#define STREAM_FUNCTION_BODY(function_name, error_result)                                             \
  const string &url = stream.to_string();                                                             \
                                                                                                      \
  const stream_functions *functions = get_stream_functions_from_url (url);                            \
  if (functions == NULL) {                                                                            \
    php_warning ("Can't find appropriate wrapper for \"%s\"", url.c_str());                           \
    return error_result;                                                                              \
  }                                                                                                   \
  if (functions->function_name == NULL) {                                                             \
    php_warning ("Wrapper \"%s\" doesn't support function " #function_name, functions->name.c_str()); \
    return error_result;                                                                              \
  }                                                                                                   \
                                                                                                      \
  return functions->function_name


Stream f$fopen (const string &stream, const string &mode) {
  STREAM_FUNCTION_BODY(fopen, false) (url, mode);
}

OrFalse <int> f$fwrite (const Stream &stream, const string &text) {
  STREAM_FUNCTION_BODY(fwrite, false) (stream, text);
}

int f$fseek (const Stream &stream, int offset, int whence) {
  STREAM_FUNCTION_BODY(fseek, -1) (stream, offset, whence);
}

bool f$rewind (const Stream &stream) {
  return f$fseek (stream, 0, 0) == 0;
}

OrFalse <int> f$ftell (const Stream &stream) {
  STREAM_FUNCTION_BODY(ftell, false) (stream);
}

OrFalse <string> f$fread (const Stream &stream, int length) {
  STREAM_FUNCTION_BODY(fread, false) (stream, length);
}

OrFalse <int> f$fpassthru (const Stream &stream) {
  STREAM_FUNCTION_BODY(fpassthru, false) (stream);
}

bool f$fflush (const Stream &stream) {
  STREAM_FUNCTION_BODY(fflush, false) (stream);
}

bool f$fclose (const Stream &stream) {
  STREAM_FUNCTION_BODY(fclose, false) (stream);
}


OrFalse <string> f$file_get_contents (const string &stream) {
  STREAM_FUNCTION_BODY(file_get_contents, false) (url);
}

OrFalse <int> f$file_put_contents (const string &stream, const var &content_var) {
  string content;
  if (content_var.is_array()) {
    content = f$implode (string(), content_var.to_array());
  } else {
    content = content_var.to_string();
  }

  STREAM_FUNCTION_BODY(file_put_contents, false) (url, content);
}
