
module pwm #(
		parameter BITS = 24,
		parameter BITS_WIDTH = 24
	) (
		input wire nreset,
		input wire clk,

		input wire[BITS + 1:0] period,
		input wire[BITS + 1:0] width,

		output wire out
	);

	reg[BITS + 1:0] cnt = 0;
	wire[BITS + 1:0] border = width << (BITS- BITS_WIDTH);
	//wire[BITS + 1:0] border = width;

	assign out = cnt < border;

	always @(posedge clk) begin
		if (!nreset) begin
			cnt <= 0;
		end else begin
			if (cnt >= period) begin
				cnt <= 0;
			end else begin
				cnt <= cnt + 1;
			end
		end
	end

endmodule
