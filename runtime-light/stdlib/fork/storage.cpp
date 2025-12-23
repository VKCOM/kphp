// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/storage.h"


namespace kphp::forks::details {

auto storage::store() noexcept -> void {
  kphp::log::assertion(!m_opt_tag.has_value());
  m_opt_tag.emplace(tagger<void>::get_tag());
}

} // namespace kphp::forks::details
