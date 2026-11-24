// variable is defined in block but access outside

import ifj26.*

@main def main(): Unit =
  val i = 10
  if true then
    val j = 12

  val k = i * j
