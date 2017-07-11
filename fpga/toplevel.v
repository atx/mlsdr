
module toplevel (
		// ADC Pins
		input wire ADC_D13,
		input wire ADC_D12,
		input wire ADC_D11,
		input wire ADC_D10,
		input wire ADC_D9,
		input wire ADC_D8,
		input wire ADC_D7,
		input wire ADC_D6,
		input wire ADC_D5,
		input wire ADC_D4,
		input wire ADC_D3,
		input wire ADC_D2,
		input wire ADC_D1,
		input wire ADC_D0,
		output wire ADC_CLK,

		// FTDI Pins
		inout wire FT_D7,
		inout wire FT_D6,
		inout wire FT_D5,
		inout wire FT_D4,
		inout wire FT_D3,
		inout wire FT_D2,
		inout wire FT_D1,
		inout wire FT_D0,
		input wire FT_nRXF,
		input wire FT_nTXE,
		output wire FT_nRD,
		output wire FT_nWR,
		output wire FT_nOE,
		input wire FT_CLK,

		input wire FT_RST,

		// Other pins
		input wire PPS,
		input wire CLK_EXT,
		input wire CLK_INT,
		inout wire SCL,
		inout wire SDA,
		output wire LED1,
		output wire LED2,
		output wire TUNER_CLK
	);

	wire clk;

	pll pll (
		.inclk0(CLK_INT),
		.c0(clk)
	);

	wire ft_rst_sync;
	sync rstsync (
		.in(FT_RST),
		.out(ft_rst_sync),
		.outclk(clk)
	);
	wire nreset = !ft_rst_sync;
	//wire nreset = 1;

	// This is shitty and will be replaced with a proper 30MHz oscillator in
	// the next board revision
	//clkdiv #(.DIV(2)) tunerdiv (
	//	.nreset(1'b1),
	//	.clk(clk),
	//	.out(TUNER_CLK)
	//);

	//assign LED2 = CLK_EXT;
	assign TUNER_CLK = CLK_EXT;

	wire[7:0] bus_address;
	wire[7:0] bus_data;
	wire bus_rd;
	wire bus_wr;

	// First bit = ADC enabled
	wire[7:0] reg_adctl_val;
	register #(.ADDRESS(8'h01)) reg_adctl (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),
		.out(reg_adctl_val)
	);
	register #(.ADDRESS(8'hf0), .MASK(8'hff)) regid1 (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.maskvals(8'h61)
	);
	register #(.ADDRESS(8'hf1), .MASK(8'hff)) regid2 (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.maskvals(8'h74)
	);
	register #(.ADDRESS(8'hf2), .MASK(8'hff)) regid3 (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.maskvals(8'h78)
	);
	register #(.ADDRESS(8'hf3), .INITVAL(8'haa)) regscratch (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr)
	);

	// TODO: Add config.h with defines for this
	register #(.ADDRESS(8'hf5), .MASK(8'hff)) regfeatures (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.maskvals(8'b00000001)
	);

	i2c #(.BASEADDR(8'h10), .CLKDIV(400)) i2c (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.scl(SCL),
		.sda(SDA)
	);

	wire adc_packer_valid;
	wire[11:0] adc_packer_data;

	wire adc_nreset = nreset && reg_adctl_val[0];
	adc #(.DIV(8), .BITS(12)) adc (
		.nreset(adc_nreset),
		.clk(clk),

		.mode(reg_adctl_val[2:1]),

		.adc_data({ADC_D11, ADC_D10, ADC_D9, ADC_D8, ADC_D7, ADC_D6, ADC_D5,
				   ADC_D4, ADC_D3, ADC_D2, ADC_D1, ADC_D0}),
		.adc_clk(ADC_CLK),

		.out(adc_packer_data),
		.valid(adc_packer_valid)
	);

	pwm #(.BITS(32), .BITS_WIDTH(32)) led_status_pwm (
		.nreset(1'b1), // The reset may be asserted on powerup, yet we want to have a status LED
		.clk(clk),

		.period(!nreset ?      500000000 :  // We are in reset
			    (!adc_nreset ? 500000000 :  // ADC is not enabled
				                40000000)), // ADC running

		.width(!nreset ?        10000000 :
		       (!adc_nreset ?   40000000 :
				                20000000)),

		.out(LED1)
	);

	wire packer_packeter_valid;
	wire[7:0] packer_packeter_data;

	packer_12to8 packer (
		.nreset(nreset),
		.clk(clk),

		.in_valid(adc_packer_valid),
		.in_data(adc_packer_data),

		.out_valid(packer_packeter_valid),
		.out_data(packer_packeter_data)
	);

	wire packeter_ftdi_valid;
	wire[7:0] packeter_ftdi_data;
	wire cmdparser_packeter_valid;
	wire[7:0] cmdparser_packeter_data;

	wire pps_synced;
	sync pssync (
		.outclk(clk),
		.in(PPS),
		.out(pps_synced)
	);

	wire ppsin_packeter_valid;
	wire[7:0] ppsin_packeter_data;

	ppsin ppsin (
		.nreset(nreset),
		.clk(clk),
		.pps(pps_synced),

		.pps_valid(ppsin_packeter_valid),
		.pps_data(ppsin_packeter_data)
	);

	assign LED2 = pps_synced;

	packeter packeter (
		.nreset(nreset),
		.clk(clk),

		.sample_valid(packer_packeter_valid),
		.sample_data(packer_packeter_data),

		.pps_valid(ppsin_packeter_valid),
		.pps_data(ppsin_packeter_data),

		.out_valid(packeter_ftdi_valid),
		.out_data(packeter_ftdi_data),

		.resp_valid(cmdparser_packeter_valid),
		.resp_data(cmdparser_packeter_data)
	);

	wire[7:0] ftdi_rddata;
	wire ftdi_rdreq;
	wire ftdi_rdempty;

	ft232 ftdi (
		.nreset(nreset),
		.clk(clk),

		.ft_clk(FT_CLK),
		.ft_nrxf(FT_nRXF),
		.ft_data({FT_D7, FT_D6, FT_D5, FT_D4, FT_D3, FT_D2, FT_D1, FT_D0}),
		.ft_noe(FT_nOE),
		.ft_nrd(FT_nRD),
		.ft_nwr(FT_nWR),
		.ft_ntxe(FT_nTXE),

		.wrreq(packeter_ftdi_valid),
		.wrdata(packeter_ftdi_data),

		.rddata(ftdi_rddata),
		.rdreq(ftdi_rdreq),
		.rdempty(ftdi_rdempty),

		//.test(LED2)
	);

	cmdparser cmdparser (
		.nreset(nreset),
		.clk(clk),

		.address(bus_address),
		.data(bus_data),
		.rd(bus_rd),
		.wr(bus_wr),

		.in_data(ftdi_rddata),
		.in_valid(!ftdi_rdempty),
		.in_ack(ftdi_rdreq),

		.out_valid(cmdparser_packeter_valid),
		.out_data(cmdparser_packeter_data)
	);

endmodule
