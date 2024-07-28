#pragma once

#include "wamon/variable_bool.h"
#include "wamon/variable_byte.h"
#include "wamon/variable_double.h"
#include "wamon/variable_int.h"
#include "wamon/variable_string.h"

namespace wamon {

/* ********************************************************************
 * API : ToVar
 *
 * ********************************************************************/
#define WAMON_TO_VAR(basic_type, transform_type)                                                 \
  if constexpr (std::is_same_v<type, basic_type>) {                                              \
    auto ret = VariableFactory(TypeFactory<basic_type>::Get(), Variable::ValueCategory::LValue); \
    As##transform_type##Variable(ret)->SetValue(std::forward<T>(v));                             \
    return ret;                                                                                  \
  }

inline std::shared_ptr<Variable> ToVar(std::string_view strv) {
  auto ret = VariableFactory(TypeFactory<std::string>::Get(), Variable::ValueCategory::LValue);
  AsStringVariable(ret)->SetValue(strv);
  return ret;
}

template <size_t n>
std::shared_ptr<Variable> ToVar(char (&cstr)[n]) {
  return ToVar(std::string_view(cstr));
}

template <typename T>
concept WAMON_SUPPORT_TOVAR = std::same_as<T, int> || std::same_as<T, double> || std::same_as<T, unsigned char> ||
    std::same_as<T, bool> || std::same_as<T, std::string>;

template <typename T>
requires WAMON_SUPPORT_TOVAR<std::remove_cvref_t<T>> std::shared_ptr<Variable> ToVar(T&& v) {
  using type = std::remove_cvref_t<T>;
  WAMON_TO_VAR(int, Int)
  WAMON_TO_VAR(double, Double)
  WAMON_TO_VAR(unsigned char, Byte)
  WAMON_TO_VAR(bool, Bool)
  WAMON_TO_VAR(std::string, String)

  WAMON_UNREACHABLE;
  return nullptr;
}

inline std::shared_ptr<Variable> ToVar(std::shared_ptr<Variable> v) { return v; }

}  // namespace wamon
