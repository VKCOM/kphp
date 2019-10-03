#pragma once
#include "common/wrappers/fmt_format.h"

char *format(char const *msg, ...) __attribute__ ((format (printf, 1, 2)));
