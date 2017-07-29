
package mlsdr

import chisel3._
import firrtl._
import java.io._

object Main extends App {
  val defaultArgs = Array("--target-dir", "build/")
  chisel3.Driver.execute(defaultArgs ++ args, () => new Registers)
  chisel3.Driver.execute(defaultArgs ++ args, () => new ADCReader(12, 6))
  chisel3.Driver.execute(defaultArgs ++ args, () => new PatternSawtooth(12, 6))
}
