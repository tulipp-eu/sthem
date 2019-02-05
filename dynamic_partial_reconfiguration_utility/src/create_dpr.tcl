##################################################################################
#
#  This file is part of the TULIPP Dynamic Partial Utility
#
#  Copyright 2018 Ahmed Sadek, Technische Universität Dresden, TULIPP EU Project
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
##################################################################################


set time_start [clock seconds]

set dpr_input dpr_input.tcl

set dpr_open [open $dpr_input r]

while {[gets $dpr_open line] != -1} {
    set s1 [regsub -all { } $line ""]
    if {[regexp {Main_Module+"(.*)"} $s1 all value]} {
        set mainModule $value
    }
    if {[regexp {Reconfigurable_Modules+"(.*)"} $s1 all value]} {
        set rModules [split $value ,]
    }
    if {[regexp {Xilinx_Build_Directory+"(.*)"} $s1 all value]} {
        set dirProjBuild $value
    }
}

close $dpr_open


set allModules [lsort -unique [list {*}$mainModule {*}$rModules]]


set supportedVers "2017.2 2017.3 2017.4 2018.1 2018.2 2018.3"
set xyzVer [version -short]
set xyVer [string range $xyzVer 0 5]
set xVer [string range $xyzVer 0 3]
set sdxVer [lsearch -all -inline ${supportedVers} ${xyVer}]

if {${xVer} == "2017"} {
set pPath "_vpl/ipi/syn"
set pName "syn"
} elseif {${xVer} == "2018"} {
set pPath "vivado/prj"
set pName "prj"
}


set language "VHDL"
set board ""
set hwPlatform ""
set chip ""

set dirProjVivado ${dirProjBuild}/_sds/p0/${pPath}
set dirProjRepo ${dirProjBuild}/_sds/iprepo/repo

set OS $tcl_platform(platform)
if {$OS == "unix"} {
    set dirXilinx "/opt/Xilinx"
} elseif {$OS == "windows"} {
    set dirXilinx "C:/Xilinx"
} else {
    puts "Unknown operating system!"
    exit
}
set dirXilinxIp "${dirXilinx}/SDx/${xyVer}/data/ip/xilinx"

file mkdir ${dirProjBuild}/AutoDPR
set dirDprWorkspace ${dirProjBuild}/AutoDPR

file mkdir $dirDprWorkspace/pr_synth
file mkdir $dirDprWorkspace/pr_design
file mkdir $dirDprWorkspace/pr_design/${mainModule}
file mkdir $dirDprWorkspace/pr_design/blank
foreach rm $rModules {
    file mkdir $dirDprWorkspace/pr_design/$rm
}
file mkdir $dirDprWorkspace/pr_modules
file mkdir $dirDprWorkspace/pr_reports
file mkdir $dirDprWorkspace/pr_bitstreams

#####################################################
# Open SDSoC pre-synthesised design
#####################################################

open_project ${dirProjVivado}/${pName}.xpr

if {[llength [get_files *.bd]] == 1 } {
  set bd_file [get_files *.bd]
} else {
  puts "Board file is not set"
}

open_bd_design $bd_file

lappend board [current_board_part]
lappend hwPlatform [current_bd_design]
lappend chip [get_property PART_NAME [current_board_part]]


if {$chip == "xc7z020clg484-1"} {
    puts "Floorplanning for Zedboard          :$chip              "
    set floorplan "SLICE_X0Y0:SLICE_X113Y49 DSP48_X0Y0:DSP48_X4Y19 RAMB18_X0Y0:RAMB18_X5Y19 RAMB36_X0Y0:RAMB36_X5Y9"
} elseif {$chip == "xc7z030sbg485-1"} {
    puts "Floorplanning for Tulipp            :$chip              "
    set floorplan "SLICE_X50Y0:SLICE_X111Y49 DSP48_X3Y0:DSP48_X4Y19 RAMB18_X3Y0:RAMB18_X5Y19 RAMB36_X3Y0:RAMB36_X5Y9"
} elseif {$chip == "xczu3eg-sfvc784-1-e"} {
    puts "Floorplanning for Tulipp UltraScale :$chip              "
    set floorplan "SLICE_X3Y60:SLICE_X6Y119 DSP48E2_X0Y24:DSP48E2_X0Y47 RAMB18_X0Y24:RAMB18_X0Y47 RAMB36_X0Y12:RAMB36_X0Y23"
} else {
    puts "Board is not exist in the database"
}


if {[regexp {^xc7z} $chip]} {
    set ps [get_bd_cells * -filter {VLNV =~ "xilinx.com:ip:processing_system7:*"}]    
} elseif {[regexp {^xczu} $chip]} {
    set ps [get_bd_cells * -filter {VLNV =~ "xilinx.com:ip:zynq_ultra_ps_e:*"}]
} else {
    puts "Device not supported"
}

puts "board            : ${board}             "
puts "hwPlatform       : ${hwPlatform}        "
puts "chip             : ${chip}              "
puts "Processing system: ${ps}                "

#####################################################
# Add decoupler for main Module (first PR region)
#####################################################


set mpins [get_bd_intf_pins -of [get_bd_cells ${mainModule}_1]]

set mapping [list "/${mainModule}_1/" "${mainModule}_1_"]

set master_list {}
foreach pin ${mpins} {
    if {[get_property MODE [get_bd_intf_pins ${pin}]] == "Master"} {
        lappend master_list [string map $mapping ${pin}]
        }
    }

startgroup
if {[regexp {^xc7z} $chip]} {
    set_property -dict [list CONFIG.PCW_QSPI_GRP_SINGLE_SS_ENABLE {1} CONFIG.PCW_GPIO_EMIO_GPIO_ENABLE {1} CONFIG.PCW_GPIO_EMIO_GPIO_IO {1}] [get_bd_cells ${ps}]
} elseif {[regexp {^xczu} $chip]} {
    set_property -dict [list CONFIG.PSU__GPIO_EMIO__PERIPHERAL__ENABLE {1} CONFIG.PSU__GPIO_EMIO__PERIPHERAL__IO {1}] [get_bd_cells ${ps}]
} 


foreach ml ${master_list} {

startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:pr_decoupler:1.0 pr_decoupler_${ml}
endgroup

set_property -dict [list CONFIG.ALL_PARAMS {INTF {intf_0 {ID 0 VLNV xilinx.com:interface:axis_rtl:1.0 MODE slave}}} CONFIG.GUI_SELECT_INTERFACE {0} CONFIG.GUI_INTERFACE_NAME {intf_0} CONFIG.GUI_SELECT_VLNV {xilinx.com:interface:axis_rtl:1.0} CONFIG.GUI_SELECT_MODE {slave} CONFIG.GUI_SIGNAL_SELECT_0 {TVALID} CONFIG.GUI_SIGNAL_SELECT_1 {TREADY} CONFIG.GUI_SIGNAL_SELECT_2 {TDATA} CONFIG.GUI_SIGNAL_SELECT_3 {TUSER} CONFIG.GUI_SIGNAL_SELECT_4 {TLAST} CONFIG.GUI_SIGNAL_SELECT_5 {TID} CONFIG.GUI_SIGNAL_SELECT_6 {TDEST} CONFIG.GUI_SIGNAL_SELECT_7 {TSTRB} CONFIG.GUI_SIGNAL_SELECT_8 {TKEEP} CONFIG.GUI_SIGNAL_DECOUPLED_0 {true} CONFIG.GUI_SIGNAL_DECOUPLED_1 {true} CONFIG.GUI_SIGNAL_PRESENT_0 {true} CONFIG.GUI_SIGNAL_PRESENT_1 {true} CONFIG.GUI_SIGNAL_PRESENT_2 {true} CONFIG.GUI_SIGNAL_PRESENT_4 {true} CONFIG.GUI_SIGNAL_WIDTH_2 {32} CONFIG.GUI_SIGNAL_WIDTH_7 {4} CONFIG.GUI_SIGNAL_WIDTH_8 {4}] [get_bd_cells pr_decoupler_${ml}]


if {[regexp {^xc7z} $chip]} {
    connect_bd_net [get_bd_pins ${ps}/GPIO_O] [get_bd_pins pr_decoupler_${ml}/decouple]
} elseif {[regexp {^xczu} $chip]} {
    connect_bd_net [get_bd_pins ${ps}/emio_gpio_i] [get_bd_pins pr_decoupler_${ml}/decouple]
}


set interface [get_bd_intf_pins -of_objects [get_bd_intf_nets ${ml}]]

delete_bd_objs [get_bd_intf_nets ${ml}]

connect_bd_intf_net [get_bd_intf_pins [lindex $interface 0]] [get_bd_intf_pins pr_decoupler_${ml}/s_intf_0]
connect_bd_intf_net [get_bd_intf_pins [lindex $interface 1]] [get_bd_intf_pins pr_decoupler_${ml}/rp_intf_0]
}



update_compile_order -fileset sim_1
update_compile_order -fileset sources_1
set_property synth_checkpoint_mode none [get_files $bd_file]
check_ip_cache -use_project_cache

generate_target all [get_files $bd_file]

#####################################################
# Synthesis design
#####################################################
set_property STEPS.OPT_DESIGN.IS_ENABLED true [get_runs impl_1]
set_property STEPS.OPT_DESIGN.ARGS.DIRECTIVE Default [get_runs impl_1]
set_msg_config -id {[Pfi 67-13]} -new_severity INFO
reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1

set synth_ok 0
set synth_status [get_property STATUS [get_runs synth_1]]
set synth_progress [get_property PROGRESS [get_runs synth_1]]
if {$synth_status == "synth_design Complete!" && $synth_progress == "100%"} {
  set synth_ok 1
}
if {$synth_ok == 0} {
  puts "ERROR: \[SDSoC 0-0\]: Synthesis failed : status $synth_status : progress $synth_progress"
  exit 1
}


file copy -force ${dirProjVivado}/${pName}.runs/synth_1/${hwPlatform}_wrapper.dcp $dirDprWorkspace/pr_synth
close_project

open_checkpoint $dirDprWorkspace/pr_synth/${hwPlatform}_wrapper.dcp
update_design -cell ${hwPlatform}_i/${mainModule}_1 -black_box
write_checkpoint -force $dirDprWorkspace/pr_synth/${hwPlatform}_bb_wrapper.dcp
close_project

#####################################################
# Reconfigurable modules: Setup
#####################################################
foreach rm $allModules {
    create_project -force ${rm} $dirDprWorkspace/pr_modules/${rm} -part ${chip}
    set_property board_part ${board} [current_project]
    set_property target_language ${language} [current_project]

    set_property ip_repo_paths "${dirProjRepo} ${dirXilinxIp}" [current_fileset]
    update_ip_catalog -rebuild

    create_ip -name ${rm} -vendor xilinx.com -library hls -version 1.0 -module_name ${rm}_1
    generate_target {instantiation_template} [get_files ${dirDprWorkspace}/pr_modules/${rm}/${rm}.srcs/sources_1/ip/${rm}_1/${rm}_1.xci]
    generate_target all [get_files  ${dirDprWorkspace}/pr_modules/${rm}/${rm}.srcs/sources_1/ip/${rm}_1/${rm}_1.xci]
    export_ip_user_files -of_objects [get_files ${dirDprWorkspace}/pr_modules/${rm}/${rm}.srcs/sources_1/ip/${rm}_1/${rm}_1.xci] -no_script -force -quiet
    create_ip_run [get_files -of_objects [get_fileset sources_1] ${dirDprWorkspace}/pr_modules/${rm}/${rm}.srcs/sources_1/ip/${rm}_1/${rm}_1.xci]
    launch_run -jobs 8 ${rm}_1_synth_1
    wait_on_run ${rm}_1_synth_1
    export_simulation -of_objects [get_files ${dirDprWorkspace}/pr_modules/${rm}/${rm}.srcs/sources_1/ip/${rm}_1/${rm}_1.xci] -directory ${dirDprWorkspace}/pr_modules/${rm}/${rm}.ip_user_files/sim_scripts -force -quiet
    file copy -force ${dirDprWorkspace}/pr_modules/${rm}/${rm}.runs/${rm}_1_synth_1/${rm}_1.dcp $dirDprWorkspace/pr_synth/${rm}.dcp
    close_project

}

#####################################################
# Prepare Dynamic Partial Reconfiguration
#####################################################

open_checkpoint $dirDprWorkspace/pr_synth/${hwPlatform}_wrapper.dcp
set_property HD.RECONFIGURABLE 1 [get_cells ${hwPlatform}_i/${mainModule}_1]
write_checkpoint -force $dirDprWorkspace/pr_design/config${mainModule}.dcp


#####################################################
# Floorplanning
#####################################################
create_pblock pblock_${mainModule}
resize_pblock [get_pblocks pblock_${mainModule}] -add ${floorplan}
add_cells_to_pblock [get_pblocks pblock_${mainModule}] [get_cells [list ${hwPlatform}_i/${mainModule}_1]] -clear_locs

set_property RESET_AFTER_RECONFIG 1 [get_pblocks pblock_${mainModule}]
set_property SNAPPING_MODE ON [get_pblocks pblock_${mainModule}]

create_drc_ruledeck ruledeck_1
add_drc_checks -ruledeck ruledeck_1 [get_drc_checks {HDPR-43 HDPR-33 HDPR-24 HDPR-21 HDPR-20 HDPR-41 HDPR-40 HDPR-30 HDPR-68 HDPR-67 HDPR-66 HDPR-65 HDPR-64 HDPR-63 HDPR-62 HDPR-61 HDPR-60 HDPR-59 HDPR-58 HDPR-57 HDPR-55 HDPR-54 HDPR-53 HDPR-51 HDPR-50 HDPR-49 HDPR-48 HDPR-47 HDPR-46 HDPR-45 HDPR-44 HDPR-42 HDPR-38 HDPR-37 HDPR-36 HDPR-35 HDPR-34 HDPR-32 HDPR-29 HDPR-28 HDPR-27 HDPR-26 HDPR-25 HDPR-23 HDPR-22 HDPR-19 HDPR-18 HDPR-17 HDPR-16 HDPR-15 HDPR-14 HDPR-13 HDPR-12 HDPR-11 HDPR-10 HDPR-9 HDPR-8 HDPR-7 HDPR-6 HDPR-5 HDPR-4 HDPR-3 HDPR-2 HDPR-1}]
report_drc -name drc_1 -ruledecks {ruledeck_1} -file {drc.log}
delete_drc_ruledeck ruledeck_1
write_xdc -force $dirDprWorkspace/pr_design/fplan.xdc

if {[file isdirectory ${dirProjVivado}/${pName}.srcs/constrs_1] == 1} {
    read_xdc [glob ${dirProjVivado}/${pName}.srcs/constrs_1/imports/new/*]
}


#####################################################
# full/static Design Configuration
#####################################################

opt_design
place_design
route_design
write_checkpoint -force $dirDprWorkspace/pr_design/${mainModule}/route_design.dcp

set DCP_${mainModule} "${dirDprWorkspace}/pr_design/${mainModule}/route_design.dcp"
report_utilization -file $dirDprWorkspace/pr_reports/${mainModule}_utilization.rpt
report_timing_summary -file $dirDprWorkspace/pr_reports/${mainModule}_timing.rpt

write_checkpoint -force -cell ${hwPlatform}_i/${mainModule}_1 $dirDprWorkspace/pr_modules/${mainModule}_route.dcp

# Static design
update_design -cells ${hwPlatform}_i/${mainModule}_1 -black_box

lock_design -level routing
write_checkpoint -force $dirDprWorkspace/pr_design/configStatic_route.dcp
close_project

#####################################################
# DPR First Partition Configuration
#####################################################
foreach rm $rModules {
    open_checkpoint $dirDprWorkspace/pr_design/configStatic_route.dcp
    read_checkpoint -cell ${hwPlatform}_i/${mainModule}_1 $dirDprWorkspace/pr_synth/${rm}.dcp

    opt_design
    place_design
    route_design
    write_checkpoint -force $dirDprWorkspace/pr_design/${rm}/route_design.dcp

    set DCP_${rm} "${dirDprWorkspace}/pr_design/${rm}/route_design.dcp"
    report_utilization -file $dirDprWorkspace/pr_reports/${rm}_utilization.rpt
    report_timing_summary -file $dirDprWorkspace/pr_reports/${rm}_timing.rpt

    write_checkpoint -force -cell ${hwPlatform}_i/${mainModule}_1 $dirDprWorkspace/pr_modules/${rm}_route.dcp
    close_project

}
#####################################################
# DPR Blank Configuration
#####################################################
open_checkpoint $dirDprWorkspace/pr_design/configStatic_route.dcp
update_design -buffer_ports -cell ${hwPlatform}_i/${mainModule}_1

place_design
route_design
write_checkpoint -force $dirDprWorkspace/pr_design/blank/route_design.dcp


set DCP_blank "${dirDprWorkspace}/pr_design/blank/route_design.dcp"

foreach rm $rModules {
    pr_verify -initial [expr \$DCP_${mainModule}] -additional "$DCP_blank [expr \$DCP_${rm}]"
}

close_project


#####################################################
# Bitstreams Generation
#####################################################
open_checkpoint [expr \$DCP_${mainModule}]
write_bitstream -force -file $dirDprWorkspace/pr_bitstreams/${mainModule}.bit
write_cfgmem -format BIN -interface SMAPx32 -disablebitswap -loadbit "up 0x0 $dirDprWorkspace/pr_bitstreams/${mainModule}_pblock_${mainModule}_partial.bit" $dirDprWorkspace/pr_bitstreams/${mainModule}_${mainModule}.bin
close_project

foreach rm $rModules {
    open_checkpoint [expr \$DCP_${rm}]
    write_bitstream -force -file $dirDprWorkspace/pr_bitstreams/${rm}.bit
    write_cfgmem -format BIN -interface SMAPx32 -disablebitswap -loadbit "up 0x0 $dirDprWorkspace/pr_bitstreams/${rm}_pblock_${mainModule}_partial.bit" $dirDprWorkspace/pr_bitstreams/${rm}_${mainModule}.bin
    close_project
}

open_checkpoint $DCP_blank
write_bitstream -force -file $dirDprWorkspace/pr_bitstreams/blank.bit
write_cfgmem -format BIN -interface SMAPx32 -disablebitswap -loadbit "up 0x0 $dirDprWorkspace/pr_bitstreams/blank_pblock_${mainModule}_partial.bit" $dirDprWorkspace/pr_bitstreams/blank_${mainModule}.bin

close_project


set time_finish [clock seconds]

set time_diff [expr {$time_finish - $time_start}]

puts "Time taken = $time_diff "
puts "The time is: [clock format $time_diff -gmt 1 -format %H:%M:%S]"

