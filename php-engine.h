#pragma once

enum class php_mode {
  cli,
  server
};

int run_main(int argc, char **argv, php_mode mode);
