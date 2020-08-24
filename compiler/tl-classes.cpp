#include "compiler/tl-classes.h"

#include "common/tlo-parsing/flat-optimization.h"
#include "common/tlo-parsing/replace-anonymous-args.h"
#include "common/tlo-parsing/tl-scheme-final-check.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/stage.h"

bool TlClasses::new_tl_long{false};

void TlClasses::load_from(const std::string &tlo_schema, bool generate_tl_internals) {
  auto tl_expected_ptr = vk::tl::parse_tlo(tlo_schema.c_str(), true);
  kphp_error_return(tl_expected_ptr.has_value(),
                    fmt_format("Error while reading tlo: {}", tl_expected_ptr.error()));

  std::unique_ptr<vk::tl::tl_scheme> scheme = std::move(std::move(tl_expected_ptr).value());
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

