#pragma once

#include "wamon/ast.h"
#include "wamon/type.h"
#include "wamon/context.h"

#include <set>

namespace wamon {

class StaticAnalyzer;

class TypeChecker {
 public:
  friend void CheckBlockStatement(TypeChecker& tc, BlockStmt* stmt);
  friend std::unique_ptr<Type> CheckAndGetSSResultType(const TypeChecker& tc, BinaryExpr* binary_expr);
  friend std::unique_ptr<Type> CheckAndGetMemberAccessResultType(const TypeChecker& tc, BinaryExpr* binary_expr);
  friend std::unique_ptr<Type> CheckAndGetMethodReturnType(const TypeChecker& tc, const MethodDef* method, const MethodCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetFuncReturnType(const TypeChecker& tc, const FunctionDef* function, const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckParamTypeAndGetResultTypeForFunction(const TypeChecker& tc, FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetCallableReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, const FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetOperatorOverrideReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, FuncCallExpr* call_expr);
  friend std::unique_ptr<Type> CheckAndGetBuiltinMethodReturnType(const TypeChecker& tc, const std::unique_ptr<Type>& ctype, const MethodCallExpr* call_expr);
  friend class BuiltinFunctions;

  explicit TypeChecker(StaticAnalyzer& sa);

  // 检测函数、结构体、方法定义中所涉及的所有类型是否合法
  // 因为定义是顺序无关的，因此解析的时候无法判别类型是否被定义，因此需要在解析后的某个阶段进行全局的类型合法性校验
  // 此外，一些特殊的类型在语法上是合法的，但是语义上不合法，例如f(void, int) -> int，函数参数类型不能是void类型
  // 这应该是类型检测的第一个阶段
  // 注意：全局变量和局部变量的类型合法性检测不在这个阶段进行，因为在语句合法性检测的时候所有函数结构体方法和全局变量都被解析，因此可以在语句合法性检测中检测
  void CheckTypes();

  void CheckType(const std::unique_ptr<Type>& type, const std::string& context_info, bool can_be_void = false);

  // 检测全局变量的定义语句是否合法，全局变量的定义在包内是顺序相关的。
  // 这应该是类型检测的第二个阶段（因为表达式合法性检测依赖名字查找，需要全局作用域的名字获得类型）
  void CheckAndRegisterGlobalVariable();

  // 检测结构体是否存在循环依赖
  // 这应该是类型检测的第三个阶段
  void CheckStructs();

  // 检测函数定义是否合法，包括参数和返回值类型是否合法，函数块内每条语句是否合法，以及确定性返回的检测
  // 这应该是类型检测的第四个阶段
  void CheckFunctions();

  // 检测方法定义是否合法，包括参数和返回值类型是否合法，函数块内每条语句是否合法，以及确定性返回的检测
  // 这应该是类型检测的第四个阶段
  // tudo:和函数定义不同，方法中可以存在self表达式，后续可以支持对结构体的数据成员做一些修饰以提供特别的性质，比如:
  //   可以给某个数据成员修饰const属性，使得其不可被修改（这个还需要类型系统中指针类型相应的适配）；
  //   可以给某个数据成员修饰private属性，使得只有某些特别的成员函数才能访问；
  //   可以给某个数据成员修饰no_addr属性，使得不允许对该数据成员进行取地址操作，以保证不会有垂悬指针的问题；
  // 基于以上原因，将方法检测与函数检测分开实现，尽管目前它们并无太大区别
  void CheckMethods();

  // 检测运算符重载是否合法，包括参数和返回值类型是否合法，块内每条语句是否合法，以及确定性返回的检测
  // 此外还有运算符重载相关的特别规定，比如：
  //    重载函数的参数不应该都是内置类型；
  //    目前仅支持为基本类型提供运算符重载（函数类型、数组类型和指针类型均不支持重载）；
  //    除了调用运算符之外的重载，最多只应该有两个参数（二元运算符）,并且不能没有参数；
  //    某些运算符不支持重载（例如赋值运算符，我们总是希望它按照内置逻辑进行不应该由用户重载）；
  // 这应该是类型检测的第四个阶段
  void CheckOperatorOverride();

  const StaticAnalyzer& GetStaticAnalyzer() const {
    return static_analyzer_;
  }

  StaticAnalyzer& GetStaticAnalyzer() {
    return static_analyzer_;
  }

 private:
  StaticAnalyzer& static_analyzer_;

  // 对表达式树进行后序遍历，根据子节点的情况检测每个节点的类型是否合法
  std::unique_ptr<Type> GetExpressionType(Expression* expr) const;

  void CheckStatement(Statement* stmt);

  // 对函数体进行确定性返回分析
  // 定义 基本块：
  // 基本块是指没有分支结构的语句块
  // 定义 语句块：
  // BlockStmt的别称，指任何合法语句构成的代码块
  // 一个语句块是决定性返回的，如果它满足以下性质：
  //   其代码块中包含这样一条return语句：该return语句之后不存在分支结构的语句块，或者
  //   其最后一个分支语句的所有语句块都是决定性返回的
  // 注：
  //   由于循环语句可能是运行时不执行的，因此其对决定性返回这一性质没有影响（循环语句内可以包含return语句也可以不包含）
  //   对于最基本的BlockStmt，仅影响局部变量的定义域而不改变执行流，因此可以直接合并到上级BlockStmt中进行分析，当然为了实现间接也可以作为独立的基本块进行分析
  //   如果分支语句只有if分支而没有else分支，则其对决定性返回性质没有影响（因为它也可能是运行时不执行的）
  void CheckDeterministicReturn(FunctionDef* func);

  void CheckDeterministicReturn(MethodDef* method);

  bool IsDeterministicReturn(BlockStmt* basic_block);
};

}