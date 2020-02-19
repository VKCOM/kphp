#include <array>
#include <gtest/gtest.h>

#include "runtime/interface.h"

class RuntimeTestsEnvironment : public testing::Environment {
public:
  ~RuntimeTestsEnvironment() final {}

  void SetUp() final {
    testing::Environment::SetUp();

    global_init_runtime_libs();
    global_init_script_allocator();

    init_runtime_environment(nullptr, memory_.data(), memory_.size());
  }

  void TearDown() final {
    free_runtime_environment();

    testing::Environment::TearDown();
  }

private:
  std::array<uint8_t, 16*1024*1024> memory_;
};

const testing::Environment* runtime_tests_env = testing::AddGlobalTestEnvironment(new RuntimeTestsEnvironment);
