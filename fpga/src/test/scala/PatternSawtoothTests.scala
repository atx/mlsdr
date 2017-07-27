

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.iotesters.{ChiselFlatSpec, PeekPokeTester, Driver}


class PatternSawtoothTesterSimpleRead(c: PatternSawtooth) extends PeekPokeTester(c) {
  poke(c.io.enable, true)
  var next = 0;
  for (x <- 0 to 10000) {
    if (peek(c.io.out.valid) == 1) {
      expect(c.io.out.bits, next)
      next = (next + 1) % 256
    }
    step(1)
  }
}

class PatternSawtoothTesterEnableTest(c: PatternSawtooth) extends PeekPokeTester(c) {
  poke(c.io.enable, false)
  for (_ <- 0 to 100) {
    step(1)
    expect(c.io.out.valid, false)
  }
}

class PatternSawtoothTests extends ChiselFlatSpec {
  private val backendNames = Array[String]("verilator", "firrtl")
  for (backendName <- backendNames) {
    "PatternSawtooth" should s"properly generate a sawtooth wave (with $backendName)" in {
      Driver(() => new PatternSawtooth(8, 8), backendName) {
        c => new PatternSawtoothTesterSimpleRead(c)
      } should be (true)
    }

    "PatternSawtooth" should s"not output data when disabled (with $backendName)" in {
      Driver(() => new PatternSawtooth(8, 8), backendName) {
        c => new PatternSawtoothTesterEnableTest(c)
      } should be (true)
    }
  }
}
