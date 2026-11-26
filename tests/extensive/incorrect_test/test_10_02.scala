// zero step with by

import ifj26.*

@main def main(): Unit =
  for i <- 0 until 10 by 0 do
    print(toStr(i))
