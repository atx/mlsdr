
module packer_12to8 (
		input wire nreset,
		input wire clk,

		input wire[11:0] in_data,
		input wire in_valid,

		output reg[7:0] out_data = 0,
		output reg out_valid = 0
	);

	reg[11:0] buffer = 0;
	reg state = 0;

	reg[23:0] buffer_o = 0;
	reg[2:0] state_o = 0;

	always @(posedge clk) begin
		if (!nreset) begin
			buffer <= 0;
			state <= 0;
			buffer_o <= 0;
			state_o <= 0;
			out_data <= 0;
			out_valid <= 0;
		end else begin
			if (in_valid) begin
				if (state) begin
					buffer_o <= (in_data << 12) | buffer;
					state_o <= 3;
					state <= 0;
				end else begin
					buffer <= in_data;
					state <= 1;
				end
			end
			// Here we just assume that this never overflows, hooray
			if (state_o > 0) begin
				out_valid <= 1;
				out_data <= buffer_o & 8'hff;
				buffer_o <= buffer_o >> 8;
				state_o <= state_o - 1;
			end else begin
				out_valid <= 0;
				out_data <= 0;
			end
		end
	end

endmodule
