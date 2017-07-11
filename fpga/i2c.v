
// A rather ghetto I2C master implementation, don't expect anything and you
// won't be disappointed

module i2c #(
		parameter BASEADDR = 0,
		parameter CLKDIV = 1000
	) (
		input wire nreset,
		input wire clk,
		input wire[7:0] address,
		inout wire[7:0] data,
		input wire rd,
		input wire wr,

		// I2C interface
		inout wire scl,
		inout wire sda
	);

	localparam S_IDLE = 0, S_READ = 1, S_RACK = 2, S_WRITE = 3, S_WACK = 4, S_END1 = 9, S_END2 = 10;

	reg[7:0] state = S_IDLE;

	wire[9:0] cmd_avail;
	register #(.ADDRESS(BASEADDR + 0), .MASK(8'b11111111)) reg_i2cs (
		.nreset(nreset),
		.clk(clk),

		.address(address),
		.data(data),
		.rd(rd),
		.wr(wr),

		.maskvals({1'b0, 1'b0, 1'b0, 1'b0,
				   1'b0, 1'b0, 1'b0, cmd_avail != 0 || state != S_IDLE})
	);

	wire[7:0] cmd_top_raw;
	wire cmd_pop;
	register_fifo_wr #(.ADDRESS(BASEADDR + 1)) reg_i2ccmd (
		.nreset(nreset),
		.clk(clk),

		.address(address),
		.data(data),
		.rd(rd),
		.wr(wr),

		.rddata(cmd_top_raw),
		.rdreq(cmd_pop),
		.usedw(cmd_avail)
	);
	wire[7:0] cmd_top = cmd_avail > 0 ? cmd_top_raw : 0;
	wire[7:0] bytes_todo = cmd_top & ~(1 << 7);
	wire can_start_write = cmd_top & (1 << 7) && cmd_avail >= (bytes_todo + 2);
	wire can_start_read = !(cmd_top & (1 << 7)) && bytes_todo > 0 && cmd_avail >= 2;

	reg[7:0] rbyte = 0;
	reg cmd_pop_pre = 0;
	reg reg_i2cr_wrreq_pre = 0;
	wire reg_i2cr_wrreq;

	edgetrigger trig_i2cr_wrreq (
		.clk(clk),
		.in(reg_i2cr_wrreq_pre),
		.out(reg_i2cr_wrreq)
	);

	edgetrigger trig_cmd_pop (
		.clk(clk),
		.in(cmd_pop_pre),
		.out(cmd_pop)
	);

	register_fifo_rd #(.ADDRESS(BASEADDR + 2)) reg_i2cr (
		.nreset(nreset),
		.clk(clk),

		.address(address),
		.data(data),
		.rd(rd),
		.wr(wr),

		.wrreq(reg_i2cr_wrreq),
		.wrdata(rbyte)
	);

	wire i2c_preclk;
	clkdiv #(.DIV(CLKDIV / 2)) prediv (
		.nreset(nreset),
		.clk(clk),
		.out(i2c_preclk)
	);
	wire i2c_clk;
	clkdiv #(.DIV(1)) div (
		.nreset(nreset),
		.clk(i2c_preclk),
		.out(i2c_clk)
	);

	reg sda_out = 1;
	reg sda_out_ = 1;
	assign sda = !sda_out_ ? 1'b0 : 1'bZ;
	reg[15:0] bcnt = 0;

	reg scl_oe = 0;
	reg scl_oe_ = 0;
	assign scl = (scl_oe_ && !i2c_clk) ? 0 : 1'bz;

	reg[15:0] bitcount = 0;
	reg[7:0] byte = 0;
	wire[2:0] bitidx = (bitcount - 1) % 8;
	wire selected_bit = cmd_top[bitidx];

	reg writing = 0;

	// Note: This definitely does not support clock stretching,
	// reading unlimited amount of bytes nor probably anything any sort of
	// reasonable I2C implementation would

	always @(negedge i2c_clk) begin
		if (!nreset) begin
			state <= S_IDLE;
			sda_out <= 1;
			scl_oe <= 1;
			bitcount <= 0;
			writing <= 0;
			rbyte <= 0;
			cmd_pop_pre <= 0;
		end else begin
			case (state)
				S_IDLE: begin
					if (can_start_write || can_start_read) begin
						// For reading, we first go to S_WRITE and after
						// writing the address to S_READ
						state <= S_WRITE;
						writing <= can_start_write;
						cmd_pop_pre <= 1;
						// Start!
						sda_out <= 0;
						scl_oe <= 1;
						byte <= 0;
						bitcount <= (bytes_todo + 1) * 8; // Device address, memory address, value
					end
				end
				S_WRITE: begin
					scl_oe <= 1;
					cmd_pop_pre <= 0;
					sda_out <= selected_bit;
					bitcount <= bitcount - 1;
					if (bitcount % 8 == 1) begin
						state <= S_WACK;
					end
				end
				S_WACK: begin
					cmd_pop_pre <= 1;
					byte <= byte + 1;
					sda_out <= 1;
					if (bitcount > 0) begin
						state <= writing ? S_WRITE : S_READ;
					end else begin
						state <= S_END1;
					end
				end
				S_READ: begin
					cmd_pop_pre <= 0;
					bitcount <= bitcount - 1;
					sda_out <= 1;
					if (bitcount % 8 != 0) begin
						rbyte <= (rbyte << 1) | sda;
					end
					if (bitcount % 8 == 1) begin
						state <= S_RACK;
					end
					reg_i2cr_wrreq_pre <= 0;
				end
				S_RACK: begin
					rbyte <= (rbyte << 1) | sda;
					reg_i2cr_wrreq_pre <= 1;
					if (bitcount > 0) begin
						sda_out <= 0;
						state <= S_READ;
					end else begin
						// We don't ack the last byte
						state <= S_END1;
					end
				end
				S_END1: begin
					state <= S_END2;
					scl_oe <= 0;
					sda_out <= 0;
					cmd_pop_pre <= 0;
					reg_i2cr_wrreq_pre <= 0;
				end
				S_END2: begin
					sda_out <= 1;
					scl_oe <= 0;
					state <= S_IDLE;
				end
			endcase
		end
	end

	always @(posedge i2c_clk) begin
		scl_oe_ <= scl_oe;
	end

	always @(negedge i2c_preclk) begin
		sda_out_ <= sda_out;
	end

endmodule
