#pragma once

#include <variant.hpp>

namespace vk {
using ::mpark::variant;
using ::mpark::get;
using ::mpark::get_if;
using ::mpark::holds_alternative;
using ::mpark::visit;
using ::mpark::monostate;

namespace details {

template<class... Fs>
struct overloaded;

template<class F>
struct overloaded<F> : F {
  explicit overloaded(const F &f) :
    F(f) {}
};

template<class F, class... Fs>
struct overloaded<F, Fs...> : overloaded<F>, overloaded<Fs...> {
  explicit overloaded(const F &f, const Fs&... fs) :
    overloaded<F>(f),
    overloaded<Fs...>(fs...) {}
  using overloaded<F>::operator();
  using overloaded<Fs...>::operator();
};

} // namespace details

template<class... F>
auto make_overloaded(F... f) {
  return details::overloaded<F...>(f...);
}
} // namespace vk
