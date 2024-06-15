### before
关于wamon中`identifier`的概念见 `doc/identifier_and_keyword.md`

### package
wamon使用包(`package`)的概念组织源码，任何源代码文件必须属于某个包，同时源码文件的第一行有效代码必须是包声明语句，通过该语句声明自己所在的包。
包声明语句：`package identifier;`
包声明语句之后是0个或多个包导入语句（包导入语句是可选的）。
包导入语句：`import identifier;`

接下来是一系列可以在包作用域中定义的实体，包括：
- 全局变量的定义
  变量定义语句见 `doc/expr_and_stmt.md`
- 结构体的定义
  结构体定义的语法如下：
  ```
  struct struct_name[type: identifier] {
    type field_name[type: identifier];
    type field_name[type: identifier];
    ...
  }
  ```
- 结构体方法的定义
  结构体方法定义的语法如下：
  ```
  method struct_name[type: identifier] {
    func method_name[type: identifier](param_list) -> type
      stmt_block
    
    operator()(param_list) -> type
      stmt_block

    destructor()
      stmt_block
  }
  ```
  `method`块中的三类方法均是可选的，其中:
  * `stmt_block`的含义见 `doc/expr_and_stmt`。
  * `param_list`的定义是，以逗号分割的`type var_name[type: identifier]`序列。
- 全局函数的定义
  全局函数定义的语法如下：
  ```
  func (param_list) -> type
    stmt_block
  ```
- enum的定义
  enum定义的语法如下：
  ```
  enum enum_name[type: identifier] {
    enum_item[type: identifier];
    enum_item[type: identifier];
    ...
  }
  ```
  enum字面量的语法如下：`enum enum_name : enum_item`，例如：`enum Color:Blue`

  ### 实体引用规则
  当引用同一个包内的实体时可以省略包名称，当引用其他包内的实体时需要在实体前加上包名称和作用域运算符(\:\:)，例如：
  * `call sort::merge_sort:(param_list)` 调用sort包内的merge_sort函数
  * `enum draw::Color:Red` 声明一个enum常量，enum的类型是包draw内的Color
  * `let v : f((int, int) -> int) = math::add;` 定义一个函数类型的变量，变量的值是math包的add全局函数。
  