#pragma once

#include <utility>

namespace wamon {

template <typename T>
class MoveWrapper {
 public:
  MoveWrapper(T&& obj) : obj_(std::move(obj)) {}

  MoveWrapper(const MoveWrapper& other) : obj_(std::move(const_cast<MoveWrapper*>(&other)->obj_)) {}

  MoveWrapper& operator=(const MoveWrapper& other) { obj_ = std::move(other.obj_); }

  T Get() && { return std::move(obj_); }

 private:
  T obj_;
};

}  // namespace wamon
