#CALL THIS SCRIPT FROM THE PARENT FOLDER!!!!
#Set project name -- Change for other projects!!!!
set projName accelerator_pr

#Open the project, regardless of whether it exists
create_project -f $projName 8v3_shell/$projName -part xcvu095-ffvc1517-2-e
#Add project ip directories. Change for other projects!
set_property  ip_repo_paths  {8v3_shell/ocl_ips hls_proj/accelerator/solution_partitionAndParallel/impl/ip} [current_project]
update_ip_catalog

#source pr_region_2_bd.tcl
#source 8v3_shell/nn_bd.tcl
source accelerator_bd.tcl
source script_template/create_pr_1.tcl