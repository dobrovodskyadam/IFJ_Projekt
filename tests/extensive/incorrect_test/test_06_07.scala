// classic recursion with an unspecified return type

import ifj26.*

def factorial(n: Int) =
  if n > 0 then
    return countdown(n - 1) * n
  else
    return 1

@main def main(): Unit =
  factorial()
