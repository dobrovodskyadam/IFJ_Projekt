// returns a Double but the function's declared return type is Int

import ifj26.*

def half(n: Int): Int =
  n / 2.0

@main def main(): Unit =
  print(toStr(half(4)))
