

`timescale 1 ps / 1 ps

module i2c_tb();

	reg nreset = 1;
	reg clk = 0;
	reg[7:0] address = 0;
	reg[7:0] data_out = 0;
	reg data_oe = 0;
	wire[7:0] data = data_oe ? data_out : 8'hZZ;
	reg rd = 0;
	reg wr = 0;

	initial begin
		$dumpfile("i2c_tb.vcd");
		$dumpvars(clk);
	end

	task write;
		input[7:0] to;
		input[7:0] val;

		begin
			@(posedge clk);
			wr = 1;
			address = to;
			data_oe = 1;
			data_out = val;
			@(posedge clk);
			wr = 0;
			address = 0;
			data_oe = 0;
			data_out = 0;
		end
	endtask

	task read;
		input[7:0] from;

		begin
			@(posedge clk);
			rd = 1;
			address = from;
			@(posedge clk);
			rd = 0;
			address = 0;
		end

	endtask

	i2c #(.BASEADDR(0), .CLKDIV(5)) i2c (
		.nreset(nreset),
		.clk(clk),
		.address(address),
		.data(data),
		.rd(rd),
		.wr(wr)
	);

	reg[7:0] test;

	initial begin
		@(posedge clk);
		nreset = 0;
		#10
		@(posedge clk);
		nreset = 1;
		#15
		write(1, 8'h80 | 2); // Write
		write(1, 8'b10101010); // Address
		write(1, 8'b00110010); // 
		write(1, 8'b00010001);
		write(1, 8'h80 | 2); // Write
		write(1, 8'b11011010); // Address
		write(1, 8'b10100110); // 
		write(1, 8'b01010001);
		#3000
		write(1, 3);
		write(1, 8'b11010011);
		//#100
		//write(0, 8'b10101011); // Read
		//write(3, 10);
		//#4000
		//read(4);

	end

	always begin
		#2 clk <= !clk;
	end

endmodule
