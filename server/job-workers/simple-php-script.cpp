// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/interface.h"
#include "server/php-runner.h"
#include "server/php-script.h"

#include "server/job-workers/simple-php-script.h"


namespace job_workers {

static SharedMemorySlice *server_reply{nullptr};
static SharedMemorySlice *server_request{nullptr};
static void *php_script{nullptr};

void store_reply(SharedMemorySlice *reply) noexcept {
  assert(!server_reply);
  server_reply = reply;
}

SharedMemorySlice *fetch_request() noexcept {
  return server_request;
}

SharedMemorySlice *run_simple_php_script(SharedMemorySlice *request) noexcept {
  assert(!server_reply);
  assert(!server_request);

  server_request = request;

  if (!php_script) {
    php_script = php_script_create(10 * 1024 * 1024, 10 * 1024 * 1024);
  }
  php_script_init(php_script, get_script(), nullptr);
  php_script_iterate(php_script);
  php_script_finish(php_script);
  php_script_clear(php_script);

  SharedMemorySlice *reply = server_reply;
  server_reply = nullptr;
  server_request = nullptr;
  return reply;
}

} // namespace job_workers
