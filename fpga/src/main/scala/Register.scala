

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.experimental._

class RegisterBus extends Bundle {
  val address = Input(UInt(8.W))
  val data = Analog(8.W)
  val rd = Input(Bool())
  val wr = Input(Bool())

  def <>(other: RegisterBus) = {
    // Standard mass-assignment breaks with Analog for some reason
    address <> other.address
    attach(data, other.data)
    rd <> other.rd
    wr <> other.wr
  }
}

class BaseRegister(val address: Int) extends MultiIOModule {
  val bus = IO(new RegisterBus)

  val data = Module(new Tristate(UInt(8.W)))
  attach(data.io.tristate, bus.data)

  val selected = bus.address === address.U
  val readRequested = selected && bus.rd
  val writeRequested = selected && bus.wr
}

class ConstantRegister(address: Int, val value: Int) extends BaseRegister(address) {
  data.io.driven := readRequested
  data.io.value := value.U
}

class MaskedRegister(address: Int, val mask: Int = 0x00, val initval: Int = 0x00) extends BaseRegister(address) {
  val io = IO(new Bundle {
    val maskValues = Input(UInt(8.W))
    val value = Output(UInt(8.W))
  })

  val internal = RegInit(initval.U(8.W))

  io.value := (internal & ~(mask.U(8.W))) | (io.maskValues & mask.U(8.W))

  data.io.driven := readRequested
  data.io.value := io.value

  when (writeRequested) {
    internal := data.io.out
  }
}
