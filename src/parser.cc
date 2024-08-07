#include "wamon/parser.h"

#include <cassert>
#include <stack>
#include <utility>

#include "wamon/capture_id_item.h"
#include "wamon/method_def.h"
#include "wamon/operator.h"
#include "wamon/package_unit.h"
#include "wamon/parsing_package.h"

namespace wamon {
/*
 * @brief 判断tokens[begin]处的token值 == token，如果索引不合法，或者token值不相同，返回false; 否则递增索引，返回true。
 */
bool AssertToken(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    return false;
  }
  begin += 1;
  return true;
}

/*
 * @brief 判断tokens[begin]处的token值 == token，如果索引不合法，或者token值不相同，抛出runtime_error异常，否则递增索引
 * @todo 具化异常信息
 */
void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token, const char *file, int line) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    throw WamonException("assert token error, assert {}, but {}, file : {}, line {}", GetTokenStr(token),
                         GetTokenStr(tokens[begin].token), file, line);
  }
  begin += 1;
}

// package_name, var_name
template <bool without_packagename = false>
std::pair<std::string, std::string> ParseIdentifier(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                    size_t &begin) {
  if (tokens.size() <= begin || tokens[begin].token != Token::ID || tokens[begin].type != WamonToken::ValueType::STR) {
    throw WamonException("parse identifier error : {}", GetTokenStr(tokens[begin].token));
  }
  std::string v1 = tokens[begin].Get<std::string>();
  begin += 1;
  if (tokens.size() <= begin || tokens[begin].token != Token::SCOPE) {
    return {pu.GetCurrentParsingPackage(), v1};
  }
  if constexpr (without_packagename) {
    throw WamonException("ParseIdentifier error, without_packagename == true but still parse package_name {}", v1);
  }
  begin += 1;
  if (tokens[begin].token != Token::ID || tokens[begin].type != WamonToken::ValueType::STR) {
    throw WamonException("parse identifier error : {}", GetTokenStr(tokens[begin].token));
  }
  std::string v2 = tokens[begin].Get<std::string>();
  begin += 1;
  AssertInImportListOrThrow(pu, v1);
  return {v1, v2};
}

std::pair<std::string, std::string> ParseBasicType(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                   size_t &begin) {
  if (tokens.size() <= begin) {
    throw WamonException("parse basic type error, invalid index {}", begin);
  }
  std::string package_name;
  std::string ret;
  if (tokens[begin].token == Token::STRING) {
    ret = "string";
    begin += 1;
  } else if (tokens[begin].token == Token::INT) {
    ret = "int";
    begin += 1;
  } else if (tokens[begin].token == Token::BYTE) {
    ret = "byte";
    begin += 1;
  } else if (tokens[begin].token == Token::BOOL) {
    ret = "bool";
    begin += 1;
  } else if (tokens[begin].token == Token::DOUBLE) {
    ret = "double";
    begin += 1;
  } else if (tokens[begin].token == Token::VOID) {
    ret = "void";
    begin += 1;
  } else {
    auto tmp = ParseIdentifier(pu, tokens, begin);
    package_name = tmp.first;
    ret = tmp.second;
  }
  return {package_name, ret};
}

// 从tokens[begin]开始解析一个类型信息，更新begin，并返回解析到的类型
// 仅从语法分析的角度进行解析，不一定是合法的类型。
std::unique_ptr<Type> ParseType(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t &begin) {
  if (AssertToken(tokens, begin, Token::PTR)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    auto nested_type = ParseType(pu, tokens, begin);
    auto tmp = std::make_unique<PointerType>(std::move(nested_type));
    if (right_parent != begin) {
      throw WamonException("parse ptr type error");
    }
    begin += 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::LIST)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    auto nested_type = ParseType(pu, tokens, begin);
    std::unique_ptr<ListType> tmp(new ListType(std::move(nested_type)));
    begin = right_parent + 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::F)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    std::vector<std::unique_ptr<Type>> param_list;
    std::unique_ptr<Type> return_type;
    ParseTypeList(pu, tokens, begin, right_parent, param_list, return_type);

    std::unique_ptr<FuncType> tmp(new FuncType(std::move(param_list), std::move(return_type)));
    begin = right_parent + 1;
    return tmp;
  }
  // parse basic type
  std::unique_ptr<Type> ret;
  auto [package_name, type_name] = ParseBasicType(pu, tokens, begin);
  if (!package_name.empty()) {
    type_name = package_name + "$" + type_name;
  }
  ret = std::make_unique<BasicType>(type_name);
  return ret;
}

/*
 *  ( ( type1, type2, type3, ..., typen ) -> return_type )
 * begin                                              end
 */
void ParseTypeList(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin, size_t end,
                   std::vector<std::unique_ptr<Type>> &param_list, std::unique_ptr<Type> &return_type) {
  size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  begin += 1;
  while (begin != right_parent) {
    auto type = ParseType(pu, tokens, begin);
    param_list.push_back(std::move(type));
    if (AssertToken(tokens, begin, Token::COMMA) == false) {
      break;
    }
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS, __FILE__, __LINE__);
  if (AssertToken(tokens, begin, Token::ARROW) == true) {
    return_type = ParseType(pu, tokens, begin);
  } else {
    // bug fix : 如果没有指定也要初始化为void
    return_type = GetVoidType();
  }
  if (begin != end) {
    throw WamonException("parse type list error");
  }
}

/*
 * [ id1, id2, ... , idn ]
 * todo : update to : [ [move|ref] id1, [move|ref] id2, ...]
 */
void ParseCaptureIdList(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin, size_t end,
                        std::vector<CaptureIdItem> &ids) {
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACKETS, __FILE__, __LINE__);
  while (true) {
    if (begin == end) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACKETS, __FILE__, __LINE__);
      break;
    }
    CaptureIdItem item;
    if (AssertToken(tokens, begin, wamon::Token::MOVE)) {
      item.type = CaptureIdItem::Type::MOVE;
    } else if (AssertToken(tokens, begin, wamon::Token::REF)) {
      item.type = CaptureIdItem::Type::REF;
    } else {
      item.type = CaptureIdItem::Type::NORMAL;
    }
    auto [package_name, id] = ParseIdentifier(pu, tokens, begin);
    id = package_name + "$" + id;
    // fix: src/parser.cc:182:54: error: 'id' in capture list does not name a variable
    std::string_view id_view(id);
    auto it = std::find_if(ids.begin(), ids.end(), [&id_view](auto &item) -> bool { return item.id == id_view; });
    if (it != ids.end()) {
      throw WamonException("ParseCpatureIdList error, duplicate id {}", id);
    }
    item.id = id;
    ids.push_back(item);
    if (begin < end) {
      AssertTokenOrThrow(tokens, begin, Token::COMMA, __FILE__, __LINE__);
    }
  }
}

/*
 * lambda [id1, id2, ...] ( type1 param1, type2 param2, ...) -> return_type {
 *     block_stmt
 * }
 * parser解析一个lambda并将其注册为一个全局函数，返回该函数的名字
 * PackageUnit需要传递进parser中，不然无法注册
 */
std::string ParseLambda(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t &begin) {
  auto name = pu.CreateUniqueLambdaName();
  std::unique_ptr<FunctionDef> lambda_def(new FunctionDef(name));

  AssertTokenOrThrow(tokens, begin, Token::LAMBDA, __FILE__, __LINE__);
  size_t capture_end = FindMatchedToken<Token::LEFT_BRACKETS, Token::RIGHT_BRACKETS>(tokens, begin);
  std::vector<CaptureIdItem> capture_ids;
  ParseCaptureIdList(pu, tokens, begin, capture_end, capture_ids);
  lambda_def->SetCaptureIds(std::move(capture_ids));

  begin = capture_end + 1;
  size_t param_end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(pu, tokens, begin, param_end);
  for (auto &&each : param_list) {
    lambda_def->AddParamList(std::move(each));
  }
  begin = param_end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(pu, tokens, begin);
  lambda_def->SetReturnType(std::move(return_type));

  auto end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
  lambda_def->SetBlockStmt(std::move(stmt_block));

  pu.AddLambdaFunction(name, std::move(lambda_def));
  begin = end + 1;
  return name;
}

/*
 * 这个enum标识了在ParseExpression循环中，上一次解析的情况，如果：
 * 这是解析的第一个token，BEGIN，
 * 上一个解析的token是二元运算符， B_OP，
 * 上一个解析的token是单元运算符，U_OP,
 * 上一个解析的token不是运算符，NO_OP,
 */
enum class ParseExpressionState {
  BEGIN,
  B_OP,
  U_OP,
  NO_OP,
};

bool canParseUnaryOperator(const ParseExpressionState &ps) {
  return ps == ParseExpressionState::BEGIN || ps == ParseExpressionState::B_OP || ps == ParseExpressionState::U_OP;
}

bool canParseBinaryOperator(const ParseExpressionState &ps) { return ps == ParseExpressionState::NO_OP; }

/*
 * 将单元运算符的运算附加到operand，例如：
 * * & id
 * u_opers = stack [* &], id = operand
 */
std::unique_ptr<Expression> AttachUnaryOperators(std::unique_ptr<Expression> operand, std::stack<Token> &u_opers) {
  while (u_opers.empty() == false) {
    std::unique_ptr<UnaryExpr> tmp(new UnaryExpr());
    tmp->SetOp(u_opers.top());
    u_opers.pop();
    tmp->SetOperand(std::move(operand));
    operand = std::move(tmp);
  }
  return operand;
}

static void PushBoperators(std::stack<Token> &b_operators, std::stack<std::unique_ptr<Expression>> &operands,
                           Token current_token) {
  while (b_operators.empty() == false &&
         Operator::Instance().GetLevel(current_token) <= Operator::Instance().GetLevel(b_operators.top())) {
    std::unique_ptr<BinaryExpr> bin_expr(new BinaryExpr());
    if (operands.size() < 2 || b_operators.empty() == true) {
      throw WamonException("parse expression error");
    }
    bin_expr->SetOp(b_operators.top());
    b_operators.pop();

    bin_expr->SetRight(std::move(operands.top()));
    operands.pop();

    bin_expr->SetLeft(std::move(operands.top()));
    operands.pop();
    operands.push(std::move(bin_expr));
  }
  b_operators.push(current_token);
}

// 目前支持：
//  - 函数调用表达式
//  - 字面量表达式（5种基本类型）
//  - 方法调用表达式
//  - 成员调用表达式
//  - lambda表达式
//  - id表达式
//  - new表达式
//  - alloc\dealloc表达式
//  - 表达式嵌套（二元运算，括号）
std::unique_ptr<Expression> ParseExpression(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                            size_t end) {
  assert(begin <= end);
  if (begin == end) {
    return nullptr;
  }
  std::stack<std::unique_ptr<Expression>> operands;
  std::stack<Token> b_operators;
  std::stack<Token> u_operators;
  ParseExpressionState parse_state = ParseExpressionState::BEGIN;
  for (size_t i = begin; i < end; ++i) {
    Token current_token = tokens[i].token;
    if (current_token == Token::LEFT_PARENTHESIS) {
      size_t match_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, i);
      auto expr = ParseExpression(pu, tokens, i + 1, match_parent);
      operands.push(AttachUnaryOperators(std::move(expr), u_operators));
      i = match_parent;
      parse_state = ParseExpressionState::NO_OP;
      continue;
    }
    if (canParseUnaryOperator(parse_state) && Operator::Instance().FindUnary(current_token) == true) {
      u_operators.push(current_token);
      parse_state = ParseExpressionState::U_OP;
      continue;
    }
    if (canParseBinaryOperator(parse_state) && Operator::Instance().FindBinary(current_token) == true) {
      PushBoperators(b_operators, operands, current_token);
      parse_state = ParseExpressionState::B_OP;
      if (current_token == Token::AS) {
        ++i;
        auto type = ParseType(pu, tokens, i);
        std::unique_ptr<TypeExpr> type_expr(new TypeExpr());
        type_expr->SetType(std::move(type));
        operands.push(AttachUnaryOperators(std::move(type_expr), u_operators));
        i -= 1;
        parse_state = ParseExpressionState::NO_OP;
      }
    } else if (canParseBinaryOperator(parse_state) && current_token == Token::LEFT_BRACKETS) {
      // var_name [ nested_expr ]
      //                        i
      // 将其等价为 var_name [] nested_expr
      // 像普通二元运算符一样处理，根据优先级
      PushBoperators(b_operators, operands, Token::SUBSCRIPT);
      size_t right_bracket = FindMatchedToken<Token::LEFT_BRACKETS, Token::RIGHT_BRACKETS>(tokens, i);
      auto nested_expr = ParseExpression(pu, tokens, i + 1, right_bracket);
      i = right_bracket;
      operands.push(std::move(nested_expr));
      parse_state = ParseExpressionState::NO_OP;
    } else {
      parse_state = ParseExpressionState::NO_OP;
      // 函数调用表达式
      // call expression : [method_name] ( param_list )
      //   i             tmp            tmp2         tmp3
      if (current_token == Token::CALL) {
        size_t tmp = FindNextToken<Token::COLON, false, true>(tokens, i + 1);
        auto caller = ParseExpression(pu, tokens, i + 1, tmp);
        size_t tmp2 = tmp + 1;
        if (AssertToken(tokens, tmp2, Token::LEFT_PARENTHESIS)) {
          size_t tmp3 = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp2 - 1);
          auto param_list = ParseExprList(pu, tokens, tmp2 - 1, tmp3);
          std::unique_ptr<FuncCallExpr> func_call_expr(new FuncCallExpr());
          func_call_expr->SetCaller(std::move(caller));
          func_call_expr->SetParameters(std::move(param_list));
          operands.push(AttachUnaryOperators(std::move(func_call_expr), u_operators));
          i = tmp3;
        } else {
          auto [_, method_name] = ParseIdentifier<true>(pu, tokens, tmp2);
          size_t tmp3 = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp2);
          auto param_list = ParseExprList(pu, tokens, tmp2, tmp3);
          std::unique_ptr<MethodCallExpr> method_call_expr(new MethodCallExpr());
          method_call_expr->SetCaller(std::move(caller));
          method_call_expr->SetMethodName(method_name);
          method_call_expr->SetParameters(std::move(param_list));
          operands.push(AttachUnaryOperators(std::move(method_call_expr), u_operators));
          i = tmp3;
        }
        continue;
      }
      // 字面量表达式
      if (current_token == Token::STRING_ITERAL) {
        std::unique_ptr<StringIteralExpr> str_iter_expr(new StringIteralExpr());
        str_iter_expr->SetStringIter(tokens[i].Get<std::string>());

        operands.push(AttachUnaryOperators(std::move(str_iter_expr), u_operators));
        continue;
      }
      if (current_token == Token::INT_ITERAL) {
        std::unique_ptr<IntIteralExpr> int_iter_expr(new IntIteralExpr());
        int_iter_expr->SetIntIter(tokens[i].Get<int64_t>());

        operands.push(AttachUnaryOperators(std::move(int_iter_expr), u_operators));
        continue;
      }
      if (current_token == Token::BYTE_ITERAL) {
        std::unique_ptr<ByteIteralExpr> byte_iter_expr(new ByteIteralExpr());
        byte_iter_expr->SetByteIter(tokens[i].Get<uint8_t>());

        operands.push(AttachUnaryOperators(std::move(byte_iter_expr), u_operators));
        continue;
      }
      if (current_token == Token::DOUBLE_ITERAL) {
        std::unique_ptr<DoubleIteralExpr> double_iter_expr(new DoubleIteralExpr());
        double_iter_expr->SetDoubleIter(tokens[i].Get<double>());

        operands.push(AttachUnaryOperators(std::move(double_iter_expr), u_operators));
        continue;
      }
      if (current_token == Token::TRUE || current_token == Token::FALSE) {
        std::unique_ptr<BoolIteralExpr> bool_iter_expr(new BoolIteralExpr());
        bool_iter_expr->SetBoolIter(current_token == Token::TRUE ? true : false);
        operands.push(AttachUnaryOperators(std::move(bool_iter_expr), u_operators));
        continue;
      }
      // enum 字面量表达式
      // enum enum_type : enum_item
      if (current_token == Token::ENUM) {
        i += 1;
        std::unique_ptr<EnumIteralExpr> enum_expr(new EnumIteralExpr());
        auto type = ParseType(pu, tokens, i);
        AssertTokenOrThrow(tokens, i, Token::COLON, __FILE__, __LINE__);
        auto enum_item = ParseIdentifier<true>(pu, tokens, i);
        enum_expr->SetEnumType(std::move(type));
        enum_expr->SetEnumItem(enum_item.second);
        i -= 1;
        operands.push(AttachUnaryOperators(std::move(enum_expr), u_operators));
        continue;
      }
      if (current_token == Token::SELF) {
        std::unique_ptr<SelfExpr> self_expr(new SelfExpr());
        operands.push(AttachUnaryOperators(std::move(self_expr), u_operators));
        continue;
      }
      if (current_token == Token::LAMBDA) {
        std::unique_ptr<LambdaExpr> lambda_expr(new LambdaExpr());
        std::string lambda_func_name = ParseLambda(pu, tokens, i);
        lambda_expr->SetLambdaFuncName(lambda_func_name);
        i -= 1;
        operands.push(AttachUnaryOperators(std::move(lambda_expr), u_operators));
        continue;
      }
      if (current_token == Token::NEW) {
        std::unique_ptr<NewExpr> new_expr(new NewExpr());
        i += 1;
        auto type = ParseType(pu, tokens, i);
        size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, i);
        auto expr_list = ParseExprList(pu, tokens, i, right_parent);
        new_expr->SetNewType(std::move(type));
        new_expr->SetParameters(std::move(expr_list));
        i = right_parent;
        operands.push(AttachUnaryOperators(std::move(new_expr), u_operators));
        continue;
      }
      if (current_token == Token::ALLOC) {
        std::unique_ptr<AllocExpr> alloc_expr(new AllocExpr());
        i += 1;
        auto type = ParseType(pu, tokens, i);
        size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, i);
        auto expr_list = ParseExprList(pu, tokens, i, right_parent);
        alloc_expr->SetAllocType(std::move(type));
        alloc_expr->SetParameters(std::move(expr_list));
        i = right_parent;
        operands.push(AttachUnaryOperators(std::move(alloc_expr), u_operators));
        continue;
      }
      if (current_token == Token::DEALLOC) {
        std::unique_ptr<DeallocExpr> dealloc_expr(new DeallocExpr());
        auto expr = ParseExpression(pu, tokens, i + 1, end);
        dealloc_expr->SetDeallocParam(std::move(expr));
        i = end;
        operands.push(AttachUnaryOperators(std::move(dealloc_expr), u_operators));
        continue;
      }
      size_t tmp = i;
      auto [package_name, var_name] = ParseIdentifier(pu, tokens, tmp);
      // id表达式
      std::unique_ptr<IdExpr> id_expr(new IdExpr());
      id_expr->SetPackageName(package_name);
      id_expr->SetId(var_name);
      i = tmp - 1;
      operands.push(AttachUnaryOperators(std::move(id_expr), u_operators));
    }
  }
  while (b_operators.empty() == false) {
    if (operands.size() < 2 || b_operators.empty() == true) {
      throw WamonException("parse expression error");
    }
    std::unique_ptr<BinaryExpr> bin_expr(new BinaryExpr());
    bin_expr->SetOp(b_operators.top());
    b_operators.pop();

    bin_expr->SetRight(std::move(operands.top()));
    operands.pop();

    bin_expr->SetLeft(std::move(operands.top()));
    operands.pop();
    operands.push(std::move(bin_expr));
  }
  if (operands.size() != 1 || b_operators.empty() != true || u_operators.empty() != true) {
    throw WamonException("parse expression error, operands.size() != 1");
  }
  if (parse_state != ParseExpressionState::NO_OP) {
    throw WamonException("parse expression error, parse_state != NO_OP");
  }
  return std::move(operands.top());
}

std::unique_ptr<Statement> TryToParseBlockStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                               size_t &next);

std::unique_ptr<Statement> TryToParseIfStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                            size_t &next) {
  std::unique_ptr<IfStmt> ret = std::make_unique<IfStmt>();
  if (AssertToken(tokens, begin, Token::IF) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_check = ParseExpression(pu, tokens, begin + 1, end);
  ret->SetCheck(std::move(expr_check));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_if = ParseStmtBlock(pu, tokens, begin, end);
  ret->SetIfStmt(std::move(stmt_if));

  begin = end + 1;
  while (true) {
    if (!AssertToken(tokens, begin, Token::ELIF)) {
      break;
    }
    end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    auto expr_check = ParseExpression(pu, tokens, begin + 1, end);
    begin = end + 1;
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
    ret->AddElifItem(std::move(expr_check), std::move(stmt_block));
    begin = end + 1;
  }
  if (AssertToken(tokens, begin, Token::ELSE)) {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_else = ParseStmtBlock(pu, tokens, begin, end);
    ret->SetElseStmt(std::move(stmt_else));
    begin = end + 1;
  }
  next = begin;
  return ret;
}

std::unique_ptr<Statement> TryToParseForStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                             size_t &next) {
  std::unique_ptr<ForStmt> ret = std::make_unique<ForStmt>();
  if (AssertToken(tokens, begin, Token::FOR) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  size_t tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  auto init = ParseExpression(pu, tokens, begin + 1, tnext);
  ret->SetInit(std::move(init));
  begin = tnext + 1;

  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  auto check = ParseExpression(pu, tokens, begin, tnext);
  ret->SetCheck(std::move(check));
  begin = tnext + 1;

  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  if (tnext != end) {
    throw WamonException("parse for stmt error");
  }
  auto update = ParseExpression(pu, tokens, begin, end);
  ret->SetUpdate(std::move(update));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto block = ParseStmtBlock(pu, tokens, begin, end);
  ret->SetBlock(std::move(block));
  next = end + 1;
  return ret;
}

std::unique_ptr<Statement> TryToParseWhileStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                               size_t &next) {
  std::unique_ptr<WhileStmt> ret(new WhileStmt());
  if (AssertToken(tokens, begin, Token::WHILE) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto check = ParseExpression(pu, tokens, begin + 1, end);
  ret->SetCheck(std::move(check));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
  ret->SetBlock(std::move(stmt_block));
  next = end + 1;
  return ret;
}

// wrapper for TryToParseVariableDeclaration
// 全局变量的包前缀会在Merge Package的时候加上
// 非全局变量在这里加上
std::unique_ptr<Statement> TryToParseVarDefStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                                size_t &next) {
  std::unique_ptr<Statement> ret = TryToParseVariableDeclaration(pu, tokens, begin);
  if (ret == nullptr) {
    return ret;
  }
  auto vn = static_cast<VariableDefineStmt *>(ret.get())->GetVarName();
  static_cast<VariableDefineStmt *>(ret.get())->SetVarName(pu.GetCurrentParsingPackage() + "$" + vn);
  next = begin;
  return ret;
}

std::unique_ptr<Statement> TryToParseSkipStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                              size_t &next) {
  std::unique_ptr<Statement> ret(nullptr);
  if (AssertToken(tokens, begin, Token::CONTINUE)) {
    ret.reset(new ContinueStmt());
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    next = begin;
  } else if (AssertToken(tokens, begin, Token::BREAK)) {
    ret.reset(new BreakStmt());
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    next = begin;
  } else if (AssertToken(tokens, begin, Token::RETURN)) {
    ret.reset(new ReturnStmt());
    if (AssertToken(tokens, begin, Token::SEMICOLON) == false) {
      auto end = FindNextToken<Token::SEMICOLON, false, true>(tokens, begin);
      auto expr = ParseExpression(pu, tokens, begin, end);
      next = end + 1;
      static_cast<ReturnStmt *>(ret.get())->SetReturn(std::move(expr));
    } else {
      next = begin;
    }
  }
  return ret;
}

std::unique_ptr<ExpressionStmt> ParseExprStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                              size_t &next) {
  // parse expr stmt.
  size_t colon = FindNextToken<Token::SEMICOLON, false, true>(tokens, begin);
  auto expr = ParseExpression(pu, tokens, begin, colon);
  next = colon + 1;
  std::unique_ptr<ExpressionStmt> expr_stmt(new ExpressionStmt());
  expr_stmt->SetExpr(std::move(expr));
  return expr_stmt;
}

// 从tokens[begin]开始解析一个语句，并更新next为下一次解析的开始位置
// 目前支持：
//  - if语句
//  - for语句
//  - while语句
//  - 变量定义语句
//  - 表达式语句
std::unique_ptr<Statement> ParseStatement(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                          size_t &next) {
  std::unique_ptr<Statement> ret(nullptr);
  ret = TryToParseBlockStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseIfStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseForStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseWhileStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseVarDefStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseSkipStmt(pu, tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  return ParseExprStmt(pu, tokens, begin, next);
}

std::unique_ptr<BlockStmt> ParseStmtBlock(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                          size_t end) {
  std::vector<std::unique_ptr<Statement>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE, __FILE__, __LINE__);
  while (begin < end) {
    size_t next = 0;
    auto stmt = ParseStatement(pu, tokens, begin, next);
    ret.push_back(std::move(stmt));
    begin = next;
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACE, __FILE__, __LINE__);
  auto tmp = std::unique_ptr<BlockStmt>(new BlockStmt());
  tmp->SetBlock(std::move(ret));
  return tmp;
}

std::unique_ptr<Statement> TryToParseBlockStmt(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                               size_t &next) {
  std::unique_ptr<BlockStmt> ret = std::make_unique<BlockStmt>();
  size_t tmp = begin;
  if (AssertToken(tokens, tmp, Token::LEFT_BRACE) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  ret = ParseStmtBlock(pu, tokens, begin, end);
  next = end + 1;
  return ret;
}

//   (  type id, type id, ... )
// begin                     end
std::vector<ParameterListItem> ParseParameterList(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t begin,
                                                  size_t end) {
  std::vector<ParameterListItem> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_PARENTHESIS, __FILE__, __LINE__);
  if (AssertToken(tokens, begin, Token::RIGHT_PARENTHESIS)) {
    return ret;
  }
  while (true) {
    auto type = ParseType(pu, tokens, begin);
    bool is_ref = false;
    if (AssertToken(tokens, begin, Token::REF)) {
      is_ref = true;
    }
    auto [package_name, id] = ParseIdentifier(pu, tokens, begin);
    id = package_name + "$" + id;
    ret.push_back({id, std::move(type), is_ref});
    bool succ = AssertToken(tokens, begin, Token::COMMA);
    if (succ == false) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS, __FILE__, __LINE__);
      break;
    }
  }
  if (begin != end + 1) {
    throw WamonException("parse parameter list error, {} != {}", begin, end + 1);
  }
  return ret;
}

//   (  expr1, expr2, expr3, ...   )
// begin                          end
std::vector<std::unique_ptr<Expression>> ParseExprList(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                       size_t begin, size_t end) {
  std::vector<std::unique_ptr<Expression>> ret;
  size_t current = begin;
  while (current != end) {
    size_t next = FindNextToken<Token::COMMA>(tokens, current, end);
    auto expr = ParseExpression(pu, tokens, current + 1, next);
    if (expr != nullptr) {
      ret.push_back(std::move(expr));
    }
    current = next;
  }
  return ret;
}

std::unique_ptr<FunctionDef> TryToParseFunctionDeclaration(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                           size_t &begin) {
  std::unique_ptr<FunctionDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::FUNC);
  if (succ == false) {
    return ret;
  }
  auto [_, func_name] = ParseIdentifier<true>(pu, tokens, begin);
  //
  ret.reset(new FunctionDef(func_name));
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(pu, tokens, begin, end);
  for (auto &&each : param_list) {
    //
    ret->AddParamList(std::move(each));
  }
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(pu, tokens, begin);
  //
  ret->SetReturnType(std::move(return_type));
  if (AssertToken(tokens, begin, Token::SEMICOLON)) {
    ret->SetBlockStmt(nullptr);
  } else {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
    //
    ret->SetBlockStmt(std::move(stmt_block));
    begin = end + 1;
  }
  return ret;
}

// operator +/-/*/()/... (Type id, int a, string b) -> int {
//  ...
// }
// call id(a, b);
std::unique_ptr<OperatorDef> TryToParseOperatorOverride(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                        size_t &begin, Token &token) {
  std::unique_ptr<OperatorDef> ret;
  bool succ = AssertToken(tokens, begin, Token::OPERATOR);
  if (!succ) {
    return ret;
  }
  if (AssertToken(tokens, begin, Token::LEFT_PARENTHESIS)) {
    token = Token::LEFT_PARENTHESIS;
    AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS, __FILE__, __LINE__);
    ret.reset(new OperatorDef(Token::LEFT_PARENTHESIS));
  } else {
    if (begin >= tokens.size()) {
      throw WamonException("parse operator override error");
    }
    Token op = tokens[begin].token;
    bool succ = Operator::Instance().CanBeOverride(op);
    if (succ == false) {
      throw WamonException("operator {} can't be override", GetTokenStr(op));
    }
    token = op;
    ret.reset(new OperatorDef(op));
    begin += 1;
  }
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(pu, tokens, begin, end);
  for (auto &&each : param_list) {
    //
    ret->AddParamList(std::move(each));
  }
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(pu, tokens, begin);
  //
  ret->SetReturnType(std::move(return_type));

  if (AssertToken(tokens, begin, Token::SEMICOLON)) {
    ret->SetBlockStmt(nullptr);
  } else {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
    //
    ret->SetBlockStmt(std::move(stmt_block));
    begin = end + 1;
  }
  return ret;
}

// destructor() {
//  ...
// }
// no param_list, no return_type, no function_name
std::unique_ptr<FunctionDef> TryToParseDestructor(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                  size_t &begin) {
  if (!AssertToken(tokens, begin, Token::DESTRUCTOR)) {
    return nullptr;
  }
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(pu, tokens, begin, end);
  begin = end + 1;
  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(pu, tokens, begin, end);
  begin = end + 1;
  if (!param_list.empty()) {
    throw WamonException("TryToParseDestructor error, params.size() == {}", param_list.size());
  }
  std::unique_ptr<FunctionDef> ret(new FunctionDef("__destructor"));
  ret->SetBlockStmt(std::move(stmt_block));
  ret->SetReturnType(GetVoidType());
  return ret;
}
// method type_name {
//   func method_name(param_list) -> return_type {
//     ...
//   }
// }

// 转换逻辑
// 将运算符token的字符串形式+参数列表类型的type_info编码到一起，作为方法名字，这样可以支持重载
std::unique_ptr<MethodDef> OperatorOverrideToMethod(const std::string &type_name, std::unique_ptr<OperatorDef> &&op) {
  std::string method_name = OperatorDef::CreateName(op);
  std::unique_ptr<MethodDef> ret = std::make_unique<MethodDef>(type_name, method_name);
  ret->param_list_ = std::move(op->param_list_);
  ret->block_stmt_ = std::move(op->block_stmt_);
  ret->return_type_ = std::move(op->return_type_);
  return ret;
}

std::unique_ptr<FunctionDef> OperatorOverrideToFunc(std::unique_ptr<OperatorDef> &&op) {
  std::string func_name = OperatorDef::CreateName(op);
  std::unique_ptr<FunctionDef> ret = std::make_unique<FunctionDef>(func_name);
  ret->param_list_ = std::move(op->param_list_);
  ret->block_stmt_ = std::move(op->block_stmt_);
  ret->return_type_ = std::move(op->return_type_);
  return ret;
}

std::unique_ptr<methods_def> TryToParseMethodDeclaration(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                         size_t &begin, std::string &type_name) {
  std::unique_ptr<methods_def> ret;
  bool succ = AssertToken(tokens, begin, Token::METHOD);
  if (succ == false) {
    return ret;
  }
  ret.reset(new methods_def());
  // 只允许为结构体类型自定义方法，这里需要判断是否为复合类型以及是否为内置类型。
  auto [package_name, typen] = ParseBasicType(pu, tokens, begin);
  type_name = typen;
  size_t end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  begin += 1;
  while (begin < end) {
    // 在 method块中依次解析方法
    auto method = TryToParseFunctionDeclaration(pu, tokens, begin);
    if (method != nullptr) {
      std::unique_ptr<MethodDef> md;
      md.reset(new MethodDef(type_name, std::move(method)));
      ret->emplace_back(std::move(md));
    } else {
      auto destructor = TryToParseDestructor(pu, tokens, begin);
      if (destructor != nullptr) {
        std::unique_ptr<MethodDef> md(new MethodDef(type_name, std::move(destructor)));
        ret->emplace_back(std::move(md));
        continue;
      }
      // 如果解析方法失败尝试解析运算符重载
      Token op_token;
      auto call_op = TryToParseOperatorOverride(pu, tokens, begin, op_token);
      if (call_op == nullptr) {
        throw WamonException("parse method error, invalid operator override and method {}", method->GetFunctionName());
      }
      // 目前仅支持()运算符重载
      if (op_token != Token::LEFT_PARENTHESIS) {
        throw WamonException("only support override () operator as struct's method from now on, error_info : {}, {}",
                             GetTokenStr(op_token), method->GetFunctionName());
      }
      // 对于成员函数的运算符重载，我们总是将其转换为方法
      ret->emplace_back(OperatorOverrideToMethod(type_name, std::move(call_op)));
    }
  }
  assert(begin == end);
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACE, __FILE__, __LINE__);
  return ret;
}

std::unique_ptr<StructDef> TryToParseStructDeclaration(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                       size_t &begin) {
  std::unique_ptr<StructDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::STRUCT);
  if (succ == false) {
    return ret;
  }
  bool is_trait = false;
  if (AssertToken(tokens, begin, Token::TRAIT)) {
    is_trait = true;
  }
  auto [_, struct_name] = ParseIdentifier<true>(pu, tokens, begin);
  ret.reset(new StructDef(struct_name, is_trait));
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE, __FILE__, __LINE__);
  while (AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto type = ParseType(pu, tokens, begin);
    auto [pack_name, field_name] = ParseIdentifier<true>(pu, tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    ret->AddDataMember(field_name, std::move(type));
  }
  return ret;
}

std::unique_ptr<EnumDef> TryToParseEnumDeclaration(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                   size_t &begin) {
  std::unique_ptr<EnumDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::ENUM);
  if (succ == false) {
    return ret;
  }
  auto [_, enum_name] = ParseIdentifier<true>(pu, tokens, begin);
  ret.reset(new EnumDef(enum_name));
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE, __FILE__, __LINE__);
  while (AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto [_, fn] = ParseIdentifier<true>(pu, tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    ret->AddEnumItem(fn);
  }
  return ret;
}

// let var_name : type = (expr_list); or
// let var_name : type = expr;
std::unique_ptr<VariableDefineStmt> TryToParseVariableDeclaration(PackageUnit &pu,
                                                                  const std::vector<WamonToken> &tokens,
                                                                  size_t &begin) {
  std::unique_ptr<VariableDefineStmt> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::LET);
  if (succ == false) {
    return ret;
  }
  bool is_ref = false;
  if (AssertToken(tokens, begin, Token::REF)) {
    is_ref = true;
  }
  auto [_, var_name] = ParseIdentifier<true>(pu, tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::COLON, __FILE__, __LINE__);
  auto type = ParseType(pu, tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::ASSIGN, __FILE__, __LINE__);
  ret.reset(new VariableDefineStmt());
  ret->SetType(std::move(type));
  ret->SetVarName(var_name);
  if (is_ref == true) {
    ret->SetRefTag();
  }
  // parse expr.
  if (!AssertToken(tokens, begin, Token::LEFT_PARENTHESIS)) {
    size_t end = FindNextToken<Token::SEMICOLON>(tokens, begin);
    auto expr = ParseExpression(pu, tokens, begin, end);
    begin = end + 1;
    ret->SetConstructors(std::move(expr));
    return ret;
  }
  // parse expr list.
  begin -= 1;
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_list = ParseExprList(pu, tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
  ret->SetConstructors(std::move(expr_list));
  return ret;
}

std::string ParsePackageName(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t &begin) {
  AssertTokenOrThrow(tokens, begin, Token::PACKAGE, __FILE__, __LINE__);
  auto [_, package_name] = ParseIdentifier<true>(pu, tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
  return package_name;
}

std::vector<std::string> ParseImportPackages(PackageUnit &pu, const std::vector<WamonToken> &tokens, size_t &begin) {
  std::vector<std::string> packages;
  while (true) {
    bool succ = AssertToken(tokens, begin, Token::IMPORT);
    if (succ == false) {
      break;
    }
    auto [_, package_name] = ParseIdentifier<true>(pu, tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    if (package_name == pu.GetCurrentParsingPackage()) {
      throw WamonException("ParseImportPackages error, can not import {} self", package_name);
    }
    packages.push_back(package_name);
  }
  return packages;
}

// 两种using模式：
// using pacakge_name
// using package_name::item
// 第一种导入包里所有的符号，第二种只导入指示出的符号
// 因为对同一个包的两种using可以同时存在，因此需要求交。
std::unordered_map<std::string, UsingItem> ParseUsingPackages(PackageUnit &pu, const std::vector<WamonToken> &tokens,
                                                              size_t &begin) {
  std::unordered_map<std::string, UsingItem> using_item;
  while (true) {
    bool succ = AssertToken(tokens, begin, Token::USING);
    if (succ == false) {
      break;
    }
    auto [v1, v2] = ParseIdentifier<true>(pu, tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    std::string using_packge;
    UsingItem::using_type type;
    std::string using_v;
    // 第一种using模式，v1是被填充为本包名称的，而using语句和import语句中不可以使用本包名称
    if (v1 == pu.GetCurrentParsingPackage()) {
      using_packge = v2;
      type = UsingItem::using_type::TOTAL;
    } else {
      // 第二种using模式
      using_packge = v1;
      type = UsingItem::using_type::ITEM;
      using_v = v2;
    }
    if (!pu.InImportPackage(using_packge) && !IsPreparedPakageName(using_packge)) {
      throw WamonException("ParseUsingPackages error, invalid package name {}", using_packge);
    }
    if (type == UsingItem::using_type::TOTAL) {
      using_item[using_packge].type = type;
    } else {
      using_item[using_packge].using_set.insert(using_v);
    }
  }
  return using_item;
}

// 解析一个package
// package的构成：
// package声明语句（必须且唯一，位于package首部）
// import语句（0或多个，位于package声明语句之后，其他之前）
// using语句（0或多个，位于import语句之后，其他之前）
// 函数声明、结构体声明、变量定义声明（均位于package作用域，顺序无关）
PackageUnit Parse(const std::vector<WamonToken> &tokens) {
  if (tokens.empty() == true || tokens.back().token != Token::TEOF) {
    throw WamonException("invalid tokens");
  }
  PackageUnit package_unit;
  size_t current_index = 0;
  std::string package_name = ParsePackageName(package_unit, tokens, current_index);
  package_unit.SetName(package_name);
  package_unit.SetCurrentParsingPackage(package_name);

  std::vector<std::string> import_packages = ParseImportPackages(package_unit, tokens, current_index);
  package_unit.SetImportPackage(import_packages);

  auto using_packages = ParseUsingPackages(package_unit, tokens, current_index);
  package_unit.SetUsingPackage(using_packages);

  while (current_index < tokens.size()) {
    WamonToken token = tokens[current_index];
    if (token.token == Token::TEOF) {
      break;
    }
    size_t old_index = current_index;
    auto func_def = TryToParseFunctionDeclaration(package_unit, tokens, current_index);
    if (func_def != nullptr) {
      package_unit.AddFuncDef(std::move(func_def));
      continue;
    }
    auto struct_def = TryToParseStructDeclaration(package_unit, tokens, current_index);
    if (struct_def != nullptr) {
      package_unit.AddStructDef(std::move(struct_def));
      continue;
    }
    auto enum_def = TryToParseEnumDeclaration(package_unit, tokens, current_index);
    if (enum_def != nullptr) {
      package_unit.AddEnumDef(std::move(enum_def));
      continue;
    }
    auto var_declaration = TryToParseVariableDeclaration(package_unit, tokens, current_index);
    if (var_declaration != nullptr) {
      package_unit.AddVarDef(std::move(var_declaration));
      continue;
    }
    std::string type_name;
    auto methods_declaration = TryToParseMethodDeclaration(package_unit, tokens, current_index, type_name);
    if (methods_declaration != nullptr) {
      package_unit.AddMethod(type_name, std::move(methods_declaration));
      continue;
    }
    // 运算符重载函数会被编码为一个特殊的函数名字，该名字由运算符和参数类型唯一确定
    Token op_token;
    auto operator_override = TryToParseOperatorOverride(package_unit, tokens, current_index, op_token);
    if (operator_override != nullptr) {
      package_unit.AddFuncDef(OperatorOverrideToFunc(std::move(operator_override)));
      continue;
    }
    if (old_index == current_index) {
      throw WamonException("parse error, invalid token {}", GetTokenStr(token.token));
    }
  }
  return package_unit;
}

}  // namespace wamon
