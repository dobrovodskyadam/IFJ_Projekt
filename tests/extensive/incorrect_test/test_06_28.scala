// concatenation on list with mismathcing element types

import ifj26.*

@main def main(): Unit =
  val items = List(1, 2) ++ List("a")
  print(toStr(items))
