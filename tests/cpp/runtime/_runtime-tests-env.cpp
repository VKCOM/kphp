#include <array>
#include <cassert>
#include <gtest/gtest.h>
#include <sys/mman.h>

#include "runtime/interface.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/pdo/pdo_statement.h"
#include "runtime/storage.h"
#include "runtime/tl/rpc_response.h"
#include "server/php-engine-vars.h"
#include "server/workers-control.h"

class RuntimeTestsEnvironment final : public testing::Environment {
public:
  ~RuntimeTestsEnvironment() final {}

  static void reset_global_vars() {
    pid = 0;
    logname_id = 0;
    vk::singleton<WorkersControl>::get().set_total_workers_count(1);
  }

  void SetUp() final {
    testing::Environment::SetUp();
    script_memory = static_cast<char *>(mmap(nullptr, script_memory_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));

    reset_global_vars();

    global_init_runtime_libs();
    global_init_script_allocator();

    init_runtime_environment(null_query_data{}, script_memory, script_memory_size);
    php_disable_warnings = true;
    php_warning_level = 0;
  }

  void TearDown() final {
    reset_global_vars();

    free_runtime_environment();

    testing::Environment::TearDown();
  }

private:
  static constexpr size_t script_memory_size = 16 * 1024 * 1024;
  char *script_memory{};
};

const testing::Environment *runtime_tests_env = testing::AddGlobalTestEnvironment(new RuntimeTestsEnvironment);

template<>
int Storage::tagger<bool>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<int64_t>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<Optional<int64_t>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<void>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<thrown_exception>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<mixed>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<array<mixed>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<Optional<string>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<Optional<array<mixed>>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<array<array<mixed>>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<class_instance<C$KphpJobWorkerResponse>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<class_instance<C$VK$TL$RpcResponse>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<array<class_instance<C$VK$TL$RpcResponse>>>::get_tag() noexcept {
  return 0;
}
template<>
int Storage::tagger<class_instance<C$PDOStatement>>::get_tag() noexcept {
  return 0;
}
template<>
Storage::loader<mixed>::loader_fun Storage::loader<mixed>::get_function(int) noexcept {
  return nullptr;
}

void init_php_scripts() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
void global_init_php_scripts() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
const char *get_php_scripts_version() noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}

char **get_runtime_options(int *) noexcept {
  assert(0 && "this code shouldn't be executed and only for linkage test");
  return nullptr;
}
