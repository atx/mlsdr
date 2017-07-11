
`timescale 1 ps / 1 ps

module register_tb();

	reg nreset = 1;
	reg clk = 0;

	initial begin
		$dumpfile("register_tb.vcd");
		$dumpvars(clk);
	end

	always begin
		#2 clk <= !clk;
	end

	reg[7:0] address = 0;
	reg[7:0] data_out = 0;
	reg data_oe = 0;
	wire[7:0] data = data_oe ? data_out : 8'hZZ;
	reg wr = 0;
	reg rd = 0;

	register #(.ADDRESS(8'hab), .INITVAL(8'h10), .MASK(8'h0f)) register (
		.nreset(nreset),
		.clk(clk),

		.address(address),
		.data(data),

		.rd(rd),
		.wr(wr),

		.maskvals(8'h04)
	);

	initial begin
		#20
		@(posedge clk);
		address = 8'hab;
		rd = 1;
		@(posedge clk);
		rd = 0;
		#40
		@(posedge clk);
		wr = 1;
		data_oe = 1;
		data_out = 8'h33;
		@(posedge clk);
		wr = 0;
	end

endmodule
