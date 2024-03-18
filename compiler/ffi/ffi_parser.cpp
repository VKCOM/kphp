// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/ffi/ffi_parser.h"

#include "compiler/compiler-core.h"
#include "compiler/ffi/c_parser/parsing_driver.h"
#include "compiler/kphp_assert.h"

#include <cstring>

static void log_ffi_parser_stats(const ffi::ParsingDriver::Result &result) {
  if (G->settings().verbosity.get() >= 1) {
    fprintf(stderr, "FFI parser: allocated %ld objects (%ld collected as garbage)\n", static_cast<long>(result.num_allocated),
            static_cast<long>(result.num_deleted));
  }
}

std::string ffi_preprocess_file(const std::string &src, FFIParseResult &parse_result) {
  // this code is definitely not as good as it could be;
  // like in PHP, we don't really handle any preprocessor directives
  // apart from the special FFI_SCOPE and FFI_LIB #defines

  std::string result;
  result.reserve(src.size());
  const char *p = src.c_str();
  const char *end = p + src.size();

  while (p < end) {
    if (*p != '#') {
      result.push_back(*p);
      ++p;
      continue;
    }

    if (std::strncmp(p, "#define FFI_SCOPE \"", strlen("#define FFI_SCOPE \"")) == 0) {
      const char *str_begin = p + strlen("#define FFI_SCOPE \"");
      const char *str_end = strstr(str_begin, "\"\n");
      parse_result.scope = std::string(str_begin, static_cast<int>(str_end - str_begin));
    }
    if (std::strncmp(p, "#define FFI_LIB \"", strlen("#define FFI_LIB \"")) == 0) {
      const char *str_begin = p + strlen("#define FFI_LIB \"");
      const char *str_end = strstr(str_begin, "\"\n");
      parse_result.lib = std::string(str_begin, static_cast<int>(str_end - str_begin));
    }
    if (std::strncmp(p, "#define FFI_STATIC_LIB \"", strlen("#define FFI_STATIC_LIB \"")) == 0) {
      const char *str_begin = p + strlen("#define FFI_STATIC_LIB \"");
      const char *str_end = strstr(str_begin, "\"\n");
      parse_result.static_lib = std::string(str_begin, static_cast<int>(str_end - str_begin));
    }

    // cut the directive text, but preserve the newlines (to keep positions info valid)
    while (*p) {
      if (*p == '\n') {
        result.push_back('\n');
        ++p;
        break;
      }

      if (*p == '\\' && p[1] == '\n') {
        result.push_back('\n');
        p += 2;
      } else {
        ++p;
      }
    }
  }

  return result;
}

static void set_error(FFIParseError &dst, const std::string &src, ffi::ParsingDriver::ParseError &e) {
  dst.message = std::move(e.message);
  dst.line = 1;
  std::string chunk(src.begin(), src.begin() + e.location.begin);
  dst.line += std::count_if(src.begin(), src.begin() + e.location.begin, [](char c) { return c == '\n'; });
}

FFIParseResult ffi_parse_file(const std::string &src, FFITypedefs &typedefs) {
  FFIParseResult result;

  if (src.empty()) {
    return result;
  }

  std::string preprocessed_src = ffi_preprocess_file(src, result);

  ffi::ParsingDriver driver(preprocessed_src, typedefs);

  try {
    auto parse_result = driver.parse();
    log_ffi_parser_stats(parse_result);
    result.set_types(std::move(parse_result.types));
    result.enum_constants = parse_result.enum_constants;
  } catch (ffi::ParsingDriver::ParseError &e) {
    set_error(result.err, preprocessed_src, e);
  }

  return result;
}

std::pair<const FFIType *, FFIParseError> ffi_parse_type(const std::string &type_expr, FFITypedefs &typedefs) {
  FFIType *result_type = nullptr;
  FFIParseError err;

  // to parse an arbitrary type expression, parse it as a part of
  // a top level declaration; function argument grammar is the most
  // suitable thing for what we want here
  std::string src = "void f(" + type_expr + ");\n";
  bool expr_mode = true;
  ffi::ParsingDriver driver(src, typedefs, expr_mode);
  try {
    auto parse_result = driver.parse();
    log_ffi_parser_stats(parse_result);
    const auto &types = parse_result.types;
    kphp_assert(types.size() == 1);
    FFIType *param = types[0]->members[1];
    kphp_assert(param->kind == FFITypeKind::Var);
    result_type = param->members[0];
    delete param;
  } catch (ffi::ParsingDriver::ParseError &e) {
    set_error(err, src, e);
  }

  return {result_type, err};
}
