

module clkdiv #(
		parameter DIV = 1
	) (
		input wire nreset,
		input wire clk,
		output reg out = 0
	);

	reg[$clog2(DIV):0] cnt = 0;

	always @(posedge clk) begin
		if (!nreset) begin
			cnt <= 0;
		end else begin
			if (cnt == (DIV - 1)) begin
				cnt <= 0;
				out = !out;
			end else begin
				cnt <= cnt + 1;
			end
		end
	end

endmodule
