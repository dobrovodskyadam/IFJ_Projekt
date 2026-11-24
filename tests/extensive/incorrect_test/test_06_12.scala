// assignment returns Unit but the function's declared return type is Int

import ifj26.*

def value(n: Int): Int =
  val doubled = n * 2

@main def main(): Unit =
  print(toStr(value(1)))
