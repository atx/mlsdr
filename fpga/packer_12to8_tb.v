
`timescale 1 ps / 1 ps

module packer_12to8_tb();

	reg nreset = 1;
	reg clk = 0;

	initial begin
		$dumpfile("packer_12to8_tb.vcd");
		$dumpvars(clk);
		#1000 $finish;
	end

	always begin
		#2 clk <= !clk;
	end

	adc #(.DIV(8)) adc (
		.nreset(nreset),
		.pattern(1'b1),
		.clk(clk),

		.adc_data(12'ha5a)
	);

	packer_12to8 pack (
		.nreset(nreset),
		.clk(clk),

		.in_data(adc.out),
		.in_valid(adc.valid)
	);

endmodule
