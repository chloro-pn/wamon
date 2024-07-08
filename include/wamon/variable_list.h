#pragma once

#include "wamon/interpreter.h"
#include "wamon/variable.h"

namespace wamon {

class ListVariable : public CompoundVariable {
 public:
  ListVariable(std::unique_ptr<Type>&& element_type, ValueCategory vc, const std::string& name)
      : CompoundVariable(std::make_unique<ListType>(element_type->Clone()), vc, name),
        element_type_(std::move(element_type)) {}

  void PushBack(std::shared_ptr<Variable> element);

  void PopBack();

  size_t Size() const { return elements_.size(); }

  void Resize(size_t new_size, Interpreter& ip) {
    size_t old_size = Size();
    if (new_size == old_size) {
      return;
    } else if (new_size < old_size) {
      elements_.resize(new_size);
    } else {
      for (size_t i = 0; i < (new_size - old_size); ++i) {
        auto v = VariableFactory(element_type_, vc_, "", ip);
        v->DefaultConstruct();
        elements_.push_back(std::move(v));
      }
    }
  }

  void Clear() { elements_.clear(); }

  void Erase(size_t index) {
    if (index >= elements_.size()) {
      throw WamonException("List.Erase error, index out of range : {} >= {}", index, elements_.size());
    }
    auto it = elements_.begin();
    std::advance(it, index);
    elements_.erase(it);
  }

  void Insert(size_t index, std::shared_ptr<Variable> v) {
    if (index > Size()) {
      throw WamonException("List.Insert error, index = {}, size = {}", index, Size());
    }

    std::shared_ptr<Variable> tmp;
    if (v->IsRValue()) {
      tmp = v;
    } else {
      tmp = v->Clone();
    }
    tmp->ChangeTo(vc_);
    auto it = elements_.begin();
    std::advance(it, index);
    elements_.insert(it, std::move(tmp));
  }

  std::shared_ptr<Variable> at(size_t i) {
    if (i >= Size()) {
      throw WamonException("ListVariable.at error, index {} out of range", i);
    }
    return elements_[i];
  }

  std::string get_string_only_for_byte_list();

  void ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) override;

  void DefaultConstruct() override;

  std::shared_ptr<Variable> Clone() override;

  bool Compare(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    ListVariable* list_v = static_cast<ListVariable*>(other.get());
    if (elements_.size() != list_v->elements_.size()) {
      return false;
    }
    for (size_t i = 0; i < elements_.size(); ++i) {
      if (elements_[i]->Compare((list_v->elements_)[i]) == false) {
        return false;
      }
    }
    return true;
  }

  void Assign(const std::shared_ptr<Variable>& other) override {
    check_compare_type_match(other);
    ListVariable* list_v = static_cast<ListVariable*>(other.get());
    elements_.clear();
    for (size_t i = 0; i < list_v->elements_.size(); ++i) {
      // PushBack函数会处理值型别
      PushBack(list_v->elements_[i]);
    }
  }

  void ChangeTo(ValueCategory vc) override {
    vc_ = vc;
    for (auto& each : elements_) {
      each->ChangeTo(vc);
    }
  }

  nlohmann::json Print() override {
    nlohmann::json list;
    for (size_t i = 0; i < elements_.size(); ++i) {
      list.push_back(elements_[i]->Print());
    }
    return list;
  }

 private:
  std::unique_ptr<Type> element_type_;
  std::vector<std::shared_ptr<Variable>> elements_;
};

inline ListVariable* AsListVariable(const std::shared_ptr<Variable>& v) { return static_cast<ListVariable*>(v.get()); }

}  // namespace wamon