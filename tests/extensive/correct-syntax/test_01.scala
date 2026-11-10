// Nested Calls Test
import ifj26.*

def foo(n: Int, l: Int): Int = 1 + 2

def goo(n: Int) = n * 2

@main
def hoo(): Unit =
  val i: Double = 10.1
  val j: Double = 11.1

  if (i > 12 && j > 15) then
    print("Ahojda")
  else
    // var k = foo(goo(i), goo(j*2)) + goo(if i > 12 then 1 else 2)
    var k = foo(goo(i), goo(j*2)) + goo(11)
    print(k)
