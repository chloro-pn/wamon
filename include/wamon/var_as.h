#pragma once

#include "wamon/variable_bool.h"
#include "wamon/variable_byte.h"
#include "wamon/variable_double.h"
#include "wamon/variable_int.h"
#include "wamon/variable_string.h"

namespace wamon {

/* ********************************************************************
 * API : VarAs
 *
 * ********************************************************************/
template <typename T>
T VarAs(const std::shared_ptr<Variable>& v) {
  if constexpr (std::is_same_v<int, T>) {
    assert(IsSameType(TypeFactory<int>::Get(), v->GetType()));
    return AsIntVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<double, T>) {
    assert(IsSameType(TypeFactory<double>::Get(), v->GetType()));
    return AsDoubleVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<unsigned char, T>) {
    assert(IsSameType(TypeFactory<unsigned char>::Get(), v->GetType()));
    return AsByteVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<bool, T>) {
    assert(IsSameType(TypeFactory<bool>::Get(), v->GetType()));
    return AsBoolVariable(v)->GetValue();
  }
  if constexpr (std::is_same_v<std::string, T>) {
    assert(IsSameType(TypeFactory<std::string>::Get(), v->GetType()));
    return AsStringVariable(v)->GetValue();
  }
  throw WamonException("VarAs error, not support type {} now", v->GetTypeInfo());
  WAMON_UNREACHABLE;
  return *static_cast<T*>(nullptr);
}

}  // namespace wamon
