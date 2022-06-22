// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/utils/string-utils.h"

// perform "blahBlah" to "blah_blah" translation
std::string transform_to_snake_case(vk::string_view origin) noexcept {
  std::string name;
  name.reserve(static_cast<std::size_t>(origin.size() * 1.2));

  for (char c : origin) {
    if (std::isupper(c)) {
      if (!name.empty() && name.back() != '_') {
        name.append(1, '_');
      }
    }
    name.append(1, std::tolower(c));
  }
  return name;
}

// perform "blah_blah" to "blahBlah" translation
std::string transform_to_camel_case(vk::string_view origin) noexcept {
  std::string name;
  name.reserve(origin.size());

  std::size_t i = 0;
  if (i < origin.size() && origin[i] == '_') {
    name.append(1, '_');
    ++i;
  }

  for (; i < origin.size(); ++i) {
    char cur = origin[i];
    char next = origin[i + 1];
    if (cur == '_' && next) {
      name.append(1, std::toupper(next));
      ++i;
    } else {
      name.append(1, cur);
    }
  }

  return name;
}
