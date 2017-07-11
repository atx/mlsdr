
`timescale 1 ps / 1 ps

module packeter (
		input wire nreset,
		input wire clk,

		input wire sample_valid,
		input wire[7:0] sample_data,

		input wire pps_valid,
		input wire[7:0] pps_data,

		input wire resp_valid,
		input wire[7:0] resp_data,

		output reg out_valid = 0,
		output reg[7:0] out_data = 0,

		output wire test
	);

	localparam PACKET_MAGIC = 8'hfe;

	localparam PACKET_ESCAPED = 8'hfe;
	localparam PACKET_SAMPLE = 8'h01;
	localparam PACKET_PPS = 8'h02;
	localparam PACKET_RESP = 8'h03;
	localparam PACKET_END = 8'h88;

	// Note that this needs to be divisible by 3, otherwise the unpacking is
	// going to get screwed up on the other side
	localparam SAMPLE_PACKET_SIZE = 513;
	localparam PPS_PACKET_SIZE = 4;

	localparam SRC_SAMPLE = 1, SRC_PPS = 2, SRC_RESP = 3;
	localparam S_IDLE = 0, S_START = 1, S_TYPE = 2, S_DATA = 3, S_ESCAPE = 4, S_END = 5;

	reg[15:0] sizecnt = 0;

	reg[2:0] source = 0;
	reg[3:0] state = S_IDLE;
	reg[3:0] retstate = S_IDLE;
	wire rdreq = state == S_DATA;

	wire fifo_sample_full;
	wire fifo_sample_empty;
	wire fifo_sample_rdreq = source == SRC_SAMPLE && rdreq;
	wire[7:0] fifo_sample_q;
	wire[9:0] fifo_sample_usedw;

	fifo8_short fifo_sample (
		.sclr(!nreset),
		.clock(clk),

		.full(fifo_sample_full),
		.empty(fifo_sample_empty),

		.wrreq(sample_valid),
		.data(sample_data),

		.rdreq(fifo_sample_rdreq),
		.q(fifo_sample_q),

		.usedw(fifo_sample_usedw)
	);

	wire fifo_pps_empty;
	wire fifo_pps_rdreq = source == SRC_PPS && rdreq;
	wire[7:0] fifo_pps_q;
	wire[9:0] fifo_pps_usedw;

	fifo8_short fifo_pps (
		.sclr(!nreset),
		.clock(clk),

		.empty(fifo_pps_empty),

		.wrreq(pps_valid),
		.data(pps_data),

		.rdreq(fifo_pps_rdreq),
		.q(fifo_pps_q),

		.usedw(fifo_pps_usedw)
	);

	wire fifo_resp_empty;
	wire fifo_resp_rdreq = source == SRC_RESP && rdreq;
	wire[7:0] fifo_resp_q;
	fifo8_short fifo_resp (
		.sclr(!nreset),
		.clock(clk),

		.empty(fifo_resp_empty),

		.wrreq(resp_valid),
		.data(resp_data),

		.rdreq(fifo_resp_rdreq),
		.q(fifo_resp_q)
	);

	wire[7:0] selected_q = source == SRC_SAMPLE ? fifo_sample_q :
							(source == SRC_PPS ? fifo_pps_q :
							(source == SRC_RESP ? fifo_resp_q :
							 8'h00));

	assign test = fifo_sample_full;

	always @(posedge clk) begin
		if (!nreset) begin
			sizecnt <= 0;
			source <= 0;
			state <= S_IDLE;
			out_valid <= 0;
			out_data <= 0;
		end else begin
			case (state)
				S_IDLE: begin
					// The FIFOs have some latency, usedw can be non-zero even
					// if empty is high
					//rdreq <= 0;
					out_valid <= 0;
					out_data <= 0;
					if (fifo_sample_usedw > SAMPLE_PACKET_SIZE && !fifo_sample_empty) begin
						state <= S_START;
						sizecnt <= SAMPLE_PACKET_SIZE;
						source <= SRC_SAMPLE;
					end else if (fifo_pps_usedw >= PPS_PACKET_SIZE) begin
						state <= S_START;
						sizecnt <= PPS_PACKET_SIZE;
						source <= SRC_PPS;
					end else if (!fifo_resp_empty) begin
						state <= S_START;
						sizecnt <= 1;
						source <= SRC_RESP;
					end
				end
				S_START: begin
					out_valid <= 1;
					out_data <= PACKET_MAGIC;
					state <= S_TYPE;
				end
				S_TYPE: begin
					out_valid <= 1;
					case (source)
						SRC_SAMPLE: begin
							out_data <= PACKET_SAMPLE;
						end
						SRC_PPS: begin
							out_data <= PACKET_PPS;
						end
						SRC_RESP: begin
							out_data <= PACKET_RESP;
						end
					endcase
					state <= S_DATA;
				end
				S_DATA: begin
					out_valid <= 1;
					//rdreq <= sizecnt > 0;
					out_data <= selected_q;
					sizecnt <= sizecnt - 1;
					if (selected_q == PACKET_MAGIC) begin
						state <= S_ESCAPE;
					end else if (sizecnt == 1) begin
						state <= S_IDLE;
					end
				end
				S_ESCAPE: begin
					out_valid <= 1;
					out_data <= PACKET_MAGIC;
					state <= sizecnt == 0 ? S_IDLE : S_DATA;
				end
			endcase
		end
	end
endmodule
