

package mlsdr

import chisel3._
import chisel3.util._

class Packer12to8 extends Module {
  val io = IO(new Bundle {
    val in = Input(Valid(UInt(12.W)))
    val out = Output(Valid(UInt(8.W)))
  })

  val inBuffer = Reg(UInt(12.W))
  val outBuffer = Reg(UInt(24.W))
  val inState = RegInit(false.B)
  val outCnt = RegInit(0.U(3.U.getWidth.W))

  when (io.in.valid) {
    inState := !inState
    when (inState) {
      outBuffer := (io.in.bits << 12.U) | inBuffer
      outCnt := 3.U
    } .otherwise {
      inBuffer := io.in.bits
    }
  }

  io.out.bits := outBuffer & 0xff.U
  when (!(io.in.valid && inState) && outCnt > 0.U) {
    io.out.valid := outCnt > 0.U
    outCnt := outCnt - 1.U
    outBuffer := outBuffer >> 8.U
  }
}
