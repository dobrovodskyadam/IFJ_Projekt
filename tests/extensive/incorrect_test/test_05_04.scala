// member access with wrong type

import ifj26.*

@main def main(): Unit =
  val items = List(1, 2, 3)
  items.drop("1")
