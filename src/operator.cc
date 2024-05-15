#include "wamon/operator.h"

#include "wamon/interpreter.h"
#include "wamon/operator_def.h"

namespace wamon {

static void register_buildin_operators(std::unordered_map<Token, int>& ops) {
  ops[Token::COMPARE] = 0;
  ops[Token::ASSIGN] = 0;
  ops[Token::PLUS] = 1;
  ops[Token::MINUS] = 1;
  ops[Token::MULTIPLY] = 2;
  ops[Token::DIVIDE] = 2;
  ops[Token::AND] = 5;
  ops[Token::OR] = 4;
  ops[Token::PIPE] = 6;
  ops[Token::MEMBER_ACCESS] = 7;
  ops[Token::SUBSCRIPT] = 7;
  ops[Token::AS] = 8;
}

static void register_buildin_u_operators(std::unordered_map<Token, int>& ops) {
  // -
  ops[Token::MINUS] = 0;
  // *
  ops[Token::MULTIPLY] = 0;
  // &
  ops[Token::ADDRESS_OF] = 0;
  // !
  ops[Token::NOT] = 0;
  // move
  ops[Token::MOVE] = 0;
}

static void register_buildin_operator_handles(std::unordered_map<std::string, Operator::BinaryOperatorType>& handles) {
  // operator +
  std::vector<std::unique_ptr<Type>> operands;
  operands.push_back(TypeFactory<int>::Get());
  operands.push_back(TypeFactory<int>::Get());
  std::string tmp = OperatorDef::CreateName(Token::PLUS, operands);
  handles[tmp] = [](Interpreter&, std::shared_ptr<Variable> v1,
                    std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    int v = AsIntVariable(v1)->GetValue() + AsIntVariable(v2)->GetValue();
    return std::make_shared<IntVariable>(v, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  operands.push_back(TypeFactory<double>::Get());
  operands.push_back(TypeFactory<double>::Get());
  tmp = OperatorDef::CreateName(Token::PLUS, operands);
  handles[tmp] = [](Interpreter&, std::shared_ptr<Variable> v1,
                    std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    double v = AsDoubleVariable(v1)->GetValue() + AsDoubleVariable(v2)->GetValue();
    return std::make_shared<DoubleVariable>(v, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  operands.push_back(TypeFactory<std::string>::Get());
  operands.push_back(TypeFactory<std::string>::Get());
  tmp = OperatorDef::CreateName(Token::PLUS, operands);
  handles[tmp] = [](Interpreter&, std::shared_ptr<Variable> v1,
                    std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    std::string v = AsStringVariable(v1)->GetValue() + AsStringVariable(v2)->GetValue();
    return std::make_shared<StringVariable>(v, Variable::ValueCategory::RValue, "");
  };

  // operator -、*、/ for type int and double
  static Token ops[3] = {
      Token::MINUS,
      Token::MULTIPLY,
      Token::DIVIDE,
  };

  static std::unique_ptr<Type> types[2] = {
      TypeFactory<int>::Get(),
      TypeFactory<double>::Get(),
  };

  static Operator::BinaryOperatorType dhandles[6] = {
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        int v = AsIntVariable(v1)->GetValue() - AsIntVariable(v2)->GetValue();
        return std::make_shared<IntVariable>(v, Variable::ValueCategory::RValue, "");
      },
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        double v = AsDoubleVariable(v1)->GetValue() - AsDoubleVariable(v2)->GetValue();
        return std::make_shared<DoubleVariable>(v, Variable::ValueCategory::RValue, "");
      },
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        int v = AsIntVariable(v1)->GetValue() * AsIntVariable(v2)->GetValue();
        return std::make_shared<IntVariable>(v, Variable::ValueCategory::RValue, "");
      },
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        double v = AsDoubleVariable(v1)->GetValue() * AsDoubleVariable(v2)->GetValue();
        return std::make_shared<DoubleVariable>(v, Variable::ValueCategory::RValue, "");
      },
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        int v = AsIntVariable(v1)->GetValue() / AsIntVariable(v2)->GetValue();
        return std::make_shared<IntVariable>(v, Variable::ValueCategory::RValue, "");
      },
      [](Interpreter&, std::shared_ptr<Variable> v1, std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
        double v = AsDoubleVariable(v1)->GetValue() / AsDoubleVariable(v2)->GetValue();
        return std::make_shared<DoubleVariable>(v, Variable::ValueCategory::RValue, "");
      },
  };

  size_t handle_index = 0;

  for (auto& op : ops) {
    for (auto& type : types) {
      operands.clear();
      operands.push_back(type->Clone());
      operands.push_back(type->Clone());
      tmp = OperatorDef::CreateName(op, operands);
      handles[tmp] = dhandles[handle_index];
      handle_index += 1;
    }
  }

  // operator .
  operands.clear();
  handles[GetTokenStr(Token::MEMBER_ACCESS)] = [](Interpreter&, std::shared_ptr<Variable> v1,
                                                  std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    const std::string& data_member_name = AsStringVariable(v2)->GetValue();
    auto data = AsStructVariable(v1);
    return data->GetDataMemberByName(data_member_name);
  };

  // operator && ||
  operands.clear();
  operands.push_back(TypeFactory<bool>::Get());
  operands.push_back(TypeFactory<bool>::Get());
  tmp = OperatorDef::CreateName(Token::AND, operands);
  handles[tmp] = [](Interpreter&, std::shared_ptr<Variable> v1,
                    std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    double v = AsBoolVariable(v1)->GetValue() && AsBoolVariable(v2)->GetValue();
    return std::make_shared<BoolVariable>(v, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  operands.push_back(TypeFactory<bool>::Get());
  operands.push_back(TypeFactory<bool>::Get());
  tmp = OperatorDef::CreateName(Token::OR, operands);
  handles[tmp] = [](Interpreter&, std::shared_ptr<Variable> v1,
                    std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    double v = AsBoolVariable(v1)->GetValue() || AsBoolVariable(v2)->GetValue();
    return std::make_shared<BoolVariable>(v, Variable::ValueCategory::RValue, "");
  };

  // operator ==、=
  operands.clear();
  handles[GetTokenStr(Token::COMPARE)] = [](Interpreter&, std::shared_ptr<Variable> v1,
                                            std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    bool ret = v1->Compare(v2);
    return std::make_shared<BoolVariable>(ret, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  handles[GetTokenStr(Token::ASSIGN)] = [](Interpreter&, std::shared_ptr<Variable> v1,
                                           std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    v1->Assign(v2);
    return std::make_shared<VoidVariable>();
  };

  // operator []
  handles[GetTokenStr(Token::SUBSCRIPT)] = [](Interpreter&, std::shared_ptr<Variable> v1,
                                              std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    return AsListVariable(v1)->at(AsIntVariable(v2)->GetValue());
  };

  // operator as
  handles[GetTokenStr(Token::AS)] = [](Interpreter& interpreter, std::shared_ptr<Variable> v1,
                                       std::shared_ptr<Variable> v2) -> std::shared_ptr<Variable> {
    assert(AsTypeVariable(v2) != nullptr);
    auto from_type = v1->GetType();
    auto to_type = v2->GetType();
    if (IsIntType(from_type) && IsDoubleType(to_type)) {
      return std::make_shared<DoubleVariable>(AsIntVariable(v1)->GetValue(), Variable::ValueCategory::RValue, "");
    }
    if (IsDoubleType(from_type) && IsIntType(to_type)) {
      return std::make_shared<IntVariable>(static_cast<int>(AsDoubleVariable(v1)->GetValue()),
                                           Variable::ValueCategory::RValue, "");
    }
    if (IsIntType(from_type) && IsBoolType(to_type)) {
      return std::make_shared<BoolVariable>(AsIntVariable(v1)->GetValue() == 0 ? false : true,
                                            Variable::ValueCategory::RValue, "");
    }
    if (TypeFactory<std::vector<unsigned char>>::Get()->GetTypeInfo() == from_type->GetTypeInfo() &&
        IsStringType(to_type)) {
      auto tmp = std::make_shared<StringVariable>("", Variable::ValueCategory::RValue, "");
      tmp->SetValue(AsListVariable(v1)->get_string_only_for_byte_list());
      return tmp;
    }
    // struct to struct trait
    auto v = VariableFactory(to_type, Variable::ValueCategory::RValue, "", interpreter.GetPackageUnit());
    v->ConstructByFields({v1});
    return v;
  };
}

static void register_buildin_uoperator_handles(std::unordered_map<std::string, Operator::UnaryOperatorType>& handles) {
  std::vector<std::unique_ptr<Type>> operands;
  handles[GetTokenStr(Token::DIVIDE)] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    return AsPointerVariable(v)->GetHoldVariable();
  };
  handles[GetTokenStr(Token::ADDRESS_OF)] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    auto ret = std::make_shared<PointerVariable>(v->GetType(), Variable::ValueCategory::RValue, "");
    ret->SetHoldVariable(v);
    return ret;
  };
  handles[GetTokenStr(Token::MOVE)] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    v->ChangeTo(Variable::ValueCategory::RValue);
    return v;
  };
  operands.push_back(TypeFactory<bool>::Get());
  auto tmp = OperatorDef::CreateName(Token::NOT, operands);
  handles[tmp] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    bool tmp = AsBoolVariable(v)->GetValue();
    return std::make_shared<BoolVariable>(!tmp, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  operands.push_back(TypeFactory<int>::Get());
  tmp = OperatorDef::CreateName(Token::MINUS, operands);
  handles[tmp] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    int tmp = AsIntVariable(v)->GetValue();
    return std::make_shared<IntVariable>(-tmp, Variable::ValueCategory::RValue, "");
  };

  operands.clear();
  operands.push_back(TypeFactory<double>::Get());
  tmp = OperatorDef::CreateName(Token::MINUS, operands);
  handles[tmp] = [](std::shared_ptr<Variable> v) -> std::shared_ptr<Variable> {
    double tmp = AsDoubleVariable(v)->GetValue();
    return std::make_shared<DoubleVariable>(-tmp, Variable::ValueCategory::RValue, "");
  };
}

std::shared_ptr<Variable> Operator::Calculate(Interpreter& interpreter, Token op, std::shared_ptr<Variable> v1,
                                              std::shared_ptr<Variable> v2) {
  std::string tmp;
  std::vector<std::unique_ptr<Type>> opernads;
  opernads.push_back(v1->GetType());
  opernads.push_back(v2->GetType());
  if (op == Token::MEMBER_ACCESS || op == Token::COMPARE || op == Token::ASSIGN || op == Token::SUBSCRIPT ||
      op == Token::AS) {
    tmp = GetTokenStr(op);
  } else {
    tmp = OperatorDef::CreateName(op, opernads);
  }
  auto handle = operator_handles_.find(tmp);
  if (handle == operator_handles_.end()) {
    return nullptr;
  }
  return (*handle).second(interpreter, v1, v2);
}

std::shared_ptr<Variable> Operator::Calculate(Interpreter& interpreter, Token op, std::shared_ptr<Variable> v) {
  std::string handle_name;
  // &和*对所有类型适用，因此特殊处理
  if (op == Token::ADDRESS_OF || op == Token::MULTIPLY || op == Token::MOVE) {
    handle_name = GetTokenStr(op);
  } else {
    std::vector<std::unique_ptr<Type>> operands;
    operands.push_back(v->GetType());
    handle_name = OperatorDef::CreateName(op, operands);
  }
  auto handle = uoperator_handles_.find(handle_name);
  if (handle == uoperator_handles_.end()) {
    return nullptr;
  }
  return (*handle).second(v);
}

Operator::Operator() {
  register_buildin_operators(operators_);
  register_buildin_u_operators(uoperators_);
  register_buildin_operator_handles(operator_handles_);
  register_buildin_uoperator_handles(uoperator_handles_);
}
}  // namespace wamon
