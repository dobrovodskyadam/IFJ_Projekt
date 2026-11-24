// branches of if values with mismatching types

import ifj26.*

@main def main(): Unit =
  val x = 3
  val result = if x > 2 then "big" else 0
  print(toStr(result))
