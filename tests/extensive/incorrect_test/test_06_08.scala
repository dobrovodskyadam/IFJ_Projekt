// indirect recursion between functions with undeclared return types

import ifj26.*

def foo(n: Int) = if n == 0 then goo(n-1) else -1

def goo(n: Int) = if n == 0 then foo(n-1) else 1

@main def main(): Unit =
  foo(3)
