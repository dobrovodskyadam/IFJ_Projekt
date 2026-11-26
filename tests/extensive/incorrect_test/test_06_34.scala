// concatenation used on wrong types

import ifj26.*

@main def main(): Unit =
  val items = 1 ++ List(1)
  print(toStr(items))
