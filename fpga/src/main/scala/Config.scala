

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.experimental._

object Config {
  val clockFrequency = 90000000
  val hasTuner = false
  val adcDivisor = clockFrequency / 15000000
  // TODO: i2cDiv
}
