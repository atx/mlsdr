
module ppsin (
		input wire nreset,
		input wire clk,

		input wire pps,

		output wire[7:0] pps_data,
		output wire pps_valid
	);

	reg[32:0] counter = 0;
	reg prevpps = 0;

	reg[32:0] data = 0;
	reg[3:0] outcnt = 0;

	assign pps_data = outcnt == 0 ? 8'h00 : (data >> ((outcnt - 1) * 8));
	assign pps_valid = outcnt > 0;

	always @(posedge clk) begin
		if (!nreset) begin
			counter <= 0;
			prevpps <= 0;
			data <= 0;
			outcnt <= 0;
		end else begin
			prevpps <= pps;
			// outcnt should be 0 during normal operation, but just to be
			// sure...
			if (!prevpps && pps && outcnt == 0) begin
				data <= counter;
				outcnt <= 4;
				counter <= 0;
			end else begin
				counter <= counter + 1;
			end
		end
		if (outcnt > 0) begin
			outcnt <= outcnt - 1;
		end
	end

endmodule
