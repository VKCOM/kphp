// MPark.Variant
//
// Copyright Michael Park, 2015-2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPARK_VARIANT_HPP
#define MPARK_VARIANT_HPP

/*
   variant synopsis

namespace std {

  // 20.7.2, class template variant
  template <class... Types>
  class variant {
  public:

    // 20.7.2.1, constructors
    constexpr variant() noexcept(see below);
    variant(const variant&);
    variant(variant&&) noexcept(see below);

    template <class T> constexpr variant(T&&) noexcept(see below);

    template <class T, class... Args>
    constexpr explicit variant(in_place_type_t<T>, Args&&...);

    template <class T, class U, class... Args>
    constexpr explicit variant(
        in_place_type_t<T>, initializer_list<U>, Args&&...);

    template <size_t I, class... Args>
    constexpr explicit variant(in_place_index_t<I>, Args&&...);

    template <size_t I, class U, class... Args>
    constexpr explicit variant(
        in_place_index_t<I>, initializer_list<U>, Args&&...);

    // 20.7.2.2, destructor
    ~variant();

    // 20.7.2.3, assignment
    variant& operator=(const variant&);
    variant& operator=(variant&&) noexcept(see below);

    template <class T> variant& operator=(T&&) noexcept(see below);

    // 20.7.2.4, modifiers
    template <class T, class... Args>
    void emplace(Args&&...);

    template <class T, class U, class... Args>
    void emplace(initializer_list<U>, Args&&...);

    template <size_t I, class... Args>
    void emplace(Args&&...);

    template <size_t I, class U, class...  Args>
    void emplace(initializer_list<U>, Args&&...);

    // 20.7.2.5, value status
    constexpr bool valueless_by_exception() const noexcept;
    constexpr size_t index() const noexcept;

    // 20.7.2.6, swap
    void swap(variant&) noexcept(see below);
  };

  // 20.7.3, variant helper classes
  template <class T> struct variant_size; // undefined

  template <class T>
  constexpr size_t variant_size_v = variant_size<T>::value;

  template <class T> struct variant_size<const T>;
  template <class T> struct variant_size<volatile T>;
  template <class T> struct variant_size<const volatile T>;

  template <class... Types>
  struct variant_size<variant<Types...>>;

  template <size_t I, class T> struct variant_alternative; // undefined

  template <size_t I, class T>
  using variant_alternative_t = typename variant_alternative<I, T>::type;

  template <size_t I, class T> struct variant_alternative<I, const T>;
  template <size_t I, class T> struct variant_alternative<I, volatile T>;
  template <size_t I, class T> struct variant_alternative<I, const volatile T>;

  template <size_t I, class... Types>
  struct variant_alternative<I, variant<Types...>>;

  constexpr size_t variant_npos = -1;

  // 20.7.4, value access
  template <class T, class... Types>
  constexpr bool holds_alternative(const variant<Types...>&) noexcept;

  template <size_t I, class... Types>
  constexpr variant_alternative_t<I, variant<Types...>>&
  get(variant<Types...>&);

  template <size_t I, class... Types>
  constexpr variant_alternative_t<I, variant<Types...>>&&
  get(variant<Types...>&&);

  template <size_t I, class... Types>
  constexpr variant_alternative_t<I, variant<Types...>> const&
  get(const variant<Types...>&);

  template <size_t I, class... Types>
  constexpr variant_alternative_t<I, variant<Types...>> const&&
  get(const variant<Types...>&&);

  template <class T, class...  Types>
  constexpr T& get(variant<Types...>&);

  template <class T, class... Types>
  constexpr T&& get(variant<Types...>&&);

  template <class T, class... Types>
  constexpr const T& get(const variant<Types...>&);

  template <class T, class... Types>
  constexpr const T&& get(const variant<Types...>&&);

  template <size_t I, class... Types>
  constexpr add_pointer_t<variant_alternative_t<I, variant<Types...>>>
  get_if(variant<Types...>*) noexcept;

  template <size_t I, class... Types>
  constexpr add_pointer_t<const variant_alternative_t<I, variant<Types...>>>
  get_if(const variant<Types...>*) noexcept;

  template <class T, class... Types>
  constexpr add_pointer_t<T>
  get_if(variant<Types...>*) noexcept;

  template <class T, class... Types>
  constexpr add_pointer_t<const T>
  get_if(const variant<Types...>*) noexcept;

  // 20.7.5, relational operators
  template <class... Types>
  constexpr bool operator==(const variant<Types...>&, const variant<Types...>&);

  template <class... Types>
  constexpr bool operator!=(const variant<Types...>&, const variant<Types...>&);

  template <class... Types>
  constexpr bool operator<(const variant<Types...>&, const variant<Types...>&);

  template <class... Types>
  constexpr bool operator>(const variant<Types...>&, const variant<Types...>&);

  template <class... Types>
  constexpr bool operator<=(const variant<Types...>&, const variant<Types...>&);

  template <class... Types>
  constexpr bool operator>=(const variant<Types...>&, const variant<Types...>&);

  // 20.7.6, visitation
  template <class Visitor, class... Variants>
  constexpr see below visit(Visitor&&, Variants&&...);

  // 20.7.7, class monostate
  struct monostate;

  // 20.7.8, monostate relational operators
  constexpr bool operator<(monostate, monostate) noexcept;
  constexpr bool operator>(monostate, monostate) noexcept;
  constexpr bool operator<=(monostate, monostate) noexcept;
  constexpr bool operator>=(monostate, monostate) noexcept;
  constexpr bool operator==(monostate, monostate) noexcept;
  constexpr bool operator!=(monostate, monostate) noexcept;

  // 20.7.9, specialized algorithms
  template <class... Types>
  void swap(variant<Types...>&, variant<Types...>&) noexcept(see below);

  // 20.7.10, class bad_variant_access
  class bad_variant_access;

  // 20.7.11, hash support
  template <class T> struct hash;
  template <class... Types> struct hash<variant<Types...>>;
  template <> struct hash<monostate>;

} // namespace std

*/

#include <array>
#include <cstddef>
#include <exception>
#include <functional>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

// MPark.Variant
//
// Copyright Michael Park, 2015-2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPARK_IN_PLACE_HPP
#define MPARK_IN_PLACE_HPP

#include <cstddef>

namespace mpark {

  struct in_place_t {
    explicit in_place_t() = default;
  };

  constexpr in_place_t in_place{};

  template <std::size_t I>
  struct in_place_index_t {
    explicit in_place_index_t() = default;
  };

  template <std::size_t I>
  constexpr in_place_index_t<I> in_place_index{};

  template <typename T>
  struct in_place_type_t {
    explicit in_place_type_t() = default;
  };

  template <typename T>
  constexpr in_place_type_t<T> in_place_type{};

}  // namespace mpark

#endif  // MPARK_IN_PLACE_HPP

// MPark.Variant
//
// Copyright Michael Park, 2015-2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPARK_LIB_HPP
#define MPARK_LIB_HPP

#include <memory>
#include <type_traits>
#include <utility>

// MPark.Variant
//
// Copyright Michael Park, 2015-2016
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MPARK_CONFIG_HPP
#define MPARK_CONFIG_HPP

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_addressof) || __GNUC__ >= 7
#define MPARK_BUILTIN_ADDRESSOF
#endif

#if __has_builtin(__type_pack_element)
#define MPARK_TYPE_PACK_ELEMENT
#endif

#endif  // MPARK_CONFIG_HPP


namespace mpark {
  namespace lib {

    inline namespace cpp17 {

      // <type_traits>
      template <bool B>
      using bool_constant = std::integral_constant<bool, B>;

      template <typename...>
      using void_t = void;

      template <typename F, typename = void>
      struct is_callable : std::false_type {};

      template <typename F>
      struct is_callable<F, void_t<std::result_of_t<F>>> : std::true_type {};

      namespace detail {
        namespace swappable {

          using std::swap;

          template <typename T>
          struct is_swappable_impl {
            private:
            template <typename U,
                      typename = decltype(swap(std::declval<U &>(),
                                               std::declval<U &>()))>
            inline static std::true_type test(int);

            template <typename U>
            inline static std::false_type test(...);

            public:
            using type = decltype(test<T>(0));
          };

          template <typename T>
          using is_swappable = typename is_swappable_impl<T>::type;

          template <typename T, bool = is_swappable<T>::value>
          struct is_nothrow_swappable
              : bool_constant<noexcept(
                    swap(std::declval<T &>(), std::declval<T &>()))> {};

          template <typename T>
          struct is_nothrow_swappable<T, false> : std::false_type {};

        }  // namespace swappable
      }  // namespace detail

      template <typename T>
      using is_swappable = detail::swappable::is_swappable<T>;

      template <typename T>
      using is_nothrow_swappable = detail::swappable::is_nothrow_swappable<T>;

      // <functional>
#define RETURN(...) -> decltype(__VA_ARGS__) { return __VA_ARGS__; }

      template <typename F, typename... As>
      inline constexpr auto invoke(F &&f, As &&... as)
          RETURN(std::forward<F>(f)(std::forward<As>(as)...))

      template <typename B, typename T, typename D>
      inline constexpr auto invoke(T B::*pmv, D &&d)
          RETURN(std::forward<D>(d).*pmv)

      template <typename Pmv, typename Ptr>
      inline constexpr auto invoke(Pmv pmv, Ptr &&ptr)
          RETURN((*std::forward<Ptr>(ptr)).*pmv)

      template <typename B, typename T, typename D, typename... As>
      inline constexpr auto invoke(T B::*pmf, D &&d, As &&... as)
          RETURN((std::forward<D>(d).*pmf)(std::forward<As>(as)...))

      template <typename Pmf, typename Ptr, typename... As>
      inline constexpr auto invoke(Pmf pmf, Ptr &&ptr, As &&... as)
          RETURN(((*std::forward<Ptr>(ptr)).*pmf)(std::forward<As>(as)...))

#undef RETURN

      // <memory>
#ifdef MPARK_BUILTIN_ADDRESSOF
      template <typename T>
      inline constexpr T *addressof(T &arg) {
        return __builtin_addressof(arg);
      }
#else
      namespace detail {

        namespace has_addressof_impl {

          struct fail;

          template <typename T>
          inline fail operator&(T &&);

          template <typename T>
          inline static constexpr bool impl() {
            return (std::is_class<T>::value || std::is_union<T>::value) &&
                   !std::is_same<decltype(&std::declval<T &>()), fail>::value;
          }

        }  // namespace has_addressof_impl

        template <typename T>
        using has_addressof = bool_constant<has_addressof_impl::impl<T>()>;

      }  // namespace detail

      template <typename T,
                std::enable_if_t<detail::has_addressof<T>::value, int> = 0>
      inline T *addressof(T &arg) {
        return std::addressof(arg);
      }

      template <typename T,
                std::enable_if_t<!detail::has_addressof<T>::value, int> = 0>
      inline constexpr T *addressof(T &arg) {
        return &arg;
      }
#endif

      template <typename T>
      inline const T *addressof(const T &&) = delete;

    }  // namespace cpp17

    template <typename T>
    struct identity {
      using type = T;
    };

#ifdef MPARK_TYPE_PACK_ELEMENT
    template <std::size_t I, typename... Ts>
    using type_pack_element_t = __type_pack_element<I, Ts...>;
#else
    template <std::size_t I, typename... Ts>
    struct type_pack_element_impl {
      private:
      template <std::size_t, typename T>
      struct indexed_type {
        using type = T;
      };

      template <std::size_t... Is>
      inline static auto make_set(std::index_sequence<Is...>) {
        struct set : indexed_type<Is, Ts>... {};
        return set{};
      }

      template <typename T>
      inline static std::enable_if<true, T> impl(indexed_type<I, T>);

      inline static std::enable_if<false, void> impl(...);

      public:
      using type = decltype(impl(make_set(std::index_sequence_for<Ts...>{})));
    };

    template <std::size_t I, typename... Ts>
    using type_pack_element = typename type_pack_element_impl<I, Ts...>::type;

    template <std::size_t I, typename... Ts>
    using type_pack_element_t = typename type_pack_element<I, Ts...>::type;
#endif

  }  // namespace lib
}  // namespace mpark

#endif  // MPARK_LIB_HPP


namespace mpark {

  class bad_variant_access : public std::exception {
    public:
    virtual const char *what() const noexcept { return "bad_variant_access"; }
  };

  template <typename... Ts>
  class variant;

  template <typename T>
  struct variant_size;

  template <typename T>
  constexpr std::size_t variant_size_v = variant_size<T>::value;

  template <typename T>
  struct variant_size<const T> : variant_size<T> {};

  template <typename T>
  struct variant_size<volatile T> : variant_size<T> {};

  template <typename T>
  struct variant_size<const volatile T> : variant_size<T> {};

  template <typename... Ts>
  struct variant_size<variant<Ts...>>
      : std::integral_constant<std::size_t, sizeof...(Ts)> {};

  template <std::size_t I, typename T>
  struct variant_alternative;

  template <std::size_t I, typename T>
  using variant_alternative_t = typename variant_alternative<I, T>::type;

  template <std::size_t I, typename T>
  struct variant_alternative<I, const T>
      : std::add_const<variant_alternative_t<I, T>> {};

  template <std::size_t I, typename T>
  struct variant_alternative<I, volatile T>
      : std::add_volatile<variant_alternative_t<I, T>> {};

  template <std::size_t I, typename T>
  struct variant_alternative<I, const volatile T>
      : std::add_cv<variant_alternative_t<I, T>> {};

  template <std::size_t I, typename... Ts>
  struct variant_alternative<I, variant<Ts...>> {
    static_assert(I < sizeof...(Ts),
                  "`variant_alternative` index out of range.");
    using type = lib::type_pack_element_t<I, Ts...>;
  };

  constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

  namespace detail {

    inline constexpr bool all(std::initializer_list<bool> bs) {
      for (bool b : bs) {
        if (!b) {
          return false;
        }
      }
      return true;
    }

    constexpr std::size_t not_found = static_cast<std::size_t>(-1);
    constexpr std::size_t ambiguous = static_cast<std::size_t>(-2);

    template <typename T, typename... Ts>
    inline constexpr std::size_t find_index() {
      constexpr bool matches[] = {std::is_same<T, Ts>::value...};
      std::size_t result = not_found;
      for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
        if (matches[i]) {
          if (result != not_found) {
            return ambiguous;
          }
          result = i;
        }
      }
      return result;
    }

    template <std::size_t I>
    using find_index_sfinae_impl =
        std::enable_if_t<I != not_found && I != ambiguous,
                         std::integral_constant<std::size_t, I>>;

    template <typename T, typename... Ts>
    using find_index_sfinae = find_index_sfinae_impl<find_index<T, Ts...>()>;

    template <std::size_t I>
    struct find_index_checked_impl : std::integral_constant<std::size_t, I> {
      static_assert(I != not_found, "the specified type is not found.");
      static_assert(I != ambiguous, "the specified type is ambiguous.");
    };

    template <typename T, typename... Ts>
    using find_index_checked = find_index_checked_impl<find_index<T, Ts...>()>;

    struct valueless_t {};

    enum class Trait { TriviallyAvailable, Available, Unavailable };

    template <typename T,
              template <typename> class IsTriviallyAvailable,
              template <typename> class IsAvailable>
    inline constexpr Trait trait() {
      return IsTriviallyAvailable<T>::value
                 ? Trait::TriviallyAvailable
                 : IsAvailable<T>::value ? Trait::Available
                                         : Trait::Unavailable;
    }

    inline constexpr Trait common_trait(std::initializer_list<Trait> traits) {
      Trait result = Trait::TriviallyAvailable;
      for (Trait t : traits) {
        if (static_cast<int>(t) > static_cast<int>(result)) {
          result = t;
        }
      }
      return result;
    }

    template <typename... Ts>
    struct traits {
      static constexpr Trait copy_constructible_trait =
          common_trait({trait<Ts,
                              std::is_trivially_copy_constructible,
                              std::is_copy_constructible>()...});

      static constexpr Trait move_constructible_trait =
          common_trait({trait<Ts,
                              std::is_trivially_move_constructible,
                              std::is_move_constructible>()...});

      static constexpr Trait copy_assignable_trait =
          common_trait({copy_constructible_trait,
                        move_constructible_trait,
                        trait<Ts,
                              std::is_trivially_copy_assignable,
                              std::is_copy_assignable>()...});

      static constexpr Trait move_assignable_trait =
          common_trait({move_constructible_trait,
                        trait<Ts,
                              std::is_trivially_move_assignable,
                              std::is_move_assignable>()...});

      static constexpr Trait destructible_trait =
          common_trait({trait<Ts,
                              std::is_trivially_destructible,
                              std::is_destructible>()...});
    };

    namespace access {

      struct recursive_union {
        template <typename V>
        inline static constexpr auto &&get_alt(V &&v, in_place_index_t<0>) {
          return std::forward<V>(v).head_;
        }

        template <typename V, std::size_t I>
        inline static constexpr auto &&get_alt(V &&v, in_place_index_t<I>) {
          return get_alt(std::forward<V>(v).tail_, in_place_index<I - 1>);
        }
      };

      struct base {
        template <std::size_t I, typename V>
        inline static constexpr auto &&get_alt(V &&v) {
          return recursive_union::get_alt(std::forward<V>(v).data_,
                                          in_place_index<I>);
        }
      };

      struct variant {
        template <std::size_t I, typename V>
        inline static constexpr auto &&get_alt(V &&v) {
          return base::get_alt<I>(std::forward<V>(v).impl_);
        }
      };

    }  // namespace access

    namespace visitation {

      struct base {
        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_alt_at(std::size_t index,
                                                            Visitor &&visitor,
                                                            Vs &&... vs) {
          constexpr auto fdiagonal =
              make_fdiagonal<Visitor &&,
                             decltype(std::forward<Vs>(vs).as_base())...>();
          return fdiagonal[index](std::forward<Visitor>(visitor),
                                  std::forward<Vs>(vs).as_base()...);
        }

        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_alt(Visitor &&visitor,
                                                         Vs &&... vs) {
          constexpr auto fmatrix =
              make_fmatrix<Visitor &&,
                           decltype(std::forward<Vs>(vs).as_base())...>();
          const std::size_t indices[] = {vs.index()...};
          return at(fmatrix, indices)(std::forward<Visitor>(visitor),
                                      std::forward<Vs>(vs).as_base()...);
        }

        private:
        template <typename T>
        inline static constexpr const T &at_impl(const T &elem,
                                                 const std::size_t *) {
          return elem;
        }

        template <typename T, std::size_t N>
        inline static constexpr auto &&at_impl(const std::array<T, N> &elems,
                                               const std::size_t *index) {
          return at_impl(elems[*index], index + 1);
        }

        template <typename T, std::size_t N, std::size_t I>
        inline static constexpr auto &&at(const std::array<T, N> &elems,
                                          const std::size_t (&indices)[I]) {
          return at_impl(elems, indices);
        }

        template <typename F, typename... Fs>
        inline static constexpr void std_visit_visitor_return_type_check() {
          static_assert(all({std::is_same<F, Fs>::value...}),
                        "`std::visit` requires the visitor to have a single "
                        "return type.");
        }

        template <typename... Fs>
        inline static constexpr auto make_farray(Fs &&... fs) {
          std_visit_visitor_return_type_check<std::decay_t<Fs>...>();
          using result = std::array<std::common_type_t<std::decay_t<Fs>...>,
                                    sizeof...(Fs)>;
          return result{{std::forward<Fs>(fs)...}};
        }

        template <std::size_t... Is>
        struct dispatcher {
          template <typename F, typename... Vs>
          inline static constexpr decltype(auto) dispatch(F f, Vs... vs) {
            return lib::invoke(
                static_cast<F>(f),
                access::base::get_alt<Is>(static_cast<Vs>(vs))...);
          }
        };

        template <typename F, typename... Vs, std::size_t... Is>
        inline static constexpr auto make_dispatch(std::index_sequence<Is...>) {
          return &dispatcher<Is...>::template dispatch<F, Vs...>;
        }

        template <std::size_t I, typename F, typename... Vs>
        inline static constexpr auto make_fdiagonal_impl() {
          return make_dispatch<F, Vs...>(
              std::index_sequence<(lib::identity<Vs>{}, I)...>{});
        }

        template <typename F, typename... Vs, std::size_t... Is>
        inline static constexpr auto make_fdiagonal_impl(
            std::index_sequence<Is...>) {
          return base::make_farray(make_fdiagonal_impl<Is, F, Vs...>()...);
        }

        template <typename F, typename V, typename... Vs>
        inline static constexpr auto make_fdiagonal() {
          constexpr std::size_t N = std::decay_t<V>::size();
          static_assert(all({(N == std::decay_t<Vs>::size())...}),
                        "all of the variants must be the same size.");
          return make_fdiagonal_impl<F, V, Vs...>(
              std::make_index_sequence<N>{});
        }

        template <typename F, typename... Vs, std::size_t... Is>
        inline static constexpr auto make_fmatrix_impl(
            std::index_sequence<Is...> is) {
          return make_dispatch<F, Vs...>(is);
        }

        template <typename F,
                  typename... Vs,
                  std::size_t... Is,
                  std::size_t... Js,
                  typename... Ls>
        inline static constexpr auto make_fmatrix_impl(
            std::index_sequence<Is...>, std::index_sequence<Js...>, Ls... ls) {
          return base::make_farray(make_fmatrix_impl<F, Vs...>(
              std::index_sequence<Is..., Js>{}, ls...)...);
        }

        template <typename F, typename... Vs>
        inline static constexpr auto make_fmatrix() {
          return make_fmatrix_impl<F, Vs...>(
              std::index_sequence<>{},
              std::make_index_sequence<std::decay_t<Vs>::size()>{}...);
        }
      };

      struct variant {
        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_alt_at(std::size_t index,
                                                            Visitor &&visitor,
                                                            Vs &&... vs) {
          return base::visit_alt_at(index,
                                    std::forward<Visitor>(visitor),
                                    std::forward<Vs>(vs).impl_...);
        }

        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_alt(Visitor &&visitor,
                                                         Vs &&... vs) {
          return base::visit_alt(std::forward<Visitor>(visitor),
                                 std::forward<Vs>(vs).impl_...);
        }

        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_value_at(std::size_t index,
                                                              Visitor &&visitor,
                                                              Vs &&... vs) {
          return visit_alt_at(
              index,
              make_value_visitor(std::forward<Visitor>(visitor)),
              std::forward<Vs>(vs)...);
        }

        template <typename Visitor, typename... Vs>
        inline static constexpr decltype(auto) visit_value(Visitor &&visitor,
                                                           Vs &&... vs) {
          return visit_alt(make_value_visitor(std::forward<Visitor>(visitor)),
                           std::forward<Vs>(vs)...);
        }

        private:
        template <typename Visitor, typename... Values>
        inline static constexpr void std_visit_exhaustive_visitor_check() {
          static_assert(lib::is_callable<Visitor(Values...)>::value,
                        "`std::visit` requires the visitor to be exhaustive.");
        }

        template <typename Visitor>
        struct value_visitor {
          template <typename... Alts>
          inline constexpr decltype(auto) operator()(Alts &&... alts) const {
            std_visit_exhaustive_visitor_check<
                Visitor,
                decltype(std::forward<Alts>(alts).value_)...>();
            return lib::invoke(std::forward<Visitor>(visitor_),
                               std::forward<Alts>(alts).value_...);
          }
          Visitor &&visitor_;
        };

        template <typename Visitor>
        inline static constexpr auto make_value_visitor(Visitor &&visitor) {
          return value_visitor<Visitor>{std::forward<Visitor>(visitor)};
        }
      };

    }  // namespace visitation

    template <std::size_t Index, typename T>
    struct alt {
      using value_type = T;

      template <typename... Args>
      inline explicit constexpr alt(in_place_t, Args &&... args)
          : value_(std::forward<Args>(args)...) {}

      value_type value_;
    };

    template <Trait DestructibleTrait, std::size_t Index, typename... Ts>
    union recursive_union;

    template <Trait DestructibleTrait, std::size_t Index>
    union recursive_union<DestructibleTrait, Index> {};

#define MPARK_VARIANT_RECURSIVE_UNION(destructible_trait, destructor)  \
  template <std::size_t Index, typename T, typename... Ts>             \
  union recursive_union<destructible_trait, Index, T, Ts...> {         \
    public:                                                            \
    inline explicit constexpr recursive_union(valueless_t) noexcept    \
        : dummy_{} {}                                                  \
                                                                       \
    template <typename... Args>                                        \
    inline explicit constexpr recursive_union(in_place_index_t<0>,     \
                                              Args &&... args)         \
        : head_(in_place, std::forward<Args>(args)...) {}              \
                                                                       \
    template <std::size_t I, typename... Args>                         \
    inline explicit constexpr recursive_union(in_place_index_t<I>,     \
                                              Args &&... args)         \
        : tail_(in_place_index<I - 1>, std::forward<Args>(args)...) {} \
                                                                       \
    recursive_union(const recursive_union &) = default;                \
    recursive_union(recursive_union &&) = default;                     \
                                                                       \
    destructor                                                         \
                                                                       \
    recursive_union &operator=(const recursive_union &) = default;     \
    recursive_union &operator=(recursive_union &&) = default;          \
                                                                       \
    private:                                                           \
    char dummy_;                                                       \
    alt<Index, T> head_;                                               \
    recursive_union<destructible_trait, Index + 1, Ts...> tail_;       \
                                                                       \
    friend struct access::recursive_union;                             \
  }

    MPARK_VARIANT_RECURSIVE_UNION(Trait::TriviallyAvailable,
                                  ~recursive_union() = default;);
    MPARK_VARIANT_RECURSIVE_UNION(Trait::Available,
                                  ~recursive_union() {});
    MPARK_VARIANT_RECURSIVE_UNION(Trait::Unavailable,
                                  ~recursive_union() = delete;);

#undef _MPARK_VARIANT_UNION

    template <Trait DestructibleTrait, typename... Ts>
    class base {
      public:
      inline explicit constexpr base(valueless_t tag) noexcept
          : data_(tag), index_(-1) {}

      template <std::size_t I, typename... Args>
      inline explicit constexpr base(in_place_index_t<I>, Args &&... args)
          : data_(in_place_index<I>, std::forward<Args>(args)...), index_(I) {}

      inline constexpr bool valueless_by_exception() const noexcept {
        return index() == variant_npos;
      }

      inline constexpr std::size_t index() const noexcept {
        return index_ == static_cast<unsigned int>(-1) ? variant_npos : index_;
      }

      protected:
      inline constexpr base &as_base() & { return *this; }
      inline constexpr base &&as_base() && { return std::move(*this); }
      inline constexpr const base &as_base() const & { return *this; }
      inline constexpr const base &&as_base() const && { return std::move(*this); }

      inline static constexpr std::size_t size() { return sizeof...(Ts); }

      recursive_union<DestructibleTrait, 0, Ts...> data_;
      unsigned int index_;

      friend struct access::base;
      friend struct visitation::base;
    };

    template <typename Traits, Trait = Traits::destructible_trait>
    class destructor;

#define MPARK_VARIANT_DESTRUCTOR(destructible_trait, definition, destroy) \
  template <typename... Ts>                                               \
  class destructor<traits<Ts...>, destructible_trait>                     \
      : public base<destructible_trait, Ts...> {                          \
    using super = base<destructible_trait, Ts...>;                        \
                                                                          \
    public:                                                               \
    using super::super;                                                   \
    using super::operator=;                                               \
                                                                          \
    destructor(const destructor &) = default;                             \
    destructor(destructor &&) = default;                                  \
    definition                                                            \
    destructor &operator=(const destructor &) = default;                  \
    destructor &operator=(destructor &&) = default;                       \
                                                                          \
    protected:                                                            \
    inline destroy                                                        \
  }

    MPARK_VARIANT_DESTRUCTOR(Trait::TriviallyAvailable,
                             ~destructor() = default;,
                             void destroy() noexcept { this->index_ = -1; });

    MPARK_VARIANT_DESTRUCTOR(Trait::Available,
                             ~destructor() { destroy(); },
                             void destroy() noexcept {
                               if (!this->valueless_by_exception()) {
                                 visitation::base::visit_alt(
                                     [](auto &alt) noexcept {
                                       using alt_type =
                                           std::decay_t<decltype(alt)>;
                                       alt.~alt_type();
                                     },
                                     *this);
                               }
                               this->index_ = -1;
                             });

    MPARK_VARIANT_DESTRUCTOR(Trait::Unavailable,
                             ~destructor() = delete;,
                             void destroy() noexcept = delete;);

#undef MPARK_VARIANT_DESTRUCTOR

    template <typename Traits>
    class constructor : public destructor<Traits> {
      using super = destructor<Traits>;

      public:
      using super::super;
      using super::operator=;

      protected:
      template <std::size_t I, typename T, typename... Args>
      inline static void construct_alt(alt<I, T> &a, Args &&... args) {
        ::new (static_cast<void *>(lib::addressof(a)))
            alt<I, T>(in_place, std::forward<Args>(args)...);
      }

      template <typename Rhs>
      inline static void generic_construct(constructor &lhs, Rhs &&rhs) {
        lhs.destroy();
        if (!rhs.valueless_by_exception()) {
          visitation::base::visit_alt_at(
              rhs.index(),
              [](auto &lhs_alt, auto &&rhs_alt) {
                constructor::construct_alt(
                    lhs_alt, std::forward<decltype(rhs_alt)>(rhs_alt).value_);
              },
              lhs,
              std::forward<Rhs>(rhs));
          lhs.index_ = rhs.index();
        }
      }
    };

    template <typename Traits, Trait = Traits::move_constructible_trait>
    class move_constructor;

#define MPARK_VARIANT_MOVE_CONSTRUCTOR(move_constructible_trait, definition) \
  template <typename... Ts>                                                  \
  class move_constructor<traits<Ts...>, move_constructible_trait>            \
      : public constructor<traits<Ts...>> {                                  \
    using super = constructor<traits<Ts...>>;                                \
                                                                             \
    public:                                                                  \
    using super::super;                                                      \
    using super::operator=;                                                  \
                                                                             \
    move_constructor(const move_constructor &) = default;                    \
    definition                                                               \
    ~move_constructor() = default;                                           \
    move_constructor &operator=(const move_constructor &) = default;         \
    move_constructor &operator=(move_constructor &&) = default;              \
  }

    MPARK_VARIANT_MOVE_CONSTRUCTOR(
        Trait::TriviallyAvailable,
        move_constructor(move_constructor &&that) = default;);

    MPARK_VARIANT_MOVE_CONSTRUCTOR(
        Trait::Available,
        move_constructor(move_constructor &&that) noexcept(
            all({std::is_nothrow_move_constructible<Ts>::value...}))
            : move_constructor(valueless_t{}) {
          this->generic_construct(*this, std::move(that));
        });

    MPARK_VARIANT_MOVE_CONSTRUCTOR(
        Trait::Unavailable,
        move_constructor(move_constructor &&) = delete;);

#undef MPARK_VARIANT_MOVE_CONSTRUCTOR

    template <typename Traits, Trait = Traits::copy_constructible_trait>
    class copy_constructor;

#define MPARK_VARIANT_COPY_CONSTRUCTOR(copy_constructible_trait, definition) \
  template <typename... Ts>                                                  \
  class copy_constructor<traits<Ts...>, copy_constructible_trait>            \
      : public move_constructor<traits<Ts...>> {                             \
    using super = move_constructor<traits<Ts...>>;                           \
                                                                             \
    public:                                                                  \
    using super::super;                                                      \
    using super::operator=;                                                  \
                                                                             \
    definition                                                               \
    copy_constructor(copy_constructor &&) = default;                         \
    ~copy_constructor() = default;                                           \
    copy_constructor &operator=(const copy_constructor &) = default;         \
    copy_constructor &operator=(copy_constructor &&) = default;              \
  }

    MPARK_VARIANT_COPY_CONSTRUCTOR(
        Trait::TriviallyAvailable,
        copy_constructor(const copy_constructor &that) = default;);

    MPARK_VARIANT_COPY_CONSTRUCTOR(
        Trait::Available,
        copy_constructor(const copy_constructor &that)
            : copy_constructor(valueless_t{}) {
          this->generic_construct(*this, that);
        });

    MPARK_VARIANT_COPY_CONSTRUCTOR(
        Trait::Unavailable,
        copy_constructor(const copy_constructor &) = delete;);

#undef MPARK_VARIANT_COPY_CONSTRUCTOR

    template <typename Traits>
    class assignment : public copy_constructor<Traits> {
      using super = copy_constructor<Traits>;

      public:
      using super::super;
      using super::operator=;

      template <std::size_t I, typename... Args>
      inline void emplace(Args &&... args) {
        this->destroy();
        this->construct_alt(access::base::get_alt<I>(*this),
                            std::forward<Args>(args)...);
        this->index_ = I;
      }

      protected:
      template <bool CopyAssign, std::size_t I, typename T, typename Arg>
      inline void assign_alt(alt<I, T> &a,
                             Arg &&arg,
                             lib::bool_constant<CopyAssign> tag) {
        if (this->index() == I) {
          a.value_ = std::forward<Arg>(arg);
        } else {
          struct {
            void operator()(std::true_type) const {
              this_->emplace<I>(T(std::forward<Arg>(arg_)));
            }
            void operator()(std::false_type) const {
              this_->emplace<I>(std::forward<Arg>(arg_));
            }
            assignment *this_;
            Arg &&arg_;
          } impl{this, std::forward<Arg>(arg)};
          impl(tag);
        }
      }

      template <typename That>
      inline void generic_assign(That &&that) {
        if (this->valueless_by_exception() && that.valueless_by_exception()) {
          // do nothing.
        } else if (that.valueless_by_exception()) {
          this->destroy();
        } else {
          visitation::base::visit_alt_at(
              that.index(),
              [this](auto &this_alt, auto &&that_alt) {
                this->assign_alt(
                    this_alt,
                    std::forward<decltype(that_alt)>(that_alt).value_,
                    std::is_lvalue_reference<That>{});
              },
              *this,
              std::forward<That>(that));
        }
      }
    };

    template <typename Traits, Trait = Traits::move_assignable_trait>
    class move_assignment;

#define MPARK_VARIANT_MOVE_ASSIGNMENT(move_assignable_trait, definition) \
  template <typename... Ts>                                              \
  class move_assignment<traits<Ts...>, move_assignable_trait>            \
      : public assignment<traits<Ts...>> {                               \
    using super = assignment<traits<Ts...>>;                             \
                                                                         \
    public:                                                              \
    using super::super;                                                  \
    using super::operator=;                                              \
                                                                         \
    move_assignment(const move_assignment &) = default;                  \
    move_assignment(move_assignment &&) = default;                       \
    ~move_assignment() = default;                                        \
    move_assignment &operator=(const move_assignment &) = default;       \
    definition                                                           \
  }

    MPARK_VARIANT_MOVE_ASSIGNMENT(
        Trait::TriviallyAvailable,
        move_assignment &operator=(move_assignment &&that) = default;);

    MPARK_VARIANT_MOVE_ASSIGNMENT(
        Trait::Available,
        move_assignment &
        operator=(move_assignment &&that) noexcept(
            all({std::is_nothrow_move_constructible<Ts>::value &&
                 std::is_nothrow_move_assignable<Ts>::value...})) {
          this->generic_assign(std::move(that));
          return *this;
        });

    MPARK_VARIANT_MOVE_ASSIGNMENT(
        Trait::Unavailable,
        move_assignment &operator=(move_assignment &&) = delete;);

#undef MPARK_VARIANT_MOVE_ASSIGNMENT

    template <typename Traits, Trait = Traits::copy_assignable_trait>
    class copy_assignment;

#define MPARK_VARIANT_COPY_ASSIGNMENT(copy_assignable_trait, definition) \
  template <typename... Ts>                                              \
  class copy_assignment<traits<Ts...>, copy_assignable_trait>            \
      : public move_assignment<traits<Ts...>> {                          \
    using super = move_assignment<traits<Ts...>>;                        \
                                                                         \
    public:                                                              \
    using super::super;                                                  \
    using super::operator=;                                              \
                                                                         \
    copy_assignment(const copy_assignment &) = default;                  \
    copy_assignment(copy_assignment &&) = default;                       \
    ~copy_assignment() = default;                                        \
    definition                                                           \
    copy_assignment &operator=(copy_assignment &&) = default;            \
  }

    MPARK_VARIANT_COPY_ASSIGNMENT(
        Trait::TriviallyAvailable,
        copy_assignment &operator=(const copy_assignment &that) = default;);

    MPARK_VARIANT_COPY_ASSIGNMENT(
        Trait::Available,
        copy_assignment &operator=(const copy_assignment &that) {
          this->generic_assign(that);
          return *this;
        });

    MPARK_VARIANT_COPY_ASSIGNMENT(
        Trait::Unavailable,
        copy_assignment &operator=(const copy_assignment &) = delete;);

#undef MPARK_VARIANT_COPY_ASSIGNMENT

    template <typename... Ts>
    class impl : public copy_assignment<traits<Ts...>> {
      using super = copy_assignment<traits<Ts...>>;

      public:
      using super::super;
      using super::operator=;

      template <std::size_t I, typename Arg>
      inline void assign(Arg &&arg) {
        this->assign_alt(access::base::get_alt<I>(*this),
                         std::forward<Arg>(arg),
                         std::false_type{});
      }

      inline void swap(impl &that) {
        if (this->valueless_by_exception() && that.valueless_by_exception()) {
          // do nothing.
        } else if (this->index() == that.index()) {
          visitation::base::visit_alt_at(this->index(),
                                         [](auto &this_alt, auto &that_alt) {
                                           using std::swap;
                                           swap(this_alt.value_,
                                                that_alt.value_);
                                         },
                                         *this,
                                         that);
        } else {
          impl *lhs = this;
          impl *rhs = lib::addressof(that);
          if (lhs->move_nothrow() && !rhs->move_nothrow()) {
            std::swap(lhs, rhs);
          }
          impl tmp(std::move(*rhs));
          // EXTENSION: When the move construction of `lhs` into `rhs` throws
          // and `tmp` is nothrow move constructible then we move `tmp` back
          // into `rhs` and provide the strong exception safety guarentee.
          try {
            this->generic_construct(*rhs, std::move(*lhs));
          } catch (...) {
            if (tmp.move_nothrow()) {
              this->generic_construct(*rhs, std::move(tmp));
            }
            throw;
          }
          this->generic_construct(*lhs, std::move(tmp));
        }
      }

      private:
      inline constexpr bool move_nothrow() const {
        constexpr bool results[] = {
            std::is_nothrow_move_constructible<Ts>::value...};
        return this->valueless_by_exception() || results[this->index()];
      }
    };

    template <typename... Ts>
    struct overload;

    template <>
    struct overload<> {
      void operator()() const;
    };

    template <typename T, typename... Ts>
    struct overload<T, Ts...> : overload<Ts...> {
      using overload<Ts...>::operator();
      lib::identity<T> operator()(T) const;
    };

    template <typename T, typename... Ts>
    using best_match_t =
        typename std::result_of_t<overload<Ts...>(T &&)>::type;

  }  // detail

  template <typename... Ts>
  class variant {
    static_assert(0 < sizeof...(Ts),
                  "variant must consist of at least one alternative.");

    static_assert(detail::all({!std::is_array<Ts>::value...}),
                  "variant can not have an array type as an alternative.");

    static_assert(detail::all({!std::is_reference<Ts>::value...}),
                  "variant can not have a reference type as an alternative.");

    static_assert(detail::all({!std::is_void<Ts>::value...}),
                  "variant can not have a void type as an alternative.");

    public:
    template <
        typename Front = lib::type_pack_element_t<0, Ts...>,
        std::enable_if_t<std::is_default_constructible<Front>::value, int> = 0>
    inline constexpr variant() noexcept(
        std::is_nothrow_default_constructible<Front>::value)
        : impl_(in_place_index<0>) {}

    variant(const variant &) = default;
    variant(variant &&) = default;

    template <typename Arg,
              std::enable_if_t<!std::is_same<std::decay_t<Arg>, variant>::value,
                               int> = 0,
              typename T = detail::best_match_t<Arg, Ts...>,
              std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
              std::enable_if_t<std::is_constructible<T, Arg>::value, int> = 0>
    inline constexpr variant(Arg &&arg) noexcept(
        std::is_nothrow_constructible<T, Arg>::value)
        : impl_(in_place_index<I>, std::forward<Arg>(arg)) {}

    template <
        std::size_t I,
        typename... Args,
        typename T = lib::type_pack_element_t<I, Ts...>,
        std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    inline explicit constexpr variant(
        in_place_index_t<I>,
        Args &&... args) noexcept(std::is_nothrow_constructible<T,
                                                                Args...>::value)
        : impl_(in_place_index<I>, std::forward<Args>(args)...) {}

    template <
        std::size_t I,
        typename Up,
        typename... Args,
        typename T = lib::type_pack_element_t<I, Ts...>,
        std::enable_if_t<std::is_constructible<T,
                                               std::initializer_list<Up> &,
                                               Args...>::value,
                         int> = 0>
    inline explicit constexpr variant(
        in_place_index_t<I>,
        std::initializer_list<Up> il,
        Args &&... args) noexcept(std::
                                      is_nothrow_constructible<
                                          T,
                                          std::initializer_list<Up> &,
                                          Args...>::value)
        : impl_(in_place_index<I>, il, std::forward<Args>(args)...) {}

    template <
        typename T,
        typename... Args,
        std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
        std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    inline explicit constexpr variant(
        in_place_type_t<T>,
        Args &&... args) noexcept(std::is_nothrow_constructible<T,
                                                                Args...>::value)
        : impl_(in_place_index<I>, std::forward<Args>(args)...) {}

    template <
        typename T,
        typename Up,
        typename... Args,
        std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
        std::enable_if_t<std::is_constructible<T,
                                               std::initializer_list<Up> &,
                                               Args...>::value,
                         int> = 0>
    inline explicit constexpr variant(
        in_place_type_t<T>,
        std::initializer_list<Up> il,
        Args &&... args) noexcept(std::
                                      is_nothrow_constructible<
                                          T,
                                          std::initializer_list<Up> &,
                                          Args...>::value)
        : impl_(in_place_index<I>, il, std::forward<Args>(args)...) {}

    ~variant() = default;

    variant &operator=(const variant &) = default;
    variant &operator=(variant &&) = default;

    template <typename Arg,
              std::enable_if_t<!std::is_same<std::decay_t<Arg>, variant>::value,
                               int> = 0,
              typename T = detail::best_match_t<Arg, Ts...>,
              std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
              std::enable_if_t<std::is_assignable<T &, Arg>::value &&
                                   std::is_constructible<T, Arg>::value,
                               int> = 0>
    inline variant &operator=(Arg &&arg) noexcept(
        (std::is_nothrow_assignable<T &, Arg>::value &&
         std::is_nothrow_constructible<T, Arg>::value)) {
      impl_.template assign<I>(std::forward<Arg>(arg));
      return *this;
    }

    template <
        std::size_t I,
        typename... Args,
        typename T = lib::type_pack_element_t<I, Ts...>,
        std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    inline void emplace(Args &&... args) {
      impl_.template emplace<I>(std::forward<Args>(args)...);
    }

    template <
        std::size_t I,
        typename Up,
        typename... Args,
        typename T = lib::type_pack_element_t<I, Ts...>,
        std::enable_if_t<std::is_constructible<T,
                                               std::initializer_list<Up> &,
                                               Args...>::value,
                         int> = 0>
    inline void emplace(std::initializer_list<Up> il, Args &&... args) {
      impl_.template emplace<I>(il, std::forward<Args>(args)...);
    }

    template <
        typename T,
        typename... Args,
        std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
        std::enable_if_t<std::is_constructible<T, Args...>::value, int> = 0>
    inline void emplace(Args &&... args) {
      impl_.template emplace<I>(std::forward<Args>(args)...);
    }

    template <
        typename T,
        typename Up,
        typename... Args,
        std::size_t I = detail::find_index_sfinae<T, Ts...>::value,
        std::enable_if_t<std::is_constructible<T,
                                               std::initializer_list<Up> &,
                                               Args...>::value,
                         int> = 0>
    inline void emplace(std::initializer_list<Up> il, Args &&... args) {
      impl_.template emplace<I>(il, std::forward<Args>(args)...);
    }

    inline constexpr bool valueless_by_exception() const noexcept {
      return impl_.valueless_by_exception();
    }

    inline constexpr std::size_t index() const noexcept {
      return impl_.index();
    }

    template <
        bool Dummy = true,
        std::enable_if_t<detail::all({Dummy,
                                      (std::is_move_constructible<Ts>::value &&
                                       lib::is_swappable<Ts>::value)...}),
                         int> = 0>
    inline void swap(variant &that) noexcept(
        detail::all({(std::is_nothrow_move_constructible<Ts>::value &&
                      lib::is_nothrow_swappable<Ts>::value)...})) {
      impl_.swap(that.impl_);
    }

    private:
    detail::impl<Ts...> impl_;

    friend struct detail::access::variant;
    friend struct detail::visitation::variant;
  };

  template <std::size_t I, typename... Ts>
  inline constexpr bool holds_alternative(const variant<Ts...> &v) noexcept {
    return v.index() == I;
  }

  template <typename T, typename... Ts>
  inline constexpr bool holds_alternative(const variant<Ts...> &v) noexcept {
    return holds_alternative<detail::find_index_checked<T, Ts...>::value>(v);
  }

  namespace detail {

    template <std::size_t I, typename V>
    inline constexpr auto &&generic_get(V &&v) {
      return (holds_alternative<I>(v) ? (void)0 : throw bad_variant_access{}),
             access::variant::get_alt<I>(std::forward<V>(v)).value_;
    }

  }  // namespace detail

  template <std::size_t I, typename... Ts>
  inline constexpr variant_alternative_t<I, variant<Ts...>> &get(
      variant<Ts...> &v) {
    return detail::generic_get<I>(v);
  }

  template <std::size_t I, typename... Ts>
  inline constexpr variant_alternative_t<I, variant<Ts...>> &&get(
      variant<Ts...> &&v) {
    return detail::generic_get<I>(std::move(v));
  }

  template <std::size_t I, typename... Ts>
  inline constexpr const variant_alternative_t<I, variant<Ts...>> &get(
      const variant<Ts...> &v) {
    return detail::generic_get<I>(v);
  }

  template <std::size_t I, typename... Ts>
  inline constexpr const variant_alternative_t<I, variant<Ts...>> &&get(
      const variant<Ts...> &&v) {
    return detail::generic_get<I>(std::move(v));
  }

  template <typename T, typename... Ts>
  inline constexpr T &get(variant<Ts...> &v) {
    return get<detail::find_index_checked<T, Ts...>::value>(v);
  }

  template <typename T, typename... Ts>
  inline constexpr T &&get(variant<Ts...> &&v) {
    return get<detail::find_index_checked<T, Ts...>::value>(std::move(v));
  }

  template <typename T, typename... Ts>
  inline constexpr const T &get(const variant<Ts...> &v) {
    return get<detail::find_index_checked<T, Ts...>::value>(v);
  }

  template <typename T, typename... Ts>
  inline constexpr const T &&get(const variant<Ts...> &&v) {
    return get<detail::find_index_checked<T, Ts...>::value>(std::move(v));
  }

  namespace detail {

    template <std::size_t I, typename V>
    inline constexpr auto *generic_get_if(V *v) noexcept {
      return v && holds_alternative<I>(*v)
                 ? lib::addressof(access::variant::get_alt<I>(*v).value_)
                 : nullptr;
    }

  }  // namespace detail

  template <std::size_t I, typename... Ts>
  inline constexpr std::add_pointer_t<variant_alternative_t<I, variant<Ts...>>>
  get_if(variant<Ts...> *v) noexcept {
    return detail::generic_get_if<I>(v);
  }

  template <std::size_t I, typename... Ts>
  inline constexpr std::add_pointer_t<
      const variant_alternative_t<I, variant<Ts...>>>
  get_if(const variant<Ts...> *v) noexcept {
    return detail::generic_get_if<I>(v);
  }

  template <typename T, typename... Ts>
  inline constexpr std::add_pointer_t<T>
  get_if(variant<Ts...> *v) noexcept {
    return get_if<detail::find_index_checked<T, Ts...>::value>(v);
  }

  template <typename T, typename... Ts>
  inline constexpr std::add_pointer_t<const T>
  get_if(const variant<Ts...> *v) noexcept {
    return get_if<detail::find_index_checked<T, Ts...>::value>(v);
  }

  template <typename... Ts>
  inline constexpr bool operator==(const variant<Ts...> &lhs,
                                   const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (lhs.index() != rhs.index()) return false;
    if (lhs.valueless_by_exception()) return true;
    return variant::visit_value_at(lhs.index(), std::equal_to<>{}, lhs, rhs);
  }

  template <typename... Ts>
  inline constexpr bool operator!=(const variant<Ts...> &lhs,
                                   const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (lhs.index() != rhs.index()) return true;
    if (lhs.valueless_by_exception()) return false;
    return variant::visit_value_at(
        lhs.index(), std::not_equal_to<>{}, lhs, rhs);
  }

  template <typename... Ts>
  inline constexpr bool operator<(const variant<Ts...> &lhs,
                                  const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (rhs.valueless_by_exception()) return false;
    if (lhs.valueless_by_exception()) return true;
    if (lhs.index() < rhs.index()) return true;
    if (lhs.index() > rhs.index()) return false;
    return variant::visit_value_at(lhs.index(), std::less<>{}, lhs, rhs);
  }

  template <typename... Ts>
  inline constexpr bool operator>(const variant<Ts...> &lhs,
                                  const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (lhs.valueless_by_exception()) return false;
    if (rhs.valueless_by_exception()) return true;
    if (lhs.index() > rhs.index()) return true;
    if (lhs.index() < rhs.index()) return false;
    return variant::visit_value_at(lhs.index(), std::greater<>{}, lhs, rhs);
  }

  template <typename... Ts>
  inline constexpr bool operator<=(const variant<Ts...> &lhs,
                                   const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (lhs.valueless_by_exception()) return true;
    if (rhs.valueless_by_exception()) return false;
    if (lhs.index() < rhs.index()) return true;
    if (lhs.index() > rhs.index()) return false;
    return variant::visit_value_at(lhs.index(), std::less_equal<>{}, lhs, rhs);
  }

  template <typename... Ts>
  inline constexpr bool operator>=(const variant<Ts...> &lhs,
                                   const variant<Ts...> &rhs) {
    using detail::visitation::variant;
    if (rhs.valueless_by_exception()) return true;
    if (lhs.valueless_by_exception()) return false;
    if (lhs.index() > rhs.index()) return true;
    if (lhs.index() < rhs.index()) return false;
    return variant::visit_value_at(
        lhs.index(), std::greater_equal<>{}, lhs, rhs);
  }

  template <typename Visitor, typename... Vs>
  inline constexpr decltype(auto) visit(Visitor &&visitor, Vs &&... vs) {
    using detail::visitation::variant;
    return (detail::all({!vs.valueless_by_exception()...})
                ? (void)0
                : throw bad_variant_access{}),
           variant::visit_value(std::forward<Visitor>(visitor),
                                std::forward<Vs>(vs)...);
  }

  struct monostate {};

  inline constexpr bool operator<(monostate, monostate) noexcept {
    return false;
  }

  inline constexpr bool operator>(monostate, monostate) noexcept {
    return false;
  }

  inline constexpr bool operator<=(monostate, monostate) noexcept {
    return true;
  }

  inline constexpr bool operator>=(monostate, monostate) noexcept {
    return true;
  }

  inline constexpr bool operator==(monostate, monostate) noexcept {
    return true;
  }

  inline constexpr bool operator!=(monostate, monostate) noexcept {
    return false;
  }

  template <typename... Ts>
  inline auto swap(variant<Ts...> &lhs,
                   variant<Ts...> &rhs) noexcept(noexcept(lhs.swap(rhs)))
      -> decltype(lhs.swap(rhs)) {
    lhs.swap(rhs);
  }

}  // namespace mpark

namespace std {

  template <typename... Ts>
  struct hash<mpark::variant<Ts...>> {
    using argument_type = mpark::variant<Ts...>;
    using result_type = std::size_t;

    inline result_type operator()(const argument_type &v) const {
      using mpark::detail::visitation::variant;
      std::size_t result =
          v.valueless_by_exception()
              ? 299792458  // Random value chosen by the universe upon creation
              : variant::visit_alt(
                    [](const auto &alt) {
                      using alt_type = decay_t<decltype(alt)>;
                      using value_type = typename alt_type::value_type;
                      return hash<value_type>{}(alt.value_);
                    },
                    v);
      return hash_combine(result, hash<std::size_t>{}(v.index()));
    }

    private:
    static std::size_t hash_combine(std::size_t lhs, std::size_t rhs) {
      return lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    }
  };

  template <>
  struct hash<mpark::monostate> {
    using argument_type = mpark::monostate;
    using result_type = std::size_t;

    inline result_type operator()(const argument_type &) const {
      return 66740831;  // return a fundamentally attractive random value.
    }
  };

}  // namespace std

#endif  // MPARK_VARIANT_HPP
