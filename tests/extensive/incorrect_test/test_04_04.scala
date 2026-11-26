// shadowing and multiple redefinitions

import ifj26.*

@main def main(): Unit =
  val i = 0
  if i + 1 > 0 then
    val i = 1
  val i = 2
