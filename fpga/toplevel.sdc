## Generated SDC file "toplevel.out.sdc"

## Copyright (C) 2016  Intel Corporation. All rights reserved.
## Your use of Intel Corporation's design tools, logic functions 
## and other software and tools, and its AMPP partner logic 
## functions, and any output files from any of the foregoing 
## (including device programming or simulation files), and any 
## associated documentation or information are expressly subject 
## to the terms and conditions of the Intel Program License 
## Subscription Agreement, the Intel Quartus Prime License Agreement,
## the Intel MegaCore Function License Agreement, or other 
## applicable license agreement, including, without limitation, 
## that your use is for the sole purpose of programming logic 
## devices manufactured by Intel and sold by Intel or its 
## authorized distributors.  Please refer to the applicable 
## agreement for further details.


## VENDOR  "Altera"
## PROGRAM "Quartus Prime"
## VERSION "Version 16.1.0 Build 196 10/24/2016 SJ Lite Edition"

## DATE    "Fri Jul 15 19:01:26 2242"

##
## DEVICE  "10M08SCU169C8G"
##


#**************************************************************
# Time Information
#**************************************************************

set_time_format -unit ns -decimal_places 3



#**************************************************************
# Create Clock
#**************************************************************

create_clock -name {CLK_INT} -period 83.333 -waveform { 0.000 41.666 } [get_ports {CLK_INT}]
create_clock -name {FT_CLK} -period 16.666 -waveform { 0.000 8.333 } [get_ports { FT_CLK }]


#**************************************************************
# Create Generated Clock
#**************************************************************

create_generated_clock -name {pll|altpll_component|auto_generated|pll1|clk[0]} -source [get_pins {pll|altpll_component|auto_generated|pll1|inclk[0]}] -duty_cycle 50/1 -multiply_by 10 -master_clock {CLK_INT} [get_pins {pll|altpll_component|auto_generated|pll1|clk[0]}] 


#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************

set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -rise_to [get_clocks {FT_CLK}]  0.070  
set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -fall_to [get_clocks {FT_CLK}]  0.070  
set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -setup 0.110  
set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -hold 0.130  
set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -setup 0.110  
set_clock_uncertainty -rise_from [get_clocks {FT_CLK}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -hold 0.130  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -rise_to [get_clocks {FT_CLK}]  0.070  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -fall_to [get_clocks {FT_CLK}]  0.070  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -setup 0.110  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -hold 0.130  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -setup 0.110  
set_clock_uncertainty -fall_from [get_clocks {FT_CLK}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -hold 0.130  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {FT_CLK}] -setup 0.130  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {FT_CLK}] -hold 0.110  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {FT_CLK}] -setup 0.130  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {FT_CLK}] -hold 0.110  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}]  0.070  
set_clock_uncertainty -rise_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}]  0.070  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {FT_CLK}] -setup 0.130  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {FT_CLK}] -hold 0.110  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {FT_CLK}] -setup 0.130  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {FT_CLK}] -hold 0.110  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -rise_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}]  0.070  
set_clock_uncertainty -fall_from [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}] -fall_to [get_clocks {pll|altpll_component|auto_generated|pll1|clk[0]}]  0.070  


#**************************************************************
# Set Input Delay
#**************************************************************



#**************************************************************
# Set Output Delay
#**************************************************************



#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************

set_false_path -from [get_keepers {*rdptr_g*}] -to [get_keepers {*ws_dgrp|dffpipe_h09:dffpipe17|dffe18a*}]
set_false_path -from [get_keepers {*delayed_wrptr_g*}] -to [get_keepers {*rs_dgwp|dffpipe_g09:dffpipe14|dffe15a*}]


#**************************************************************
# Set Multicycle Path
#**************************************************************



#**************************************************************
# Set Maximum Delay
#**************************************************************



#**************************************************************
# Set Minimum Delay
#**************************************************************



#**************************************************************
# Set Input Transition
#**************************************************************

