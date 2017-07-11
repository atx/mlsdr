
`include "common.v"

module adc #(
		parameter DIV = 4,
		parameter BITS = 12
	) (
		input wire nreset,
		input wire clk,

		input wire[1:0] mode,

		// ADC interface
		input wire[BITS - 1:0] adc_data,
		output wire adc_clk,
		// Output
		output reg[BITS - 1:0] out = 0,
		output reg valid = 0
	);

	reg[BITS - 1:0] patcnt = 0;
	reg[$clog2(DIV):0] cnt = 0;

	assign adc_clk = cnt < DIV / 2;

	reg[BITS - 1:0] patout;

	always @(patcnt) begin
		case (mode)
		1: patout = patcnt & 1'b1 ? 12'hfe3 : 12'hc12;
		2: patout = patcnt;
		3: patout = patcnt & 1'b1 ? 12'h555 : 12'haaa;
		endcase
	end

	always @(posedge clk) begin
		if (!nreset) begin
			out <= 0;
			valid <= 0;
			cnt <= 0;
			patcnt <= 0;
		end else begin
			if (cnt == DIV - 1) begin
				out <= mode > 0 ? patout : adc_data;
				patcnt <= patcnt + 1;
				valid <= 1;
				cnt <= 0;
			end
			if (cnt != DIV - 1) begin
				cnt <= cnt + 1;
			end
			if (valid) begin
				valid <= 0;
			end
		end
	end

endmodule

