// bare return in a function with a declared return type

import ifj26.*

def value(n: Int): Int =
  if n > 0 then
    return
  n

@main def main(): Unit =
  print(toStr(value(1)))
