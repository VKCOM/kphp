// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/tl-classes.h"

#include "common/tlo-parsing/tlo-parsing.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/stage.h"

void TlClasses::load_from(const std::string &tlo_schema, bool generate_tl_internals) {
  auto parsing_result = vk::tl::parse_tlo(tlo_schema.c_str(), true);
  kphp_error_return(parsing_result.parsed_schema,
                    fmt_format("Error while reading tlo: {}", parsing_result.error));

  std::unique_ptr<vk::tl::tl_scheme> scheme = std::move(parsing_result.parsed_schema);
  try {
    vk::tl::replace_anonymous_args(*scheme);
    vk::tl::perform_flat_optimization(*scheme);
    vk::tl::tl_scheme_final_check(*scheme);
  } catch (const std::exception &ex) {
    kphp_error_return(false, ex.what());
  }

  scheme_ = std::move(scheme);
  php_classes_.load_from(*scheme_, generate_tl_internals);
}

