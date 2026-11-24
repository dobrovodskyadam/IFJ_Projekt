// wrong number of args

import ifj26.*

def add(a: Int, b: Int): Int = a + b

@main def main(): Unit =
  add(1,2,3)
  add(1)
