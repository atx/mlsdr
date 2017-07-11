

module ft232 (
		input wire nreset,
		input wire clk,

		// FTDI interface
		input wire ft_clk,
		input wire ft_nrxf,
		inout wire[7:0] ft_data,
		output wire ft_noe,
		output wire ft_nrd,
		output wire ft_nwr,
		input wire ft_ntxe,

		// Device->PC FIFO
		input wire[7:0] wrdata,
		input wire wrreq,
		// PC->Device FIFO
		output wire[7:0] rddata,
		input wire rdreq,
		output wire rdempty,

		output wire test
	);

	localparam S_IDLE = 0, S_WRITE = 1, S_TURN = 2, S_READ = 3, S_END = 4;

	reg[7:0] state = S_IDLE;

	wire wrfifo_rdreq;
	wire wrfifo_rdempty;
	wire[7:0] wrfifo_q;
	wire[15:0] wrfifo_rdusedw;

	ft232_fifo wrfifo (
		.aclr(!nreset),
		//.aclr(1'b0),
		.wrclk(clk),
		.data(wrdata),
		.wrreq(wrreq),

		.rdclk(ft_clk),
		.rdreq(wrfifo_rdreq),
		.rdempty(wrfifo_rdempty),
		.rdusedw(wrfifo_rdusedw),
		.q(wrfifo_q)
	);

	wire[7:0] rdfifo_data;
	wire rdfifo_wrreq;
	wire rdfifo_wrfull;

	ft232_fifo_short rdfifo (
		//.aclr(!nreset),
		//.aclr(1'b0),
		.rdclk(clk),
		.q(rddata),
		.rdreq(rdreq),
		.rdempty(rdempty),

		.wrclk(ft_clk),
		.data(ft_data),
		.wrreq(rdfifo_wrreq),
		.wrfull(rdfifo_wrfull)
	);

	assign ft_data = ft_noe ? wrfifo_q : 8'hZZ;
	assign ft_nwr = !(!wrfifo_rdempty && state == S_WRITE && !ft_ntxe);
	assign wrfifo_rdreq = !ft_nwr;

	assign ft_noe = !(state == S_TURN || state == S_READ);
	assign ft_nrd = !(state == S_READ);
	assign rdfifo_wrreq = !ft_nrd && !ft_nrxf;

	reg[15:0] timeout = 0;

	assign test = rdempty;

	always @(posedge ft_clk) begin
		timeout <= timeout + 1;
		case (state)
			S_IDLE: begin
				if (!ft_nrxf && !rdfifo_wrfull) begin
					state <= S_TURN;
				end else if (!ft_ntxe &&
					(wrfifo_rdusedw > 512 || (!wrfifo_rdempty && timeout == 0))) begin
					// We have some data ready and FTDI has some space, begin!
					state <= S_WRITE;
				end
			end
			S_WRITE: begin
				if (wrfifo_rdempty || ft_ntxe) begin
					state <= S_END;
				end
			end
			S_TURN: begin
				state <= S_READ;
			end
			S_READ: begin
				if (rdfifo_wrfull || ft_nrxf) begin
					state <= S_END;
				end
			end
			S_END: begin
				state <= S_IDLE;
			end
		endcase
	end
endmodule
