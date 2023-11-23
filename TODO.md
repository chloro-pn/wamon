### todo

* call表达式目前只能调用id(params)和id.method_name(params)的语法形式，需要支持
  - call expression ((parmas))
  - call expression : method_name ((params))
  这种语法形式，这样可以通过call和((以及:这些token确定expression的边界
* 将Operator和BuiltinFunction等全局单例对象重构为解释器持有对象，因此不同的解释器实例可以注册不同的运算符重载和cpp函数，在构造的时候可以继承内置的一些重载和函数。这也意味着语法分析和语义分析需要依赖解释器（比如查找某些函数是否被定义：通过源文件或者注册定义）
* 支持move关键字与move操作
* 支持闭包
* 支持包管理
* 为解释器实例提供更多的功能（执行表达式，打印当前运行栈、支持增量parse功能）