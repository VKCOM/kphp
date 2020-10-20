#ifndef VK_COMMON_ALGORITHMS_H
#define VK_COMMON_ALGORITHMS_H

#include <cassert>
#include <functional>

namespace vk {

#if __cplusplus >= 201703
using std::clamp;
#else
template<class T, class Compare>
__attribute__((warn_unused_result))
constexpr const T &clamp(const T &v, const T &lo, const T &hi, Compare comp) {
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
__attribute__((warn_unused_result))
constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
  return clamp(v, lo, hi, std::less<T>());
}
#endif

} // namespace vk

#endif // VK_COMMON_ALGORITHMS_H
