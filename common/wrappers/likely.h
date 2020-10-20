#pragma once

#ifndef likely
#define likely(x) __builtin_expect ((x),1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect ((x),0)
#endif
