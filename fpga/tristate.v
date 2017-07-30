
module tristate #(
		parameter WIDTH = 1
	) (
		inout wire[WIDTH-1:0] tristate,

		output wire[WIDTH-1:0] out,
		input wire[WIDTH-1:0] value,
		input wire driven
	);

	assign tristate = driven ? value : {WIDTH{1'bZ}};
	assign out = tristate;
endmodule
