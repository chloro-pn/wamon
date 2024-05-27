#pragma once

#include <memory>

namespace wamon {

template <typename T>
std::shared_ptr<T> ptr_cast(std::unique_ptr<T>&& ptr) {
  return std::shared_ptr<T>(std::move(ptr));
}

template <typename T>
std::unique_ptr<T> ptr_cast(std::shared_ptr<T>&& ptr) {
  assert(ptr.use_count() == 1);
  std::unique_ptr<T> ret(ptr.get());
  ptr.reset();
  return ret;
}

}  // namespace wamon