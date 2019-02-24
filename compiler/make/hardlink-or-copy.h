#pragma once

#include <string>

void hard_link_or_copy(const std::string &from, const std::string &to, bool replace = true);
