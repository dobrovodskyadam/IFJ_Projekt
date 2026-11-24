// cycle variable used elsewhere
// Expected exit code: 3

import ifj26.*

@main def main(): Unit =
  for i <- 1 to 3 do
    print("AHOJ")

  var j = i * i
