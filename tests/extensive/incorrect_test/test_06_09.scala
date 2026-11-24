// return in a function without a declared return type

import ifj26.*

def value(n: Int) =
  return n + 1

@main def main(): Unit =
  print(toStr(value(1)))
