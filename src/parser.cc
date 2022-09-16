#include "wamon/parser.h"

#include <stack>
#include <stdexcept>

#include "fmt/format.h"
#include "wamon/operator.h"

namespace wamon {
/*
 * @brief 判断tokens[begin]处的token值 == token
 */
bool AssertToken(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    return false;
  }
  begin += 1;
  return true;
}

void AssertTokenOrThrow(const std::vector<WamonToken> &tokens, size_t &begin, Token token) {
  if (tokens.size() <= begin || tokens[begin].token != token) {
    throw std::runtime_error(
        fmt::format("parse token error, {} != {}", GetTokenStr(token), GetTokenStr(tokens[begin].token)));
  }
  begin += 1;
}

std::string ParseIdentifier(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin || tokens[begin].token != Token::ID || tokens[begin].type != WamonToken::ValueType::STR) {
    throw std::runtime_error("parse identifier error");
  }
  std::string ret = tokens[begin].Get<std::string>();
  begin += 1;
  return ret;
}

std::string ParseBasicType(const std::vector<WamonToken> &tokens, size_t &begin) {
  if (tokens.size() <= begin) {
    throw std::runtime_error("parse basictype error");
  }
  std::string ret;
  if (tokens[begin].token == Token::STRING) {
    ret = "string";
  } else if (tokens[begin].token == Token::INT) {
    ret = "int";
  } else if (tokens[begin].token == Token::BYTE) {
    ret = "byte";
  } else if (tokens[begin].token == Token::BOOL) {
    ret = "bool";
  } else if (tokens[begin].token == Token::DOUBLE) {
    ret = "double";
  } else if (tokens[begin].token == Token::VOID) {
    ret = "void";
  }
  if (ret != "") {
    begin += 1;
  }
  return ret;
}

// 从tokens[begin]开始解析一个类型信息，更新begin，并返回解析到的类型
std::unique_ptr<Type> ParseType(const std::vector<WamonToken> &tokens, size_t &begin) {
  std::unique_ptr<Type> ret;
  std::string type_name = ParseBasicType(tokens, begin);
  if (type_name == "") {
    type_name = ParseIdentifier(tokens, begin);
  }
  ret = std::make_unique<BasicType>(type_name);
  // todo 支持复合类型
  return ret;
}

std::unique_ptr<Expression> ParseExpression(const std::vector<WamonToken> &tokens, size_t begin, size_t end) {
  if (begin == end) {
    return nullptr;
  }
  std::stack<std::unique_ptr<Expression>> operands;
  std::stack<Token> b_operators;
  std::stack<Token> u_operators;
  for (size_t i = begin; i < end; ++i) {
    Token current_token = tokens[i].token;
    if (current_token == Token::LEFT_PARENTHESIS) {
      size_t match_parent = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, i);
      auto expr = ParseExpression(tokens, i + 1, match_parent);
      operands.push(std::move(expr));
      i = match_parent;
      continue;
    }
    if (Operator::Instance().FindBinary(current_token) == true) {
      while (b_operators.empty() == false &&
             Operator::Instance().GetLevel(current_token) <= Operator::Instance().GetLevel(b_operators.top())) {
        std::unique_ptr<BinaryExpr> bin_expr(new BinaryExpr());
        if (operands.size() < 2 || b_operators.empty() == true) {
          throw std::runtime_error("parse expression error");
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
    } else {
      // 函数调用表达式
      if (current_token == Token::CALL) {
        std::unique_ptr<FuncCallExpr> func_call_expr(new FuncCallExpr());
        size_t tmp = i + 1;
        std::string func_name = ParseIdentifier(tokens, tmp);
        func_call_expr->SetFuncName(func_name);

        size_t tmp_end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp);
        auto expr_list = ParseExprList(tokens, tmp, tmp_end);
        func_call_expr->SetParameters(std::move(expr_list));

        operands.push(std::move(func_call_expr));
        i = tmp_end;
        continue;
      }
      // 字面量表达式
      if (current_token == Token::STRING_ITERAL) {
        std::unique_ptr<StringIteralExpr> str_iter_expr(new StringIteralExpr());
        str_iter_expr->SetStringIter(tokens[i].Get<std::string>());

        operands.push(std::move(str_iter_expr));
        continue;
      }
      if (current_token == Token::INT_ITERAL) {
        std::unique_ptr<IntIteralExpr> int_iter_expr(new IntIteralExpr());
        int_iter_expr->SetIntIter(tokens[i].Get<int64_t>());

        operands.push(std::move(int_iter_expr));
        continue;
      }
      if (current_token == Token::BYTE_ITERAL) {
        std::unique_ptr<ByteIteralExpr> byte_iter_expr(new ByteIteralExpr());
        byte_iter_expr->SetByteIter(tokens[i].Get<uint8_t>());

        operands.push(std::move(byte_iter_expr));
        continue;
      }
      // id表达式
      if (current_token != Token::ID) {
        throw std::runtime_error("parse expression error");
      }
      size_t tmp_i = i;
      std::string var_name = ParseIdentifier(tokens, tmp_i);
      size_t tmp = i + 1;
      if (AssertToken(tokens, tmp, Token::DECIMAL_POINT)) {
        std::string member_name = ParseIdentifier(tokens, tmp);
        if (AssertToken(tokens, tmp, Token::LEFT_PARENTHESIS)) {
          // 方法调用
          size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, tmp - 1);
          auto expr_list = ParseExprList(tokens, tmp, end);
          i = end;
          std::unique_ptr<MethodExpr> method_expr(new MethodExpr());
          method_expr->SetVarName(var_name);
          method_expr->SetMethodName(member_name);
          method_expr->SetParamList(std::move(expr_list));
          operands.push(std::move(method_expr));
          continue;
        } else {
          // 数据成员调用
          // a    .    b
          // i   i+1  i+2
          i = i + 2;
          std::unique_ptr<DataMemberExpr> data_member_expr(new DataMemberExpr());
          data_member_expr->SetVarName(var_name);
          data_member_expr->SetDataMemberName(member_name);
          operands.push(std::move(data_member_expr));
          continue;
        }
      }
      // 普通的id表达式
      std::unique_ptr<IdExpr> id_expr(new IdExpr());
      id_expr->SetId(var_name);

      operands.push(std::move(id_expr));
    }
  }
  while (b_operators.empty() == false) {
    if (operands.size() < 2 || b_operators.empty() == true) {
      throw std::runtime_error("parse expression error");
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
  if (operands.size() != 1) {
    throw std::runtime_error("parse expression error");
  }
  return std::move(operands.top());
}

std::unique_ptr<Statement> TryToParseIfStmt(const std::vector<WamonToken> &tokens, size_t begin, size_t &next) {
  std::unique_ptr<IfStmt> ret = std::make_unique<IfStmt>();
  if (AssertToken(tokens, begin, Token::IF) == false) {
    return nullptr;
  }
  auto end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  ParseExpression(tokens, begin + 1, end);
  begin = end + 1;

  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  ParseStmtBlock(tokens, begin, end);

  begin = end + 1;
  if (AssertToken(tokens, begin, Token::ELSE)) {
    end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
    ParseStmtBlock(tokens, begin, end);
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
  ParseExpression(tokens, begin + 1, tnext);
  begin = tnext + 1;
  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  ParseExpression(tokens, begin, tnext);
  begin = tnext + 1;
  tnext = FindNextToken<Token::SEMICOLON>(tokens, begin, end);
  if (tnext != end) {
    throw std::runtime_error("parse for stmt error");
  }
  ParseExpression(tokens, begin, end);
  begin = end + 1;
  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  ParseStmtBlock(tokens, begin, end);
  next = end + 1;
  return ret;
}

// 从tokens[begin]开始解析一个语句，并更新next为下一次解析的开始位置
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
  // parse expr stmt.
  size_t colon = FindNextToken<Token::SEMICOLON>(tokens, begin);
  auto expr = ParseExpression(tokens, begin, colon);
  next = colon + 1;
  std::unique_ptr<ExpressionStmt> expr_stmt(new ExpressionStmt());
  expr_stmt->SetExpr(std::move(expr));
  return expr_stmt;
}

std::vector<std::unique_ptr<Statement>> ParseStmtBlock(const std::vector<WamonToken> &tokens, size_t begin,
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
  return ret;
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
    throw std::runtime_error(fmt::format("parse parameter list error, {} != {}", begin, end + 1));
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

void TryToParseFunctionDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  bool succ = AssertToken(tokens, begin, Token::FUNC);
  if (succ == false) {
    return;
  }
  std::string func_name = ParseIdentifier(tokens, begin);
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto param_list = ParseParameterList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::ARROW);
  auto return_type = ParseType(tokens, begin);
  end = FindMatchedToken<Token::LEFT_BRACE, Token::RIGHT_BRACE>(tokens, begin);
  auto stmt_block = ParseStmtBlock(tokens, begin, end);
  begin = end + 1;
  return;
}

void TryToParseStructDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  bool succ = AssertToken(tokens, begin, Token::STRUCT);
  if (succ == false) {
    return;
  }
  std::string struct_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::LEFT_BRACE);
  while (AssertToken(tokens, begin, Token::RIGHT_BRACE) == false) {
    auto type = ParseType(tokens, begin);
    auto field_name = ParseIdentifier(tokens, begin);
    AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  }
  return;
}

// let var_name : type = (expr_list) | expr;
void TryToParseVariableDeclaration(const std::vector<WamonToken> &tokens, size_t &begin) {
  bool succ = AssertToken(tokens, begin, Token::LET);
  if (succ == false) {
    return;
  }
  std::string var_name = ParseIdentifier(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  auto type = ParseType(tokens, begin);
  AssertTokenOrThrow(tokens, begin, Token::ASSIGN);
  // parse expr list.
  size_t end = FindMatchedToken<Token::LEFT_PARENTHESIS, Token::RIGHT_PARENTHESIS>(tokens, begin);
  auto expr_list = ParseExprList(tokens, begin, end);
  begin = end + 1;
  AssertTokenOrThrow(tokens, begin, Token::SEMICOLON);
  return;
}

void Parse(const std::vector<WamonToken> &tokens) {
  if (tokens.empty() == true || tokens.back().token != Token::TEOF) {
    throw std::runtime_error("invalid tokens");
  }
  size_t current_index = 0;
  while (current_index < tokens.size()) {
    WamonToken token = tokens[current_index];
    if (token.token == Token::TEOF) {
      break;
    }
    size_t old_index = current_index;
    TryToParseFunctionDeclaration(tokens, current_index);
    TryToParseStructDeclaration(tokens, current_index);
    TryToParseVariableDeclaration(tokens, current_index);
    if (old_index == current_index) {
      throw std::runtime_error("parse error");
    }
  }
}

}  // namespace wamon