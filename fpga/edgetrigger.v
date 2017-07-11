
module edgetrigger (
		input wire clk,
		input wire in,

		output wire out
	);

	reg triggered = 0;

	assign out = in && !triggered;

	always @(posedge clk) begin
		if (in) begin
			triggered <= 1;
		end else begin
			triggered <= 0;
		end
	end

endmodule
