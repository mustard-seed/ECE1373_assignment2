#Set project name -- Change for other projects!!!!
set projName pr_region_test_proj

#Open the project, regardless of whether it exists
create_project -f pr_region 8v3_shell/$projName -part xcvu095-ffvc1517-2-e
#Add project ip directories. Change for other projects!
set_property  ip_repo_paths  {8v3_shell/ocl_ips hls_proj/conv_proj/solution1/impl/ip hls_proj/fc_proj/solution1/impl/ip } [current_project]
update_ip_catalog

#source pr_region_2_bd.tcl
source 8v3_shell/nn_bd.tcl

