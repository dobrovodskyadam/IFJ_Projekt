// classic recursion with an unspecified return type

import ifj26.*

def factorial(n: Int) =
  if n > 0 then
    factorial(n - 1) * n
  else
    1

@main def main(): Unit =
  print(toStr(factorial(5)))
