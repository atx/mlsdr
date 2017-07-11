

`timescale 1 ps / 1 ps

module packeter_tb ();
	reg nreset = 1;
	reg clk = 0;

	initial begin
		$dumpfile("packeter_tb.vcd");
		$dumpvars(clk);
	end

    always begin
		#2 clk <= !clk;
    end

	wire adc_packer_valid;
	wire[11:0] adc_packer_data;

	adc #(.DIV(40), .BITS(12)) adc (
	        .nreset(1'b1),
	        .clk(clk),
	        .pattern(1'b1),

	        .out(adc_packer_data),
	        .valid(adc_packer_valid)
	);

	wire packer_packeter_valid;
	wire[7:0] packer_packeter_data;

	packer_12to8 packer (
	        .nreset(1'b1),
	        .clk(clk),

	        .in_valid(adc_packer_valid),
	        .in_data(adc_packer_data),

	        .out_valid(packer_packeter_valid),
	        .out_data(packer_packeter_data)
	);

	wire packeter_ftdi_valid;
	wire[7:0] packeter_ftdi_data;

	packeter packeter (
	        .nreset(1'b1),
	        .clk(clk),

	        .sample_valid(packer_packeter_valid),
	        .sample_data(packer_packeter_data),
	        //.sample_valid(adc_packer_valid),
	        //.sample_data(adc_packer_data),

	        .event_valid(1'b0),
	        .event_data(8'h00),

	        .out_valid(packeter_ftdi_valid),
	        .out_data(packeter_ftdi_data)
	);

endmodule
