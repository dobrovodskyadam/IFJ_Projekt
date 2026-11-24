// Aims at nested calls and expression parsing


import ifj26.*

def printLine(value: String): Unit =
  print(value)
  print("\n")

def foo(n: Int, l: Int): Int = 1 + 2

def goo(n: Int) = n * 2

def hoo(l: Double) = l * 1.1

@main
def loo(): Unit =
  var i: Double = 10.1
  val j: Double = 11.1

  i = i + 1

  hoo(12)

  if (i > 12 && j > 15) then
    printLine("Ahojda")
  else
    // var k = foo(goo(i), goo(j*2)) + goo(if i > 12 then 1 else 2)
    var k = foo(goo(toInt(i)), goo(toInt(j*2))) + goo(11)
    printLine(toStr(k))
