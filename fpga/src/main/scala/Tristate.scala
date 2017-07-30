
package mlsdr

import chisel3._
import chisel3.util._
import chisel3.experimental._

class Tristate[+T <: Bits](gen: T) extends BlackBox(Map("WIDTH" -> gen.getWidth)) {
  val io = IO(new Bundle {
    val tristate = Analog(gen.getWidth.W)

    val out = Output(gen.chiselCloneType)
    val value = Input(gen.chiselCloneType)
    val driven = Input(Bool())
  })

  override def desiredName = "tristate"
}
