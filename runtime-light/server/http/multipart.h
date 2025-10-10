#include <string_view>

#include "runtime-common/core/runtime-core.h"

namespace kphp::http {

void parse_multipart(const std::string_view &body, const std::string_view &boundary, mixed &v$_POST, mixed &v$_FILES);

std::string_view parse_boundary(const std::string_view &content_type);

} // namespace kphp::http