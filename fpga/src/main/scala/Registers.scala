
package mlsdr

import chisel3._
import chisel3.experimental.{Analog, attach}

class Registers extends Module {
  val io = IO(new Bundle {
    val bus = new Bundle {
      val address = Input(UInt(8.W))
      val data = Analog(8.W)
      val rd = Input(Bool())
      val wr = Input(Bool())
    }

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

  def attachBus(module: Register) = {
    // TL; DR: Mass-assignment does not work with Analog stuff
    module.io.nreset := !reset
    module.io.clk := clock
    attach(module.io.data, io.bus.data)
    module.io.address := io.bus.address
    module.io.rd := io.bus.rd
    module.io.wr := io.bus.wr
  }

  def attachBus(module: RegisterConstant) = {
    // TODO: Cleanup this mess...
    module.io.nreset := !reset
    module.io.clk := clock
    attach(module.io.data, io.bus.data)
    module.io.address := io.bus.address
    module.io.rd := io.bus.rd
  }

  val regADCtl = Module(new Register(Address.ADCtl, 0x00))
  attachBus(regADCtl)
  io.adc.enable := regADCtl.io.out(0)
  io.adc.mode := regADCtl.io.out(2, 1)

  val BaseId = 0xf0
  for ((c, i) <- "atx".zipWithIndex) {
    val regId = Module(new RegisterConstant(Address.IdBase + i, c))
    attachBus(regId)
  }
  val regScratch = Module(new Register(Address.Scratchpad, 0x00, 0xaa))
  attachBus(regScratch)

  val regFeatures = Module(new RegisterConstant(Address.Features, bitMash(Seq(Config.hasTuner))))
  attachBus(regFeatures)
}
