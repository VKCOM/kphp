#include <gtest/gtest.h>

#include "compiler/compiler-core.h"
#include "compiler/lexer.h"

class CompilerTestsEnvironment final : public testing::Environment {
public:
  ~CompilerTestsEnvironment() final {}

  void SetUp() final {
    testing::Environment::SetUp();
    lexer_init();
    G = new CompilerCore();
    G->register_settings(new CompilerSettings{});
    OpInfo::init_static();
    MultiKey::init_static();
    TypeData::init_static();
  }

  void TearDown() final {
    testing::Environment::TearDown();
  }
};

const testing::Environment *compiler_tests_env = testing::AddGlobalTestEnvironment(new CompilerTestsEnvironment);
