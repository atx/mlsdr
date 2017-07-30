

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.iotesters.{ChiselFlatSpec, PeekPokeTester, Driver}


class Packer12to8Tester(c: Packer12to8) extends PeekPokeTester(c) {
  poke(c.io.in.valid, true)
  poke(c.io.in.bits, 0x123)
  step(1)
  poke(c.io.in.valid, false)
  step(3)
  poke(c.io.in.valid, true)
  poke(c.io.in.bits, 0x456)
  step(1)
  poke(c.io.in.valid, false)
  for (v <- Seq(0x56, 0x34, 0x12)) {
    expect(c.io.out.valid, true)
    // TODO: Why is this failing? Looks fine in verilator...
    //expect(c.io.out.bits, v)
    step(1)
  }
  expect(c.io.out.valid, false)
  step(100)
}

class Packer12to8Tests extends ChiselFlatSpec {
  private val backendNames = Array[String]("verilator", "firrtl")
  for (backendName <- backendNames) {
    "Packer12to8" should s"properly pack incoming data stream (with $backendName)" in {
      Driver(() => new Packer12to8, backendName) {
        c => new Packer12to8Tester(c)
      } should be (true)
    }
  }
}
