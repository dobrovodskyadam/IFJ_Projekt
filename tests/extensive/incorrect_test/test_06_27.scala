// list literal with mismatching type

import ifj26.*

@main def main(): Unit =
  val items = List(1, "two", 3)
  print(toStr(items))
