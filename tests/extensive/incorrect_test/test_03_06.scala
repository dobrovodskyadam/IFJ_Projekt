// assign of undefined but defined later
// Expected exit code: 3

import ifj26.*

@main def main(): Unit =
  val a = b
  val b = a

  print("ahojda")
