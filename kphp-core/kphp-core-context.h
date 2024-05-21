#pragma once

struct KphpCoreContext {
  static KphpCoreContext& current() noexcept;

  int show_migration_php8_warning;
};

