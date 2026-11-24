// condition expression in if value is not of a type boolean

import ifj26.*

@main def main(): Unit =
  val x = 3
  val result = if x then 1 else 0
  print(toStr(result))
