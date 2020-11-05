// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <gtest/gtest.h>

#include "common/parallel/thread-id.h"
#include "net/net-msg-buffers.h"
#include "net/net-msg.h"

class environment : public ::testing::Environment {
public:
  void SetUp() override {
    preallocate_msg_buffers();
    init_msg();
  }
};

static environment *const env __attribute__((unused)) = static_cast<environment *>(::testing::AddGlobalTestEnvironment(new environment()));
