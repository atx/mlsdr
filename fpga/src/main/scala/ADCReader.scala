

package mlsdr

import chisel3._
import chisel3.util._


class ADCReader(val bits: Int, val divideBy: Int) extends Module {
  val io = IO(new Bundle {
    val enable = Input(Bool())
    val adc = new Bundle {
      val clock = Output(Bool())
      val data = Input(UInt(bits.W))
    }
    val out = Valid(UInt(bits.W))
  })

  require(divideBy % 2 == 0)

  val (cnt, wrap) = Counter(true.B, divideBy)
  io.adc.clock := Mux(io.enable, cnt < (divideBy / 2).U, false.B)

  when (wrap && io.enable) {
    io.out.bits := io.adc.data
    io.out.valid := true.B
  } .otherwise {
    io.out.bits := 0.U
    io.out.valid := false.B
  }
}
