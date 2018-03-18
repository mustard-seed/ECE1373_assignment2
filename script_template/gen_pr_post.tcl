
proc runPPO { {numIters 1} {enablePhysOpt 1} } {
 for {set i 0} {$i < $numIters} {incr i} {
 place_design -post_place_opt
 if {$enablePhysOpt != 0} {
 phys_opt_design -bram_enable_opt 
 }
 route_design -directive HigherDelayCost
 if {[get_property SLACK [get_timing_paths ]] >= 0} {break}; #stop if timing is met
 }
}

open_checkpoint 8v3_shell/${projName}_routed.dcp
runPPO 4 1

# write_checkpoint -force 8v3_shell/${projName}_routed.dcp
# #write_checkpoint -cell static_region_i/pr_region rp2_route_design.dcp
# write_bitstream -force 8v3_shell/$projName.bit 
# report_timing_summary -delay_type min_max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -name timing_1 -file 8v3_shell/$projName.timing
# close_design
