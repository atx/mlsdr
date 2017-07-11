

module sync (
		input wire in,
		input wire outclk,
		output reg out = 0
	);

	reg int = 0;

	always @(posedge outclk) begin
		int <= in;
		out <= int;
	end

endmodule
