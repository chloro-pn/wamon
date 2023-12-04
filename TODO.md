### todo

* [done] call表达式目前只能调用id(params)和id.method_name(params)的语法形式，需要支持
  - call expression :(parmas)
  - call expression : method_name (params)
  这种语法形式，这样可以通过call和:这些token确定expression的边界
* 将Operator和BuiltinFunction等全局单例对象重构为解释器持有对象，因此不同的解释器实例可以注册不同的运算符重载和cpp函数，在构造的时候可以继承内置的一些重载和函数。这也意味着语法分析和语义分析需要依赖解释器（比如查找某些函数是否被定义：通过源文件或者注册定义）
* [done] 支持move关键字与move操作
* [doing] 支持闭包
  - 闭包的语法 ： lambda [capture_variables] (param_list) { block stmt };
  - 重构Parser函数，将PackageUnit传递进去以供注册lambda函数
  - 类型检测阶段，需要根据捕获id查找对应的类型并注册在当前栈顶
  - 运行阶段，当lambda表达式用于初始化、赋值给一个函数类型的变量时，需要在当前上下文执行捕获操作并保存在变量中，对该变量执行调用操作时需要将捕获的数据注册在运行时栈中。
* 支持包管理
* 为解释器实例提供更多的功能（执行表达式，打印当前运行栈、支持增量parse功能）
* 重构，将operator运算的类型检测从type_checker移动到operator类中
* [done] 支持trait(参考rust)
* 变量ConstructByFields成员支持复制构造，目前检测阶段可以通过但是运行时不支持，会抛出异常
* CheckTraitConstraint失败时，返回具体的失败原因