
module register #(
		parameter ADDRESS = 8'h00,
		parameter INITVAL = 8'h00,
		parameter MASK = 8'h00
	) (
		input wire nreset,
		input wire clk,

		input wire[7:0] address,
		inout wire[7:0] data,
		input wire rd,
		input wire wr,

		input wire set_force,
		input wire[7:0] set_data,

		input wire[7:0] maskvals,
		output wire[7:0] out
	);

	reg[7:0] value = INITVAL;

	assign out = (value & ~MASK) | (maskvals & MASK);

	wire selected = address == ADDRESS;

	assign data = (rd && selected) ? out : 8'hZZ;

	always @(posedge clk) begin
		if (!nreset) begin
			value <= INITVAL;
		end else begin
			if (set_force) begin
				value <= set_data;
			end else if (wr && selected) begin
				value <= data;
			end
		end
	end


endmodule

module register_constant #(
		parameter ADDRESS = 8'h00,
		parameter VALUE = 8'h00
	) (
		input wire nreset,
		input wire clk,

		input wire[7:0] address,
		inout wire[7:0] data,
		input wire rd
	);

	assign data = (address == ADDRESS && rd) ? VALUE : 8'hZZ;

endmodule

module register_fifo_rd #(
		parameter ADDRESS = 8'h00
	) (
		input wire nreset,
		input wire clk,

		input wire[7:0] address,
		inout wire[7:0] data,
		input wire rd,
		input wire wr,

		input wire [7:0] wrdata,
		input wire wrreq,

		output wire empty,
		output wire full
	);

	wire read = address == ADDRESS && rd ;

	wire[7:0] fifo_out;
	assign data = read ? fifo_out : 8'hZZ;

	fifo8_short fifo (
		.sclr(!nreset),
		.clock(clk),

		.rdreq(read),
		.q(fifo_out),

		.wrreq(wrreq),
		.data(wrdata),

		.empty(empty),
		.full(full)
	);

endmodule

module register_fifo_wr #(
		parameter ADDRESS = 8'h00
	) (
		input wire nreset,
		input wire clk,

		input wire[7:0] address,
		input wire[7:0] data,
		input wire rd,
		input wire wr,

		output wire[7:0] rddata,
		input wire rdreq,
		output wire[9:0] usedw
	);

	fifo8_short fifo (
		.sclr(!nreset),
		.clock(clk),

		.rdreq(rdreq),
		.q(rddata),
		.usedw(usedw),

		.wrreq(address == ADDRESS && wr),
		.data(data)
	);

endmodule
