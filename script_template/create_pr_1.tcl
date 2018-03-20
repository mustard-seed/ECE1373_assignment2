
make_wrapper -files [get_files 8v3_shell/$projName/$projName.srcs/sources_1/bd/pr_region_2/pr_region_2.bd] -top
add_files -norecurse 8v3_shell/$projName/$projName.srcs/sources_1/bd/pr_region_2/hdl/pr_region_2_wrapper.v
update_compile_order -fileset sources_1

generate_target {synthesis implementation} [get_files 8v3_shell/$projName/$projName.srcs/sources_1/bd/pr_region_2/pr_region_2.bd]
export_ip_user_files -of_objects [get_files 8v3_shell/$projName/$projName.srcs/sources_1/bd/pr_region_2/pr_region_2.bd] -no_script -sync -force -quiet
create_ip_run [get_files -of_objects [get_fileset sources_1] 8v3_shell/$projName/$projName.srcs/sources_1/bd/pr_region_2/pr_region_2.bd]
foreach run_name [get_runs *_synth*] {
  reset_run ${run_name}
}
launch_runs [get_runs *_synth*] -jobs 12
foreach run_name [get_runs *_synth*] {
  wait_on_run ${run_name}
}
synth_design -top pr_region_2_wrapper -mode out_of_context
write_checkpoint -force 8v3_shell/$projName.dcp
close_project
source script_template/gen_pr.tcl
 
