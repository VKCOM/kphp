// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <iostream>

#include "common/options.h"
#include "common/tl2php/gen-php-code.h"
#include "common/tl2php/tl-hints.h"
#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"
#include "common/version-string.h"

namespace vk {
namespace tl {
int convert_tlo_to_php(const char *tlo_file_path, const std::string &out_php_dir, const std::string &combined2_tl_file, bool forcibly_overwrite_dir,
                       bool generate_tests, bool generate_tl_internals) {
  std::cout << "Generating PHP classes" << std::endl << "  schema file: '" << tlo_file_path << "'" << std::endl;
  if (!combined2_tl_file.empty()) {
    std::cout << "  combined2.tl file: '" << combined2_tl_file << "'" << std::endl;
  }
  std::cout << "  destination directory: '" << out_php_dir << "'" << std::endl;
  size_t total_classes = 0;
  size_t total_functions = 0;
  size_t total_types = 0;
  size_t anonymous_types = 0;
  size_t unused_and_flatted_types = 0;
  try {
    TlHints hints;
    if (!combined2_tl_file.empty()) {
      hints.load_from_combined2_tl_file(combined2_tl_file);
    }
    auto parsing_result = tlo_parsing::parse_tlo(tlo_file_path, true);
    if (!parsing_result.parsed_schema) {
      throw std::runtime_error{"Error while reading tlo: " + parsing_result.error};
    }
    auto &schema = *parsing_result.parsed_schema;
    total_functions = schema.functions.size();
    total_types = schema.types.size();

    tlo_parsing::replace_anonymous_args(schema);
    assert(schema.types.size() >= total_types);
    anonymous_types = schema.types.size() - total_types;

    tlo_parsing::perform_flat_optimization(schema);
    assert(anonymous_types + total_types >= schema.types.size());
    unused_and_flatted_types = anonymous_types + total_types - schema.types.size();

    tlo_parsing::tl_scheme_final_check(schema);

    total_classes = gen_php_code(schema, hints, out_php_dir, forcibly_overwrite_dir, generate_tests, generate_tl_internals);
  } catch (const std::exception &ex) {
    std::cerr << "Can't convert tlo to PHP classes: " << ex.what() << std::endl;
    return 1;
  }
  std::cout << "  total functions: " << total_functions << std::endl;
  std::cout << "  total types: " << total_types << std::endl;
  std::cout << "  anonymous types: " << anonymous_types << std::endl;
  std::cout << "  unused and flatted types: " << unused_and_flatted_types << std::endl;
  std::cout << "Generated " << total_classes << " PHP classes" << std::endl;
  return 0;
}
} // namespace tl
} // namespace vk

static std::string out_php_dir = ".";
static bool forcibly_overwrite_dir = false;
static bool generate_tests = false;
static bool generate_tl_internals = false;
static std::string combined2_tl_file;

int tl2php_parse_f(int i, const char *) {
  switch (i) {
    case 'f': {
      forcibly_overwrite_dir = true;
      return 0;
    }
    case 't': {
      generate_tests = true;
      return 0;
    }
    case 'c': {
      combined2_tl_file = optarg;
      return 0;
    }
    case 'd': {
      out_php_dir = optarg;
      return 0;
    }
    case 'i': {
      generate_tl_internals = true;
      return 0;
    }
  }
  return -1;
}

void get_bool_option_from_env(bool &option_value, const char *env, bool default_value) {
  if (option_value == default_value) {
    const char *env_value = getenv(env);
    if (env_value == nullptr) {
      return;
    }
    option_value = static_cast<bool>(atoi(env_value));
  }
}

static void get_options_from_env() {
  get_bool_option_from_env(generate_tl_internals, "TL2PHP_GEN_TL_INTERNALS", false);
}

int main(int argc, char *argv[]) {
  static constexpr char version_str[]{"tl2php-0.1.0"};
  init_version_string(version_str);
  option_section_t sections[] = {OPT_GENERIC, OPT_ARRAY_END};
  usage_set_other_args_desc("TLO-FILE");
  init_parse_options(sections);
  remove_parse_option("daemonize");
  parse_option("force", no_argument, 'f', "forcibly overwrite existing directory");
  parse_option("gen-tests", no_argument, 't', "generate tests for produced PHP code");
  parse_option("output-directory", required_argument, 'd', "output directory for PHP code, default is '%s'", out_php_dir.c_str());
  parse_option("comments-file", required_argument, 'c', "specify combined2.tl for generating class comments");
  parse_option("gen-tl-internals", no_argument, 'i', "generate PHP classes for internal tl functions");
  parse_engine_options_long(argc, argv, tl2php_parse_f);
  if (argc < optind + 1) {
    std::cerr << "Please, specify tlo schema file!" << std::endl;
    usage_and_exit();
  }
  get_options_from_env();

  const char *tlo_file_path = argv[optind++];
  return vk::tl::convert_tlo_to_php(tlo_file_path, out_php_dir, combined2_tl_file, forcibly_overwrite_dir, generate_tests, generate_tl_internals);
}
