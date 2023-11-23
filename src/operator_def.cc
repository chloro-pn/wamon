#include "wamon/operator_def.h"

#include "wamon/variable.h"

namespace wamon {

std::string OperatorDef::CreateName(Token token, const std::vector<std::shared_ptr<Variable>>& param_list) {
  std::string type_list_id;
  type_list_id.reserve(16);
  for (auto& each : param_list) {
    type_list_id += each->GetTypeInfo();
    type_list_id += "-";
  }
  return std::string("__op_") + (token == Token::LEFT_PARENTHESIS ? "call" : GetTokenStr(token)) + type_list_id;
}

}  // namespace wamon