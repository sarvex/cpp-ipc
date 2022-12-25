/**
 * \file libimp/result.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the return value type with a status code
 * \date 2022-04-17
 */
#pragma once

#include <type_traits>
#include <string>
#include <tuple>
#include <cstdint>

#include "libimp/def.h"
#include "libimp/detect_plat.h"
#include "libimp/export.h"
#include "libimp/fmt.h"
#include "libimp/generic.h"
#include "libimp/error.h"

LIBIMP_NAMESPACE_BEG_
namespace detail_result {

template <typename T, typename = void>
struct default_traits;

} // namespace detail_result

template <typename T, 
          typename TypeTraits = detail_result::default_traits<T>>
class result;

/// \typedef Uses std::uint64_t as the default underlying type of result code.
using result_code = result<std::uint64_t>;

namespace detail_result {

template <typename T>
struct generic_traits {
  /// \typedef Combine data and valid identifiers with a tuple.
  using storage_t = std::tuple<T, error_code>;

  /// \brief Custom initialization.
  constexpr static void init_code(storage_t &code) noexcept {
    code = {0, error_number_limit/*make a default error code*/};
  }
  constexpr static void init_code(storage_t &code, T value, error_code const &ec) noexcept {
    code = {value, ec};
  }
  constexpr static void init_code(storage_t &code, T value) noexcept {
    code = {value, {}};
  }
  constexpr static void init_code(storage_t &code, error_code const &ec) noexcept {
    code = {{}, ec};
  }

  /// \brief Custom type data acquisition.
  constexpr static T get_value(storage_t const &code) noexcept {
    return std::get<0>(code);
  }
  constexpr static bool get_ok(storage_t const &code) noexcept {
    return !std::get<1>(code);
  }
  constexpr static error_code get_error(storage_t const &code) noexcept {
    return std::get<1>(code);
  }
};

template <typename ___>
struct default_traits<void, ___> {
  /// \typedef Use the `error_code` as the storage type.
  using storage_t = error_code;

  /// \brief Custom initialization.
  constexpr static void init_code(storage_t &code) noexcept {
    code = error_number_limit/*make a default error code*/;
  }
  constexpr static void init_code(storage_t &code, error_code const &ec) noexcept {
    code = ec;
  }

  /// \brief Custom type data acquisition.
  constexpr static bool get_ok(storage_t const &code) noexcept {
    return !code;
  }
  constexpr static error_code get_error(storage_t const &code) noexcept {
    return code;
  }

  /// \brief Custom formatted output.
  static std::string format(result<void> const &r) noexcept {
    return fmt("error = ", r.error());
  }
};

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_integral<T>::value>> : generic_traits<T> {
  /// \brief Custom initialization.
  constexpr static void init_code(storage_t &code, T value, bool ok) noexcept {
    code = {value, static_cast<error_code_t>(ok ? 0 : 
                    ((value == default_value()) ? error_number_limit : value))};
  }
  using generic_traits<T>::init_code;

  /// \brief Custom default value.
  constexpr static T default_value() noexcept {
    return 0;
  }

  /// \brief Custom formatted output.
  static std::string format(result<T> const &r) noexcept {
    return fmt(*r);
  }
};

template <typename T>
struct default_traits<T, std::enable_if_t<std::is_pointer<T>::value>> : generic_traits<T> {
  /// \brief Custom initialization.
  constexpr static void init_code(storage_t &code, std::nullptr_t, error_code const &ec) noexcept {
    code = {nullptr, ec};
  }
  constexpr static void init_code(storage_t &code, std::nullptr_t) noexcept {
    code = {nullptr, -1};
  }
  using generic_traits<T>::init_code;

  /// \brief Custom default value.
  constexpr static T default_value() noexcept {
    return nullptr;
  }

  /// \brief Custom formatted output.
  static std::string format(result<T> const &r) noexcept {
    if LIBIMP_LIKELY(r) {
      return fmt(static_cast<void *>(*r));
    }
    return fmt(static_cast<void *>(*r), ", error = ", r.error());
  }
};

} // namespace detail_result

/**
 * \class class result
 * \brief The generic wrapper for the result type.
 */
template <typename T, typename TypeTraits>
class result : public TypeTraits {
public:
  using type_traits_t = TypeTraits;
  using storage_t     = typename type_traits_t::storage_t;

private:
  storage_t code_; ///< internal data

public:
  template <typename... A, 
            typename = is_not_match<result, A...>,
            typename = decltype(type_traits_t::init_code(std::declval<storage_t &>()
                                                       , std::declval<A>()...))>
  result(A &&... args) noexcept {
    type_traits_t::init_code(code_, std::forward<A>(args)...);
  }

  T          value() const noexcept { return type_traits_t::get_value(code_); }
  bool       ok   () const noexcept { return type_traits_t::get_ok   (code_); }
  error_code error() const noexcept { return type_traits_t::get_error(code_); }

         T operator *   () const noexcept { return value(); }
  explicit operator bool() const noexcept { return ok   (); }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.code_ == rhs.code_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return !(lhs == rhs); }
};

template <typename TypeTraits>
class result<void, TypeTraits> {
public:
  using type_traits_t = TypeTraits;
  using storage_t     = typename type_traits_t::storage_t;

private:
  storage_t code_; ///< internal data

public:
  template <typename... A, 
            typename = is_not_match<result, A...>,
            typename = decltype(type_traits_t::init_code(std::declval<storage_t &>()
                                                       , std::declval<A>()...))>
  result(A &&... args) noexcept {
    type_traits_t::init_code(code_, std::forward<A>(args)...);
  }

  bool       ok   () const noexcept { return type_traits_t::get_ok   (code_); }
  error_code error() const noexcept { return type_traits_t::get_error(code_); }

  explicit operator bool() const noexcept { return ok   (); }

  friend bool operator==(result const &lhs, result const &rhs) noexcept { return lhs.code_ == rhs.code_; }
  friend bool operator!=(result const &lhs, result const &rhs) noexcept { return !(lhs == rhs); }
};

/// \brief Custom defined fmt_to method for imp::fmt
namespace detail {

template <typename T, typename D>
bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, result<T, D> r) {
  return fmt_to(ctx, (r ? "succ" : "fail"), ", value = ", result<T, D>::type_traits_t::format(r));
}

template <typename D>
bool tag_invoke(decltype(::LIBIMP::fmt_to), fmt_context &ctx, result<void, D> r) {
  return fmt_to(ctx, (r ? "succ" : "fail"), ", ", result<void, D>::type_traits_t::format(r));
}

} // namespace detail
LIBIMP_NAMESPACE_END_
