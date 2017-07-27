

package mlsdr

import chisel3._
import chisel3.util._


class PatternSawtooth(val bits: Int, val divideBy: Int) extends Module {
  val io = IO(new Bundle {
    val enable = Input(Bool())
    val out = Valid(UInt(bits.W))
  })

  require(divideBy % 2 == 0)

  val divCnt = Counter(divideBy)
  val valueCnt = Counter(scala.math.pow(2, bits).toInt)

  when (divCnt.inc() && io.enable) {
    io.out.bits := valueCnt.value
    valueCnt.inc()
    io.out.valid := true.B
  } .otherwise {
    io.out.bits := 0.U
    io.out.valid := false.B
  }
}
