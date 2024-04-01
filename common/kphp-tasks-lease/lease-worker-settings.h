// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/tl/constants/kphp.h"

// This settings is sent from tasks to KPHP in kphp.processLeaseTask

struct lease_worker_settings {
  int fields_mask{0};
  int php_timeout_ms{0};

  lease_worker_settings() = default;
  explicit lease_worker_settings(int php_timeout_ms)
    : fields_mask(vk::tl::kphp::lease_worker_settings_fields_mask::php_timeout)
    , php_timeout_ms(php_timeout_ms){};

  bool has_timeout() const {
    return fields_mask & vk::tl::kphp::lease_worker_settings_fields_mask::php_timeout;
  }
};
