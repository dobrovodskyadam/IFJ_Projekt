// assignment to function parameter

import ifj26.*

def inc(n: Int): Int =
  n = n + 1
  return n

@main def main(): Unit =
  var i = inc(5)
