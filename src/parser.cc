#include "wamon/parser.h"

#include <cassert>
#include <stack>
#include <stdexcept>

#include "fmt/format.h"
#include "wamon/operator.h"
#include "wamon/package_unit.h"

#include <iostream>

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
void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    throw WamonExecption("assert token error, assert {}, but {}", GetTokenStr(token), GetTokenStr(tokens[begin].token));
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
    std::unique_ptr<PointerType> tmp(new PointerType);
    auto nested_type = ParseType(tokens, begin);
    tmp->SetHoldType(std::move(nested_type));
    if (right_parent != begin) {
      throw WamonExecption("parse ptr type error");
    }
    begin += 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::ARRAY)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    std::unique_ptr<ArrayType> tmp(new ArrayType);
    auto nested_type = ParseType(tokens, begin);
    tmp->SetHoldType(std::move(nested_type));
    AssertTokenOrThrow(tokens, begin, Token::COMMA);
    auto count_expr = ParseExpression(tokens, begin, right_parent);
    tmp->SetCount(std::move(count_expr));

    begin = right_parent + 1;
    return tmp;
  } else if (AssertToken(tokens, begin, Token::F)) {
    size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
    begin += 1;
    std::unique_ptr<FuncType> tmp(new FuncType);
    std::vector<std::unique_ptr<Type>> param_list;
    std::unique_ptr<Type> return_type;
    ParseTypeList(tokens, begin, right_parent, param_list, return_type);

    tmp->SetParamTypeAndReturnType(std::move(param_list), std::move(return_type));
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
 *  ( type1, type2, type3, ..., typen ) -> return_type )
 * begin                                              end
 */
void ParseTypeList(const std::vector<WamonToken>& tokens, size_t begin, size_t end, std::vector<std::unique_ptr<Type>>& param_list, std::unique_ptr<Type>& return_type) {
  size_t right_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  begin += 1;
  while(begin != right_parent) {
    auto type = ParseType(tokens, begin);
    param_list.push_back(std::move(type));
    if(AssertToken(tokens, begin, Token::COMMA) == false) {
      break;
    }
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS);
  if(AssertToken(tokens, begin, Token::ARROW) == true) {
    return_type = ParseType(tokens, begin);
  }
  if (begin != end) {
    throw WamonExecption("parse type list error");
  }
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

bool canParseUnaryOperator(const ParseExpressionState& ps) {
  return ps == ParseExpressionState::BEGIN || ps == ParseExpressionState::B_OP || ps == ParseExpressionState::U_OP;
}

bool canParseBinaryOperator(const ParseExpressionState& ps) {
  return ps == ParseExpressionState::NO_OP;
}

/*
 * 将单元运算符的运算附加到operand，例如：
 * * & id
 * u_opers = stack [* &], id = operand
 */
std::unique_ptr<Expression> AttachUnaryOperators(std::unique_ptr<Expression> operand, std::stack<Token>& u_opers) {
  while(u_opers.empty() == false) {
    std::unique_ptr<UnaryExpr> tmp(new UnaryExpr);
    tmp->SetOp(u_opers.top());
    u_opers.pop();
    tmp->SetOperand(std::move(operand));
    operand = std::move(tmp);
  }
  return operand;
}

// 目前支持：
//  - 函数调用表达式
//  - 字面量表达式（5种基本类型）
//  - 方法调用表达式
//  - 成员调用表达式
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
      parse_state = ParseExpressionState::B_OP;
    } else {
      parse_state = ParseExpressionState::NO_OP;
      // 函数调用表达式
      if (current_token == Token::CALL) {
        std::unique_ptr<FuncCallExpr> func_call_expr(new FuncCallExpr());
        size_t tmp = i + 1;
        std::string func_name = ParseIdentifier(tokens, tmp);
        func_call_expr->SetFuncName(func_name);

        size_t tmp_end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp);
        auto expr_list = ParseExprList(tokens, tmp, tmp_end);
        func_call_expr->SetParameters(std::move(expr_list));
        operands.push(AttachUnaryOperators(std::move(func_call_expr), u_operators));
        i = tmp_end;
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

      size_t tmp = i;
      std::string var_name = ParseIdentifier(tokens, tmp);
      // id表达式
      std::unique_ptr<IdExpr> id_expr(new IdExpr());
      id_expr->SetId(var_name);
      operands.push(AttachUnaryOperators(std::move(id_expr), u_operators));

      if (AssertToken(tokens, tmp, Token::LEFT_BRACKETS)) {
        b_operators.push(Token::SUBSCRIPT);
        // var_name [ nested_expr ]
        //                        i
        size_t right_bracket = FindMatchedToken<Token::LEFT_BRACKETS, Token::RIGHT_BRACKETS>(tokens, tmp - 1);
        i = right_bracket;
        auto nested_expr = ParseExpression(tokens, tmp, right_bracket);
        std::unique_ptr<IndexExpr> index_expr(new IndexExpr());
        index_expr->SetName(var_name);
        index_expr->SetNestedExpr(std::move(nested_expr));
        operands.push(AttachUnaryOperators(std::move(index_expr), u_operators));
        continue;
      }
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

std::unique_ptr<Statement> TryToParseWhileStmt(const std::vector<WamonToken>& tokens, size_t begin, size_t &next) {
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
std::unique_ptr<Statement> TryToParseVarDefStmt(const std::vector<WamonToken>& tokens, size_t begin, size_t &next) {
  std::unique_ptr<Statement> ret = TryToParseVariableDeclaration(tokens, begin);
  next = begin;
  return ret;
}

std::unique_ptr<Statement> TryToParseSkipStmt(const std::vector<WamonToken>& tokens, size_t begin, size_t &next) {
  std::unique_ptr<Statement> ret(nullptr);
  if (AssertToken(tokens, begin, Token::CONTINUE)) {
    ret.reset(new ContinueStmt());
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
    next = begin;
  } else if (AssertToken(tokens, begin, Token::BREAK)) {
    ret.reset(new BreakStmt());
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
    next = begin;
  } else if (AssertToken(tokens, begin, Token::RETURN)) {
    ret.reset(new ReturnStmt());
    if (AssertToken(tokens, begin, Token::SEMICOLON) == false) {
      auto end = FindNextToken<Token::SEMICOLON>(tokens, begin);
      auto expr = ParseExpression(tokens, begin, end);
      next = end + 1;
      static_cast<ReturnStmt*>(ret.get())->SetReturn(std::move(expr));
    } else {
      next = begin;
    }
  }
  return ret;
}

std::unique_ptr<ExpressionStmt> ParseExprStmt(const std::vector<WamonToken>& tokens, size_t begin, size_t &next) {
  // parse expr stmt.
  size_t colon = FindNextToken<Token::SEMICOLON>(tokens, begin);
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

std::unique_ptr<BlockStmt> ParseStmtBlock(const std::vector<WamonToken> &tokens, size_t begin,
                                                       size_t end) {
  std::vector<std::unique_ptr<Statement>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE);
  while (begin < end) {
    size_t next = 0;
    auto stmt = ParseStatement(tokens, begin, next);
    ret.push_back(std::move(stmt));
    begin = next;
  }
  AssertTokenOrThrow(tokens, begin, Token::RIGHT_BRACE);
  auto tmp = std::unique_ptr<BlockStmt>(new BlockStmt());
  tmp->SetBlock(std::move(ret));
  return tmp;
}

//   (  type id, type id, ... )
// begin                     end
std::vector<std::pair<std::string, std::unique_ptr<Type>>> ParseParameterList(const std::vector<WamonToken> &tokens,
                                                                              size_t begin, size_t end) {
  std::vector<std::pair<std::string, std::unique_ptr<Type>>> ret;
  AssertTokenOrThrow(tokens, begin, Token::LEFT_PARENTHESIS);
  if (AssertToken(tokens, begin, Token::RIGHT_PARENTHESIS)) {
    return ret;
  }
  while (true) {
    auto type = ParseType(tokens, begin);
    std::string id = ParseIdentifier(tokens, begin);
    ret.push_back(std::pair<std::string, std::unique_ptr<Type>>(id, std::move(type)));
    bool succ = AssertToken(tokens, begin, Token::COMMA);
    if (succ == false) {
      AssertTokenOrThrow(tokens, begin, Token::RIGHT_PARENTHESIS);
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
    ret.push_back(std::move(expr));
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
  for(auto& each : param_list) {
    //
    ret->AddParamList(std::move(each.second), each.first);
  }
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW);
  auto return_type = ParseType(tokens, begin);
  //
  ret->SetReturnType(std::move(return_type));
  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(tokens, begin, end);

  //
  ret->SetBlockStmt(std::move(stmt_block));

  begin = end + 1;
  return ret;
}

std::unique_ptr<StructDef> TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<StructDef> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::STRUCT);
  if (succ == false) {
    return ret;
  }
  std::string struct_name = ParseIdentifier(tokens, begin);
  ret.reset(new StructDef(struct_name));
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE);
  while (AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto type = ParseType(tokens, begin);
    auto field_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
    ret->AddDataMember(field_name, std::move(type));
  }
  return ret;
}

// let var_name : type = (expr_list);
std::unique_ptr<VariableDefineStmt> TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<VariableDefineStmt> ret(nullptr);
  bool succ = AssertToken(tokens, begin, Token::LET);
  if (succ == false) {
    return ret;
  }
  std::string var_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::COLON);
  auto type = ParseType(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::ASSIGN);
  // parse expr list.
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_list = ParseExprList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  ret.reset(new VariableDefineStmt());
  ret->SetType(type->GetTypeInfo());
  ret->SetVarName(var_name);
  ret->SetConstructors(std::move(expr_list));
  return ret;
}



std::string ParsePackageName(const std::vector<WamonToken>& tokens, size_t &begin) {
  AssertTokenOrThrow(tokens, begin, Token::PACKAGE);
  std::string package_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  return package_name;
}

std::vector<std::string> ParseImportPackages(const std::vector<WamonToken>& tokens, size_t &begin) {
  std::vector<std::string> packages;
  while(true) {
    bool succ = AssertToken(tokens, begin, Token::IMPORT);
    if (succ == false) {
      break;
    }
    std::string package_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
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
    if (old_index == current_index) {
      throw WamonExecption("parse error");
    }
  }
  return package_unit;
}

}  // namespace wamon