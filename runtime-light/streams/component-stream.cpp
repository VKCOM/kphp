#include "runtime-light/streams/component-stream.h"

bool f$ComponentStream$$is_read_closed(const class_instance<C$ComponentStream> & stream) {
  StreamStatus status;
  GetStatusResult res = get_platform_context()->get_stream_status(stream.get()->stream_d, &status);
  if (res != GetStatusOk) {
    php_warning("cannot get stream status error %d", res);
    return true;
  }
  return status.read_status == IOClosed;
}

bool f$ComponentStream$$is_write_closed(const class_instance<C$ComponentStream> & stream) {
  StreamStatus status;
  GetStatusResult res = get_platform_context()->get_stream_status(stream.get()->stream_d, &status);
  if (res != GetStatusOk) {
    php_warning("cannot get stream status error %d", res);
    return true;
  }
  return status.write_status == IOClosed;
}

bool f$ComponentStream$$is_please_shutdown_write(const class_instance<C$ComponentStream> & stream) {
  StreamStatus status;
  GetStatusResult res = get_platform_context()->get_stream_status(stream.get()->stream_d, &status);
  if (res != GetStatusOk) {
    php_warning("cannot get stream status error %d", res);
    return true;
  }
  return status.please_shutdown_write;
}

void f$ComponentStream$$close(const class_instance<C$ComponentStream> & stream) {
  free_descriptor(stream->stream_d);
}

void f$ComponentStream$$shutdown_write(const class_instance<C$ComponentStream> & stream) {
  get_platform_context()->shutdown_write(stream->stream_d);
}

void f$ComponentStream$$please_shutdown_write(const class_instance<C$ComponentStream> & stream) {
  get_platform_context()->please_shutdown_write(stream->stream_d);
}
