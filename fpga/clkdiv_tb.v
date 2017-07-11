

module clkdiv_tb();

	reg nreset = 1;
	reg clk = 0;

	clkdiv #(.DIV(1)) div1 (
		.nreset(nreset),
		.clk(clk)
	);

	clkdiv #(.DIV(5)) div2 (
		.nreset(nreset),
		.clk(clk)
	);

	initial begin
		$dumpfile("clkdiv_tb.vcd");
		$dumpvars(clk);
		#1000 $finish;
	end

	always begin
		#2 clk <= !clk;
	end

endmodule
