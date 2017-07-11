
`timescale 1 ps / 1 ps

module cmdparser_tb();

	reg nreset = 1;
	reg clk = 0;

	initial begin
		$dumpfile("cmdparser_tb.vcd");
		$dumpvars(clk);
	end

	always begin
		#2 clk <= !clk;
	end

	register #(.ADDRESS(8'hab), .INITVAL(8'h10), .MASK(8'h0f)) register (
		.nreset(nreset),
		.clk(clk),

		.maskvals(8'h04)
	);

	reg[7:0] fifo_in = 0;
	reg fifo_wr = 0;

	fifo8_short fifo (
		.clock(clk),
		.data(fifo_in),
		.wrreq(fifo_wr)

	);

	cmdparser cmdparser (
		.nreset(nreset),
		.clk(clk),

		.address(register.address),
		.data(register.data),
		.rd(register.rd),
		.wr(register.wr),

		.in_data(fifo.q),
		.in_valid(!fifo.empty),
		.in_ack(fifo.rdreq)
	);

	initial begin
		#20
		@(posedge clk);
		fifo_wr = 1;
		fifo_in = 8'h55;
		@(posedge clk);
		fifo_in = 8'hab;
		@(posedge clk);
		fifo_wr = 0;
		#100
		@(posedge clk);
		fifo_wr = 1;
		fifo_in = 8'haa;
		@(posedge clk);
		fifo_in = 8'hab;
		@(posedge clk);
		fifo_in = 8'h50;
		@(posedge clk);
		fifo_wr = 0;
	end

endmodule
