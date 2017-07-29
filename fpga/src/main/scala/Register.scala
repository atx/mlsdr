

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.experimental._

class Register(val address: Int, val mask: Int, val initval: Int = 0x00)
    extends BlackBox(Map("ADDRESS" -> address,
                         "MASK" -> mask,
                         "INITVAL" -> initval)) {
  val io = IO(new Bundle {
    val nreset = Input(Bool())
    val clk = Input(Clock())

    val address = Input(UInt(8.W))
    val data = Analog(8.W)
    val rd = Input(Bool())
    val wr = Input(Bool())

    val maskvals = Input(UInt(8.W))

    val out = Output(UInt(8.W))
  })

  override def desiredName = "register"
  suggestName(f"register_$address%02x")
}


class RegisterConstant(val address: Int, val value: Int)
    extends BlackBox(Map("ADDRESS" -> address,
                         "VALUE" -> value)) {
  val io = IO(new Bundle {
    val nreset = Input(Bool())
    val clk = Input(Clock())

    val address = Input(UInt(8.W))
    val data = Analog(8.W)
    val rd = Input(Bool())
  })

  override def desiredName = "register_constant"
  suggestName(f"register_$address%02x")
}
