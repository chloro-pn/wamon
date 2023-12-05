#include "wamon/parser.h"

#include <cassert>
#include <stack>
#include <stdexcept>

#include "fmt/format.h"
#include "wamon/lambda_function_set.h"
#include "wamon/method_def.h"
#include "wamon/operator.h"
#include "wamon/package_unit.h"

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
    throw WamonExecption("assert token error, assert {}, but {}, file : {}, line {}", GetTokenStr(token),
                         GetTokenStr(tokens[begin].token), file, line);
  }
  begin += 1;
}

std::string ParseIdentifier(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin || tokens[begin].token != Token::ID || tokens[begin].type != WamonToken::ValueType::STR) {
    throw WamonExecption("parse identifier error : {}", GetTokenStr(tokens[begin].token));
  }
  std::string ret = tokens[begin].Get<std::string>();
  begin += 1;
  return ret;
}

std::string ParseBasicType(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin) {
    throw WamonExecption("parse basic type error, invalid index {}", begin);
  }
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
    ret = ParseIdentifier(tokens, begin);
  }
  return ret;
}

// 从tokens[begin]开始解析一个类型信息，更新begin，并返回解析到的类型
// 仅从语法分析的角度进行解析，不一定是合法的类型。
std::unique_ptr<Type> ParseType(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (AssertToken(tokens, begin, Token::PTR)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    auto nested_type = ParseType(tokens, begin);
    auto tmp = std::make_unique<PointerType>(std::move(nested_type));
    if (right_parent != begin) {
      throw WamonExecption("parse ptr type error");
    }
    begin += 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::LIST)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    auto nested_type = ParseType(tokens, begin);
    std::unique_ptr<ListType> tmp(new ListType(std::move(nested_type)));
    begin = right_parent + 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::F)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    std::vector<std::unique_ptr<Type>> param_list;
    std::unique_ptr<Type> return_type;
    ParseTypeList(tokens, begin, right_parent, param_list, return_type);

    std::unique_ptr<FuncType> tmp(new FuncType(std::move(param_list), std::move(return_type)));
    begin = right_parent + 1;
    return tmp;
  }
  // parse basic type
  std::unique_ptr<Type> ret;
  std::string type_name = ParseBasicType(tokens, begin);
  ret = std::make_unique<BasicType>(type_name);
  return ret;
}

/*
 *  ( ( type1, type2, type3, ..., typen ) -> return_type )
 * begin                                              end
 */
void ParseTypeList(const std::vector<WamonToken> &tokens, size_t begin, size_t end,
                   std::vector<std::unique_ptr<Type>> &param_list, std::unique_ptr<Type> &return_type) {
  size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  begin += 1;
  while (begin != right_parent) {
    auto type = ParseType(tokens, begin);
    param_list.push_back(std::move(type));
    if (AssertToken(tokens, begin, Token::COMMA) == false) {
      break;
    }
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS, __FILE__, __LINE__);
  if (AssertToken(tokens, begin, Token::ARROW) == true) {
    return_type = ParseType(tokens, begin);
  } else {
    // bug fix : 如果没有指定也要初始化为void
    return_type = GetVoidType();
  }
  if (begin != end) {
    throw WamonExecption("parse type list error");
  }
}

/*
 * [ id1, id2, ... , idn ]
 */
void ParseCaptureIdList(const std::vector<WamonToken> &tokens, size_t begin, size_t end,
                        std::vector<std::string> &ids) {
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACKETS, __FILE__, __LINE__);
  while (true) {
    if (begin == end) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACKETS, __FILE__, __LINE__);
      break;
    }
    auto id = ParseIdentifier(tokens, begin);
    auto it = std::find(ids.begin(), ids.end(), id);
    if (it != ids.end()) {
      throw WamonExecption("ParseCpatureIdList error, duplicate id {}", id);
    }
    ids.push_back(id);
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
std::string ParseLambda(const std::vector<WamonToken> &tokens, size_t &begin) {
  auto name = LambdaExpr::CreateUniqueLambdaName();
  std::unique_ptr<FunctionDef> lambda_def(new FunctionDef(name));

  AssertTokenOrThrow(tokens, begin, Token::LAMBDA, __FILE__, __LINE__);
  size_t capture_end = FindMatchedToken<Token::LEFT_BRACKETS, Token::RIGHT_BRACKETS>(tokens, begin);
  std::vector<std::string> capture_ids;
  ParseCaptureIdList(tokens, begin, capture_end, capture_ids);
  lambda_def->SetCaptureIds(std::move(capture_ids));

  begin = capture_end + 1;
  size_t param_end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(tokens, begin, param_end);
  for (auto &&each : param_list) {
    lambda_def->AddParamList(std::move(each.second), each.first);
  }
  begin = param_end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(tokens, begin);
  lambda_def->SetReturnType(std::move(return_type));

  auto end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(tokens, begin, end);
  lambda_def->SetBlockStmt(std::move(stmt_block));

  LambdaFunctionSet::Instance().RegisterLambdaFunction(name, std::move(lambda_def));
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
      throw WamonExecption("parse expression error");
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
//  - 表达式嵌套（二元运算，括号）
std::unique_ptr<Expression> ParseExpression(const std::vector<WamonToken> &tokens, size_t begin, size_t end) {
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
      auto expr = ParseExpression(tokens, i + 1, match_parent);
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
        auto type = ParseType(tokens, i);
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
      auto nested_expr = ParseExpression(tokens, i + 1, right_bracket);
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
        auto caller = ParseExpression(tokens, i + 1, tmp);
        size_t tmp2 = tmp + 1;
        if (AssertToken(tokens, tmp2, Token::LEFT_PARENTHESIS)) {
          size_t tmp3 = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp2 - 1);
          auto param_list = ParseExprList(tokens, tmp2 - 1, tmp3);
          std::unique_ptr<FuncCallExpr> func_call_expr(new FuncCallExpr());
          func_call_expr->SetCaller(std::move(caller));
          func_call_expr->SetParameters(std::move(param_list));
          operands.push(AttachUnaryOperators(std::move(func_call_expr), u_operators));
          i = tmp3;
        } else {
          auto method_name = ParseIdentifier(tokens, tmp2);
          size_t tmp3 = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp2);
          auto param_list = ParseExprList(tokens, tmp2, tmp3);
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
      if (current_token == Token::SELF) {
        std::unique_ptr<SelfExpr> self_expr(new SelfExpr());
        operands.push(AttachUnaryOperators(std::move(self_expr), u_operators));
        continue;
      }
      if (current_token == Token::LAMBDA) {
        std::unique_ptr<LambdaExpr> lambda_expr(new LambdaExpr());
        std::string lambda_func_name = ParseLambda(tokens, i);
        lambda_expr->SetLambdaFuncName(lambda_func_name);
        i -= 1;
        operands.push(AttachUnaryOperators(std::move(lambda_expr), u_operators));
        continue;
      }
      size_t tmp = i;
      std::string var_name = ParseIdentifier(tokens, tmp);
      // id表达式
      std::unique_ptr<IdExpr> id_expr(new IdExpr());
      id_expr->SetId(var_name);
      operands.push(AttachUnaryOperators(std::move(id_expr), u_operators));
    }
  }
  while (b_operators.empty() == false) {
    if (operands.size() < 2 || b_operators.empty() == true) {
      throw WamonExecption("parse expression error");
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
    throw WamonExecption("parse expression error, operands.size() != 1");
  }
  if (parse_state != ParseExpressionState::NO_OP) {
    throw WamonExecption("parse expression error, parse_state != NO_OP");
  }
  return std::move(operands.top());
}

std::unique_ptr<Statement> TryToParseBlockStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next);

std::unique_ptr<Statement> TryToParseIfStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<IfStmt> ret = std::make_unique<IfStmt>();
  if (AssertToken(tokens, begin, Token::IF) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_check = ParseExpression(tokens, begin + 1, end);
  ret->SetCheck(std::move(expr_check));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_if = ParseStmtBlock(tokens, begin, end);
  ret->SetIfStmt(std::move(stmt_if));

  begin = end + 1;
  if (AssertToken(tokens, begin, Token::ELSE)) {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_else = ParseStmtBlock(tokens, begin, end);
    ret->SetElseStmt(std::move(stmt_else));
    begin = end + 1;
  }

  next = begin;
  return ret;
}

std::unique_ptr<Statement> TryToParseForStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<ForStmt> ret = std::make_unique<ForStmt>();
  if (AssertToken(tokens, begin, Token::FOR) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  size_t tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  auto init = ParseExpression(tokens, begin + 1, tnext);
  ret->SetInit(std::move(init));
  begin = tnext + 1;

  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  auto check = ParseExpression(tokens, begin, tnext);
  ret->SetCheck(std::move(check));
  begin = tnext + 1;

  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  if (tnext != end) {
    throw WamonExecption("parse for stmt error");
  }
  auto update = ParseExpression(tokens, begin, end);
  ret->SetUpdate(std::move(update));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto block = ParseStmtBlock(tokens, begin, end);
  ret->SetBlock(std::move(block));
  next = end + 1;
  return ret;
}

std::unique_ptr<Statement> TryToParseWhileStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<WhileStmt> ret(new WhileStmt());
  if (AssertToken(tokens, begin, Token::WHILE) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto check = ParseExpression(tokens, begin + 1, end);
  ret->SetCheck(std::move(check));
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(tokens, begin, end);
  ret->SetBlock(std::move(stmt_block));
  next = end + 1;
  return ret;
}

// wrapper for TryToParseVariableDeclaration
std::unique_ptr<Statement> TryToParseVarDefStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<Statement> ret = TryToParseVariableDeclaration(tokens, begin);
  next = begin;
  return ret;
}

std::unique_ptr<Statement> TryToParseSkipStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
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
      auto expr = ParseExpression(tokens, begin, end);
      next = end + 1;
      static_cast<ReturnStmt *>(ret.get())->SetReturn(std::move(expr));
    } else {
      next = begin;
    }
  }
  return ret;
}

std::unique_ptr<ExpressionStmt> ParseExprStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  // parse expr stmt.
  size_t colon = FindNextToken<Token::SEMICOLON, false, true>(tokens, begin);
  auto expr = ParseExpression(tokens, begin, colon);
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
std::unique_ptr<Statement> ParseStatement(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<Statement> ret(nullptr);
  ret = TryToParseBlockStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseIfStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseForStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseWhileStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseVarDefStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  ret = TryToParseSkipStmt(tokens, begin, next);
  if (ret != nullptr) {
    return ret;
  }
  return ParseExprStmt(tokens, begin, next);
}

std::unique_ptr<BlockStmt> ParseStmtBlock(const std::vector<WamonToken> &tokens, size_t begin, size_t end) {
  std::vector<std::unique_ptr<Statement>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE, __FILE__, __LINE__);
  while (begin < end) {
    size_t next = 0;
    auto stmt = ParseStatement(tokens, begin, next);
    ret.push_back(std::move(stmt));
    begin = next;
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACE, __FILE__, __LINE__);
  auto tmp = std::unique_ptr<BlockStmt>(new BlockStmt());
  tmp->SetBlock(std::move(ret));
  return tmp;
}

std::unique_ptr<Statement> TryToParseBlockStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<BlockStmt> ret = std::make_unique<BlockStmt>();
  size_t tmp = begin;
  if (AssertToken(tokens, tmp, Token::LEFT_BRACE) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  ret = ParseStmtBlock(tokens, begin, end);
  next = end + 1;
  return ret;
}

//   (  type id, type id, ... )
// begin                     end
std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken> &tokens,
                                                                              size_t begin, size_t end) {
  std::vector<std::pair<std::string, std::unique_ptr<Type>>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_PARENTHESIS, __FILE__, __LINE__);
  if (AssertToken(tokens, begin, Token::RIGHT_PARENTHESIS)) {
    return ret;
  }
  while (true) {
    auto type = ParseType(tokens, begin);
    std::string id = ParseIdentifier(tokens, begin);
    ret.push_back(std::pair<std::string, std::unique_ptr<Type>>(id, std::move(type)));
    bool succ = AssertToken(tokens, begin, Token::COMMA);
    if (succ == false) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS, __FILE__, __LINE__);
      break;
    }
  }
  if (begin != end + 1) {
    throw WamonExecption("parse parameter list error, {} != {}", begin, end + 1);
  }
  return ret;
}

//   (  expr1, expr2, expr3, ...   )
// begin                          end
std::vector<std::unique_ptr<Expression>> ParseExprList(const std::vector<WamonToken> &tokens, size_t begin,
                                                       size_t end) {
  std::vector<std::unique_ptr<Expression>> ret;
  size_t current = begin;
  while (current != end) {
    size_t next = FindNextToken<Token::COMMA>(tokens, current, end);
    auto expr = ParseExpression(tokens, current + 1, next);
    if (expr != nullptr) {
      ret.push_back(std::move(expr));
    }
    current = next;
  }
  return ret;
}

std::unique_ptr<FunctionDef> TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<FunctionDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::FUNC);
  if (succ == false) {
    return ret;
  }
  std::string func_name = ParseIdentifier(tokens, begin);
  //
  ret.reset(new FunctionDef(func_name));
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(tokens, begin, end);
  for (auto &each : param_list) {
    //
    ret->AddParamList(std::move(each.second), each.first);
  }
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(tokens, begin);
  //
  ret->SetReturnType(std::move(return_type));
  if (AssertToken(tokens, begin, Token::SEMICOLON)) {
    ret->SetBlockStmt(nullptr);
  } else {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_block = ParseStmtBlock(tokens, begin, end);
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
std::unique_ptr<OperatorDef> TryToParseOperatorOverride(const std::vector<WamonToken> &tokens, size_t &begin,
                                                        Token &token) {
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
      throw WamonExecption("parse operator override error");
    }
    Token op = tokens[begin].token;
    bool succ = Operator::Instance().CanBeOverride(op);
    if (succ == false) {
      throw WamonExecption("operator {} can't be override", GetTokenStr(op));
    }
    token = op;
    ret.reset(new OperatorDef(op));
    begin += 1;
  }
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(tokens, begin, end);
  for (auto &each : param_list) {
    //
    ret->AddParamList(std::move(each.second), each.first);
  }
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW, __FILE__, __LINE__);
  auto return_type = ParseType(tokens, begin);
  //
  ret->SetReturnType(std::move(return_type));

  if (AssertToken(tokens, begin, Token::SEMICOLON)) {
    ret->SetBlockStmt(nullptr);
  } else {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    auto stmt_block = ParseStmtBlock(tokens, begin, end);
    //
    ret->SetBlockStmt(std::move(stmt_block));
    begin = end + 1;
  }
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

std::unique_ptr<methods_def> TryToParseMethodDeclaration(const std::vector<WamonToken> &tokens, size_t &begin,
                                                         std::string &type_name) {
  std::unique_ptr<methods_def> ret;
  bool succ = AssertToken(tokens, begin, Token::METHOD);
  if (succ == false) {
    return ret;
  }
  ret.reset(new methods_def());
  // 只允许为结构体类型自定义方法，这里需要判断是否为复合类型以及是否为内置类型。
  type_name = ParseBasicType(tokens, begin);
  size_t end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  begin += 1;
  while (begin < end) {
    // 在 method块中依次解析方法
    auto method = TryToParseFunctionDeclaration(tokens, begin);
    if (method != nullptr) {
      std::unique_ptr<MethodDef> md;
      md.reset(new MethodDef(type_name, std::move(method)));
      ret->emplace_back(std::move(md));
    } else {
      // 如果解析方法失败尝试解析运算符重载
      Token op_token;
      auto call_op = TryToParseOperatorOverride(tokens, begin, op_token);
      if (call_op == nullptr) {
        throw WamonExecption("parse method error, invalid operator override and method {}", method->GetFunctionName());
      }
      // 目前仅支持()运算符重载
      if (op_token != Token::LEFT_PARENTHESIS) {
        throw WamonExecption("only support override () operator as struct's method from now on, error_info : {}, {}",
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

std::unique_ptr<StructDef> TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<StructDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::STRUCT);
  if (succ == false) {
    return ret;
  }
  bool is_trait = false;
  if (AssertToken(tokens, begin, Token::TRAIT)) {
    is_trait = true;
  }
  std::string struct_name = ParseIdentifier(tokens, begin);
  ret.reset(new StructDef(struct_name, is_trait));
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE, __FILE__, __LINE__);
  while (AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto type = ParseType(tokens, begin);
    auto field_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    ret->AddDataMember(field_name, std::move(type));
  }
  return ret;
}

// let var_name : type = (expr_list);
std::unique_ptr<VariableDefineStmt> TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens,
                                                                  size_t &begin) {
  std::unique_ptr<VariableDefineStmt> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::LET);
  if (succ == false) {
    return ret;
  }
  std::string var_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::COLON, __FILE__, __LINE__);
  auto type = ParseType(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::ASSIGN, __FILE__, __LINE__);
  // parse expr list.
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_list = ParseExprList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
  ret.reset(new VariableDefineStmt());
  ret->SetType(std::move(type));
  ret->SetVarName(var_name);
  ret->SetConstructors(std::move(expr_list));
  return ret;
}

std::string ParsePackageName(const std::vector<WamonToken> &tokens, size_t &begin) {
  AssertTokenOrThrow(tokens, begin, Token::PACKAGE, __FILE__, __LINE__);
  std::string package_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
  return package_name;
}

std::vector<std::string> ParseImportPackages(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::vector<std::string> packages;
  while (true) {
    bool succ = AssertToken(tokens, begin, Token::IMPORT);
    if (succ == false) {
      break;
    }
    std::string package_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON, __FILE__, __LINE__);
    packages.push_back(package_name);
  }
  return packages;
}

// 解析一个package
// package的构成：
// package声明语句（必须且唯一，位于package首部）
// import语句（0或多个，位于package声明语句之后，其他之前）
// 函数声明、结构体声明、变量定义声明（均位于package作用域，顺序无关）
PackageUnit Parse(const std::vector<WamonToken> &tokens) {
  if (tokens.empty() == true || tokens.back().token != Token::TEOF) {
    throw WamonExecption("invalid tokens");
  }
  PackageUnit package_unit;
  size_t current_index = 0;
  std::string package_name = ParsePackageName(tokens, current_index);
  package_unit.SetName(package_name);

  std::vector<std::string> import_packages = ParseImportPackages(tokens, current_index);
  package_unit.SetImportPackage(import_packages);

  while (current_index < tokens.size()) {
    WamonToken token = tokens[current_index];
    if (token.token == Token::TEOF) {
      break;
    }
    size_t old_index = current_index;
    auto func_def = TryToParseFunctionDeclaration(tokens, current_index);
    if (func_def != nullptr) {
      package_unit.AddFuncDef(std::move(func_def));
      continue;
    }
    auto struct_def = TryToParseStructDeclaration(tokens, current_index);
    if (struct_def != nullptr) {
      package_unit.AddStructDef(std::move(struct_def));
      continue;
    }
    auto var_declaration = TryToParseVariableDeclaration(tokens, current_index);
    if (var_declaration != nullptr) {
      package_unit.AddVarDef(std::move(var_declaration));
      continue;
    }
    std::string type_name;
    auto methods_declaration = TryToParseMethodDeclaration(tokens, current_index, type_name);
    if (methods_declaration != nullptr) {
      package_unit.AddMethod(type_name, std::move(methods_declaration));
      continue;
    }
    // 运算符重载函数会被编码为一个特殊的函数名字，该名字由运算符和参数类型唯一确定
    Token op_token;
    auto operator_override = TryToParseOperatorOverride(tokens, current_index, op_token);
    if (operator_override != nullptr) {
      package_unit.AddFuncDef(OperatorOverrideToFunc(std::move(operator_override)));
      continue;
    }
    if (old_index == current_index) {
      throw WamonExecption("parse error, invalid token {}", GetTokenStr(token.token));
    }
  }

  package_unit.AddAllLambdaFunction(LambdaFunctionSet::Instance().GetAllLambdas());

  return package_unit;
}

}  // namespace wamon
