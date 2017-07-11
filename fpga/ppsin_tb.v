

`timescale 1 ps / 1 ps

module ppsin_tb();

	reg nreset = 1;
	reg clk = 0;
	reg pps = 0;

	always begin
		#2 clk <= !clk;
	end

	always begin
		#100
		pps = 0;
		#900
		pps = 1;
	end

	ppsin ppsin (
		.nreset(nreset),
		.clk(clk),

		.pps(pps)
	);


endmodule
