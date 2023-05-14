# deferred_apply
## Classe `deferred_apply<R>` and helper function `make_deferred_apply()` intended to hold functions and arguments for deferring function execution

This class(deferred_apply) separates function calling and actual function processing. @n 
By this functionalty, below behaviour will be possible.

function "A" called out side of function "B", then function "B" processes function "A" in side of function "B".

Usage:
```cpp
auto da = make_deferred_apply( f, a, b, ... );
auto ret = da.apply();
```

Execution of `f(a,b,...)` is deferred until `da.apply()` is called.

`deferred_apply<R>` is intended to temporarily hold arguments and defer function application, so
Values referenced by lvalues in arguments (class instances, etc.) adopt a method of retaining references and are not copied.
Instead, dangling references can occur when referencing arguments to function applications.
Therefore, class instances and their copies should not be brought out of the created scope.

Whether `apply()` can be reapplied depends on how the arguments are passed or on the properties of `f`.
For example, if a is passed by rvalue reference, the value held in `deferred_apply` may become invalid due to the first application of `apply()`.

Except that the return type for `apply()` is a template type, the type information is hidden, making it easier to define member variables.
On the other hand, unlike `deferred_applying_arguments<...>`, we cannot change the dynamically applied function f.

## 関数の実行を延期するために、関数と引数を保持することを目的としたクラス`deferred_apply<R>`とヘルパ関数`make_deferred_apply()`

`deferred_apply<R>`は、引数の一時的な保持と関数適用を延期することを目的としているため、
引数で左辺値参照された値(クラスのインスタンス等)は参照を保持する方式を採り、コピーしない。
代わりに、関数適用に引数を参照する時点でダングリング参照が発生する可能性がある。
よって、クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。

Usage:
```cpp
auto da = make_deferred_apply( f, a, b, ... );
auto ret = da.apply();
```

da.apply()が呼び出されるまで、f(a,b,...) の実行が延期される。

apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。
例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が無効値となっている可能性がある。

apply()のための戻り値の型がテンプレート型である以外は、型情報が隠されているため、メンバ変数定義も容易になる。
一方で、deferred_applying_arguments<...>とは異なり、動的に適用する関数fを変更することはできない。
