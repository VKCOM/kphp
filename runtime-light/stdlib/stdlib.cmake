prepend(
  RUNTIME_LIGHT_STDLIB_SRC
  stdlib/
  component/component-api.cpp
  confdata/confdata-functions.cpp
  crypto/crypto-functions.cpp
  crypto/crypto-state.cpp
  curl/curl-state.cpp
  exit/exit-functions.cpp
  fork/fork-state.cpp
  job-worker/job-worker-api.cpp
  job-worker/job-worker-client-state.cpp
  math/random-state.cpp
  math/math-state.cpp
  output/output-buffer.cpp
  output/print-functions.cpp
  rpc/rpc-api.cpp
  rpc/rpc-state.cpp
  rpc/rpc-extra-headers.cpp
  rpc/rpc-extra-info.cpp
  rpc/rpc-tl-builtins.cpp
  rpc/rpc-tl-error.cpp
  rpc/rpc-tl-query.cpp
  rpc/rpc-tl-request.cpp
  serialization/serialization-state.cpp
  server/http-functions.cpp
  string/regex-functions.cpp
  string/regex-state.cpp
  string/string-state.cpp
  system/system-functions.cpp
  system/system-state.cpp
  file/file-system-functions.cpp
  file/file-system-state.cpp
  file/resource.cpp
  time/time-functions.cpp
  zlib/zlib-functions.cpp)
