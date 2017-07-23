

package mlsdr

import chisel3._
import chisel3.util._
import chisel3.iotesters.{ChiselFlatSpec, PeekPokeTester, Driver}


class ADCReaderTesterSimpleRead(c: ADCReader) extends PeekPokeTester(c) {
  poke(c.io.adc.data, 0x32)
  poke(c.io.enable, true)
  for (x <- 0 to 100) {
    poke(c.io.adc.data, x)
    if (peek(c.io.out.valid) == 1) {
      expect(c.io.out.bits, x)
      expect(c.io.adc.clock, false)
    }
  }
}

class ADCReaderTesterEnableTest(c: ADCReader) extends PeekPokeTester(c) {
  poke(c.io.enable, false)
  for (_ <- 0 to 100) {
    step(1)
    expect(c.io.adc.clock, false)
    expect(c.io.out.valid, false)
  }
}

class ADCReaderTests extends ChiselFlatSpec {
  private val backendNames = Array[String]("verilator", "firrtl")
  for (backendName <- backendNames) {
    "ADCReader" should s"properly read an ADC (with $backendName)" in {
      Driver(() => new ADCReader(8, 8), backendName) {
        c => new ADCReaderTesterSimpleRead(c)
      } should be (true)
    }

    "ADCReader" should s"not read the ADC or output data when disabled (with $backendName)" in {
      Driver(() => new ADCReader(8, 8), backendName) {
        c => new ADCReaderTesterEnableTest(c)
      } should be (true)
    }
  }
}
