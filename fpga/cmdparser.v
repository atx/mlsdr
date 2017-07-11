

module cmdparser (
		input wire nreset,
		input wire clk,

		// Register bus
		output reg[7:0] address = 0,
		inout wire[7:0] data,
		output reg rd = 0,
		output reg wr = 0,

		// FTDI
		input wire[7:0] in_data,
		input wire in_valid,
		output wire in_ack,

		output reg[7:0] out_data = 0,
		output reg out_valid = 0
	);

	localparam S_IDLE = 0, S_READ_A = 1, S_READ_D = 2, S_WRITE_A = 3, S_WRITE_D = 4,
				S_END = 9;

	localparam CMD_WRITE = 8'haa, CMD_READ = 8'h55;

	reg[7:0] state = S_IDLE;

	// Output enable of the _other_ side
	reg[7:0] data_out = 0;
	assign data = !rd ? data_out : 8'hZZ;

	assign in_ack = state == S_IDLE || state == S_READ_A || state == S_WRITE_A || state == S_WRITE_D;

	always @(posedge clk) begin
		if (!nreset) begin
			state <= S_IDLE;
			address = 0;
			data_out = 0;
			rd = 0;
			wr = 0;
		end else begin
			case (state)
				S_IDLE: begin
					rd <= 0;
					wr <= 0;
					if (in_valid) begin
						case (in_data)
							CMD_READ: begin
								state <= S_READ_A;
							end
							CMD_WRITE: begin
								state <= S_WRITE_A;
							end
						endcase
					end
				end
				S_READ_A: begin
					if (in_valid) begin
						address <= in_data;
						rd <= 1;
						state <= S_READ_D;
					end
				end
				S_READ_D: begin
					rd <= 0;
					out_data <= data;
					out_valid <= 1;
					state <= S_END;
				end
				S_WRITE_A: begin
					if (in_valid) begin
						address <= in_data;
						state <= S_WRITE_D;
					end
				end
				S_WRITE_D: begin
					if (in_valid) begin
						data_out <= in_data;
						wr <= 1;
						state <= S_END;
					end
				end
				S_END: begin
					rd <= 0;
					wr <= 0;
					out_valid <= 0;
					state <= S_IDLE;
				end
			endcase
		end
	end
endmodule
