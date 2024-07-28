#include "wamon/variable_list.h"

namespace wamon {

void ListVariable::PushBack(std::shared_ptr<Variable> element) {
  if (element->IsRValue()) {
    elements_.push_back(std::move(element));
  } else {
    elements_.push_back(element->Clone());
  }
  elements_.back()->ChangeTo(vc_);
}

void ListVariable::PopBack() {
  if (elements_.empty()) {
    throw WamonException("List pop back error, empty list");
  }
  elements_.pop_back();
}

std::string ListVariable::get_string_only_for_byte_list() {
  if (!IsByteType(element_type_)) {
    throw WamonException("ListVariable.get_string_only_for_byte_list can only be called by List(byte) type");
  }
  std::string ret;
  for (auto& each : elements_) {
    ret.push_back(char(AsByteVariable(each)->GetValue()));
  }
  return ret;
}

void ListVariable::ConstructByFields(const std::vector<std::shared_ptr<Variable>>& fields) {
  for (auto& each : fields) {
    if (each->GetTypeInfo() != element_type_->GetTypeInfo()) {
      throw WamonException("ListVariable::ConstructByFields error, type dismatch : {} != {}", each->GetTypeInfo(),
                           element_type_->GetTypeInfo());
    }
    if (each->IsRValue()) {
      elements_.push_back(std::move(each));
    } else {
      elements_.push_back(each->Clone());
    }
    elements_.back()->ChangeTo(vc_);
  }
}

void ListVariable::DefaultConstruct() { elements_.clear(); }

std::shared_ptr<Variable> ListVariable::Clone() {
  std::vector<std::shared_ptr<Variable>> elements;
  for (auto& each : elements_) {
    if (each->IsRValue()) {
      elements.push_back(std::move(each));
    } else {
      elements.push_back(each->Clone());
    }
  }
  auto ret = std::make_shared<ListVariable>(element_type_->Clone(), ValueCategory::RValue);
  ret->elements_ = std::move(elements);
  return ret;
}

}  // namespace wamon