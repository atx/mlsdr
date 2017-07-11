

module adc_tb();

	reg nreset = 1;
	reg clk = 0;
	reg[11:0] adc_data = 10;
	wire adc_clk;

	initial begin
		$dumpfile("adc_tb.vcd");
		$dumpvars(clk);
		#1000 $finish;
	end

	adc #(.DIV(4)) adc (
		.nreset(nreset),
		.clk(clk),
		.adc_data(adc_data),
		.adc_clk(adc_clk)
	);

	always begin
		#2 clk <= !clk;
	end

	always @(posedge adc_clk) begin
		adc_data <= adc_data + 1;
	end

endmodule
