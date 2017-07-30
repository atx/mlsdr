
package mlsdr

import chisel3._
import chisel3.experimental.{Analog, attach}

class Registers extends Module {
  val io = IO(new Bundle {
    val bus = new RegisterBus

    val adc = new Bundle {
      val enable = Output(Bool())
      val mode = Output(UInt(2.W))
    }
  })

  object Address {
    val ADCtl = 0x01
    val IdBase = 0xf0
    val Scratchpad = 0xf3
    val Features = 0xf5
  }

  def bitMash(bits: Seq[Boolean]) = (bits.foldLeft(0) { (v, x) => (v << 1) | (if (x) 1 else 0) })

  val regADCtl = Module(new MaskedRegister(Address.ADCtl))
  regADCtl.bus <> io.bus
  io.adc.enable := regADCtl.io.value(0)
  io.adc.mode := regADCtl.io.value(2, 1)

  for ((c, i) <- "atx".zipWithIndex) {
    val regId = Module(new ConstantRegister(Address.IdBase + i, c))
    regId.bus <> io.bus
  }

  val regScratch = Module(new MaskedRegister(Address.Scratchpad, 0x00, 0xaa))
  regScratch.bus <> io.bus

  val regFeatures = Module(new ConstantRegister(Address.Features, bitMash(Seq(Config.hasTuner))))
  regFeatures.bus <> io.bus
}
