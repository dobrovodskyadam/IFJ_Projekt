// wrong number of arguments for list member

import ifj26.*

@main def main(): Unit =
  val items = List(1, 2, 3)
  items.slice(1,2,3,4)
  items.slice(1)
