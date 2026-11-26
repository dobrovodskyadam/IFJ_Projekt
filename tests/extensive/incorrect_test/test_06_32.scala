// wrong type retuned from return branch

import ifj26.*

def find(limit: Int): Int =
  for i <- 1 to limit do
    if i > 2 then
      return "found"
  0

@main def main(): Unit =
  print(toStr(find(5)))
