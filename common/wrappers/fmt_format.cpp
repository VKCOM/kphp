#include "common/wrappers/fmt_format.h"

#if !defined(FMT_STRING) && !defined(FAST_COMPILATION_FMT)
#  pragma message("PLEASE, CONSIDER UPDATING LIBFMT LIBRARY MANUALLY TO ENABLE COMPILE TIME CHECKS")
#endif
