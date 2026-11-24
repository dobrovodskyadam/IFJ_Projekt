// invalid type in function call (cannot convert implicitly Double => Int)

import ifj26.*

def twice(n: Int): Int = n * 2

@main def main(): Unit =
  twice(1.5)
