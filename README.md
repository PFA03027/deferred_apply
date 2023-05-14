# deferred_apply
## Classes and helper functions intended to hold functions and arguments for deferring function execution

This class(deferred_apply) separates function calling and actual function processing. @n 
By this functionalty, below behaviour will be possible.

function "A" called out side of function "B", then function "B" processes function "A" in side of function "B".

Usage:
auto da = make_deferred_apply( f, a, b, ... );
auto ret = da.apply();

Since it is intended for temporary storage, lvalue-referenced classes are referenced and not copied.
Instead, a dangling reference can occur at the time the argument is actually referenced.
Therefore, class instances and their copies should not be brought out of the created scope.

## 関数の実行を延期するために、関数と引数を保持することを目的としたクラスとヘルパ関数

一時的な保持を目的としているため、左辺値参照されたクラスは参照を保持する方式で、コピーしない。
代わりに、引数を実際に参照する時点でダングリング参照が発生する可能性がある。
よって、クラスのインスタンスやそのコピーを、生成したスコープの外に持ち出してはならない。

Usage:
auto da = make_deferred_apply( f, a, b, ... );
auto ret = da.apply();

da.apply()によって、f(a,b,...) が実行される。

apply()の再適用が可能かどうかは、引数をどのように渡したか、あるいはfの特性に依存する。
例えば、aが右辺値参照で引き渡された場合、1回目のapply()の適用によって、deferred_apply内で保持していた値が無効値となっている可能性がある。

apply()のための戻り値の型がテンプレート型である以外は、型情報が隠されているため、メンバ変数定義も容易になる。
一方で、deferred_applying_arguments<>とは異なり、動的に適用する関数fを変更することはできない。
