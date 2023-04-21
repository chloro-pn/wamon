#include <stack>

#include "wamon/context.h"
#include "wamon/ast.h"

namespace wamon {

class Interpreter {
 public:
  static Interpreter& Instance();

  Variable* GetVariableByName(const std::string& name);

  void Enter(std::unique_ptr<Context>&& c);

  void Leave();

  Context* GetCurrentContext();

  void RegisterGlobalVarDefStmt(std::unique_ptr<VariableDefineStmt>&& var_def_stmt);

  void Exec();

  std::unique_ptr<Variable> CallFunc(const std::string& func_name, const std::vector<Variable*>& vars);
  
 private:
  Context global_context_;
  std::stack<std::unique_ptr<Context>> context_stack_;
  std::vector<std::unique_ptr<VariableDefineStmt>> global_var_;
};

}