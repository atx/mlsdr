
`timescale 1 ps / 1 ps

module ft232_tb();

	reg nreset = 1;
	reg clk = 0;

	reg ft_clk = 0;
	reg ft_nrxf = 1;
	reg ft_ntxe = 1;

	reg ft_data_out = 8'h5a;
	reg ft_data = 8'hZZ;

	reg rdreq = 0;

	initial begin
		$dumpfile("ft232_tb.vcd");
		$dumpvars(clk);
	end

	wire adc_ftdi_valid;
	wire[7:0] adc_ftdi_data;

	adc #(.DIV(8), .BITS(8)) adc (
		.nreset(1'b1),
		.clk(clk),
		.pattern(1'b1),

		.out(adc_ftdi_data),
		.valid(adc_ftdi_valid)
	);

	ft232 ftdi_write (
		.nreset(nreset),
		.clk(clk),

		.ft_clk(ft_clk),
		.ft_ntxe(ft_ntxe),

		.wrdata(adc_ftdi_data),
		.wrreq(adc_ftdi_valid),

		.rdreq(rdreq)
	);

	wire[7:0] ftdi_read_data;

	ft232 ftdi_read (
		.nreset(nreset),
		.clk(clk),

		.ft_clk(ft_clk),
		.ft_nrxf(ft_nrxf),

		.ft_data(ftdi_read_data)
	);

	assign ftdi_read_data = ftdi_read.ft_noe ? 8'hZZ : 8'h45;


	initial begin
		#20
		ft_ntxe <= 0;
		ft_nrxf <= 0;
		#100
		ft_nrxf <= 1;
	end

	always
		#2 ft_clk <= !ft_clk;
	always
		#10 clk <= !clk;

endmodule
