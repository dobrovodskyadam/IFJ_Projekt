// guard expression is not of a type boolean

import ifj26.*

@main def main(): Unit =
  for
    i <- 1 to 5
    if i + 1
  do
    print(toStr(i))
