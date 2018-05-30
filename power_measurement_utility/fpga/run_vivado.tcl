#=======================================================#
#                                                       #
#  Script to run Vivado 2012.4 Software                 #
# 
#                                                       #
#====================================================== #

#------------------------------------------------------#
#                                                      #
# Please edit the following lines to specify important #
#  project-specific information about your Vivado run  #
#  You only need to edit the parameters pertinant      #
#                 to your design                       #
#                                                      #
#     After editing this information, the script       #
#       may be run with the following command:         #
#                                                      #
#   vivado -mode batch -source run_vivado_2012.4.tcl   #
#                                                      #
#------------------------------------------------------#


#...........................#
#                           #
# Project-level Information #
#                           #
# ..........................#


# Project Name
set project_name lynsyn


# Target Device
set device  xc7a15tftg256-3

set design_name "lynsyn"
set top_name "lynsyn"


# Specify top-level EDIF / NGC / Structural Verilog Name and location
#   - If synthesizing using RDS, this is where the EDIF file will be created/placed
#  Default: ./${design_name}.edf
set edif_name ./${design_name}.edf


# Specify Core files or directories (include path, if more than one, separate in quotes by space)
#  Default: No cores used
set cores ""

# Specify IP (.xci) files to be included in the project
# Must be in separate directories
set ip ""

# Specify XDC file names (include path if different directory, if more than one, separate in quotes by space)
#  Vivado allows the specifying or changing of constraints at any place in the flow, choose the appropriate file(s) to choose when to apply the constraint:
#    constraints_pre_synthesis  - Used for entire flow unless overridden by a later applied constraint
#    constraints_post_synthesis - Used for P&R but not synthesis unless overridden by a later applied constraint
#    constraints_post_place     - Used for routing only unless overridden by a later applied constraint
#    constraints_post_route     - Used for analysis only
#  If synthesizing using RDS and using a synthesis constraint, the same constraint will be used for implementation and is not necessary to re-specify here
#  Default: No constraints used
set constraints_pre_synthesis ""
set constraints_post_synthesis "../lynsyn.xdc"
set constraints_post_place ""
set constraints_post_route ""


# If desired to run physical synthesis optimization set this to a 1, set to 0 for normal flow
#  Default: 0 - Do not run physical synthesis optimization
set phys_opt 0


# If desired to run power optimization set this to a 1, set to 0 for normal flow
#  Default: 0 - Do not run power optimization
set power_opt 0


# If desired to create a bitstream set this to a 1, set to 0 to skip this step
#  Default: 0 - Do not create bitstream
set create_bitstream 1


# If create_bitstream is set to 1, this allows additional bitgen options to be specified
#  Default: -w -g UnconstrainedPins:Allow
set bitgen_opts "-w -g UnconstrainedPins:Allow"


#.......................#
#                       #
# Synthesis Information #
#                       #
# ......................#


# Specify synthesis tool to use or was used
#  Accepted values are RDS, XST, Synplify_pro, Synplify_premier or Precision
#  If HDL files are specified and RDS is specified, synthesis will be run
#  Otherwise an EDIF or NGC file is expected at the location specified by netlist_name
#  Default: RDS
set synthesis_tool RDS


# If using RDS, specify HDL file in the format:
#   set hdl_files {library_name {files including path sepearted by space or carriage return}}
#  If no custom libraries necessary, place all files in work library, Mixed language OK
#  Leave blank if synthesizing outside of Vivado script
#  Example:
#   set hdl_files {work {top.v lower1.vhd lower2.vhd} mylib {lower3.vhd}}
#  Default: No hdl files - supplying an EDIF/NGC netlist
set hdl_files {
    work ../rtl/lynsyn.v
}

# Specify HDL directory, adds all HDL files from specified directory for synthesis
#  If synthesizing, must at least specify top-level file with the hdl_files variable
#  Leave blank if specifying individual files or not running synthesis
#  Example:
#   set hdl_dirs {dir1 dir2 dir3}
#  Default: No hdl directories
set hdl_dirs {
    ../rtl
}

# Specify Verilog Include Directories
#  Leave blank if not using Verilog include directories or not running synthesis
#  Example:
#   set include_dirs {dir1 dir2 dir3}
#  Default: No include directories
set include_dirs {../rtl}

# Specify Verilog Golbal Include Files - Files that include global `defines and other constructs that must appear at top of comoile order
#  Leave blank if not using Verilog global include directories or not running synthesis
#  Example:
#   set global_include_files {<file_including_dir> <file_including_dir>}
#  Default: No global include files
set global_include_files {}


# Specify Verilog Defines
#  Leave blank if not specifying Verilog defines or not running synthesis
#  Example:
#   set verilog_defines "define1=value define2=value define3"
#  Default: No verilog defines
set verilog_defines ""


#.....................#
#                     #
# Optional Parameters #
#                     #
# ....................#


# Create script-only.  Setting this to a 1 will create a TCL run script but not execute the run.
#  This can be useful to further modify the flow if needed by editing the output TCL script.
set create_script_only 0


# Set any optional runtime parameters for the run here.  Encase each parameter in {}.
# Example: set runtime_parameters {{set place.givegoodresults yes} {set route.givegoodruntime 1}}
#  Default: No optional runtime parameters
set runtime_parameters {}


# Set any optional parameters for any step of the flow
#  Example: set synth_design__options "-flatten_hierarchy auto -bufg 24"
# Example: set opt_design__options "-verbose"
#  Default: flow with run with default options
# Run help synth_design to see all valid options
set synth_design__options "-flatten_hierarchy none"
# Valid options for opt_design are: -sweep, -retarget, -propconst, -remap, -verbose, -quiet
set opt_design__options ""
# Valid options for power_opt_design are: -verbose, -quiet
set power_opt_design__options ""
# Valid options for place_design are: -effort_level [quick|low|medium|high], -no_timing_driven, -quiet, -verbose
set place_design__options ""
# Valid options for phys_opt_design are: -quiet, -verbose
set phys_opt_design__options ""
# Run help route_design to see all valid options
set route_design__options ""
# Run help write_bitstream to see all valid options
set write_bitstream__options ""


# Specify any scripts to be sourced during the flow (include path if different directory, if more than one, separate in quotes by space)
# Example: set script_post_synthesis "loc_brams.tcl"
#  Default: No extra scripts used
set script_pre_synthesis ""
set script_post_synthesis ""
set script_post_place ""
set script_post_route ""


# Specify which reports you want to create
#  Possible values: 0 - Create only utilization and some timing reports (best runtime and disk space)
#                   1 - Create most important reports (balanced)
#                   2 - Create all relevent reports (most design visibility)
#  Default: 1 
set report_types 1


# If you wish to create a checkpoint at every step of the flow (increases runtime but allows reload in case of tool crash)
#  Possible values: 0 - Only create checkpoint at run completeion or error (best runtime)
#                   1 - Create and save checkpoints from every step of the flow (allows loading at any point in the flow)
#                   2 - Save checkpoint at every step of the flow but delete the prior checkpoint to save disk space
#  Default: 2 
set create_frequent_checkpoint 2


# If you wish to create sub-directories to store reports from different stages of the run (1 or 0)
#  Default: 1 - Create report sub directories
set create_report_directories 1


# Archive all log files to send to Xilinx
#  Default: 1 - Archive all generated log files upon run completion
set archive_logs 1


# Archive all database files to send to Xilinx
#  Default: 1 - Archive all generated database files upon run completion
set archive_database 1


# Creates a logic function stripped output to send to Xilinx when set to 1
#  All LUT INIT strings changed to XOR, all BRAM INITs set to 0
#  Default: 0 - Do not create a logic function stripped output
set logic_function_stripped 0


#######################################
#### Do not edit below this line ######
#######################################

proc run_command {cmd} {
  global design_name logic_function_stripped begin_time tcl_script time_log stage create_script_only create_frequent_checkpoint
  set cmd [string trim $cmd]
  puts $tcl_script "$cmd"
  flush $tcl_script
  if {!$create_script_only} {
    puts "Running at [clock format [clock seconds]]: $cmd"
    if {([lindex $cmd 0] == "exec")||([lindex $cmd 0] == "file")} {
      set cmd_short [lrange $cmd 0 1]
    } elseif {[lindex $cmd 0] == "report_timing"} {
      set cmd_short "[lrange $cmd 0 1] ($stage)"
    } elseif {[regexp {report.+} $cmd]} {
      set cmd_short "[lindex $cmd 0] ($stage)"
    } else  {
      set cmd_short [lindex $cmd 0]
    }
    set start_time [clock seconds]
    if {[catch {eval $cmd} result]} {
      puts "ERROR found when running $cmd.\n\tSaving checkpoint..."
      puts "\t$cmd_short showed error code $result and took [expr ([clock seconds]-$start_time)/3600] hour(s), [expr (([clock seconds]-$start_time)%3600)/60] minute(s) and [expr (([clock seconds]-$start_time)%3600)%60] second(s)."
      lappend time_log "[format "%02u:%02u:%02u - $cmd_short" [expr ([clock seconds]-$start_time)/3600] [expr (([clock seconds]-$start_time)%3600)/60] [expr (([clock seconds]-$start_time)%3600)%60]]"
      puts "\tEntire run took [expr ([clock seconds]-$begin_time)/3600] hour(s), [expr (([clock seconds]-$begin_time)%3600)/60] minute(s) and [expr (([clock seconds]-$begin_time)%3600)%60] second(s)."
      puts $tcl_script "\n# An error occured when running $cmd_short\n# Please consult prior run log/report files for details."

      set stage ${stage}_debug
      set create_frequent_checkpoint 1
      checkpoint
      close $tcl_script
      if {[file exists vivado.log]} {
        file copy -force vivado.log ${design_name}.rdi
      }
      puts "Vivado run completed with error on [clock format [clock seconds]]."
      archive_files
      puts "The script run_vivado_2012.2.tcl completed with error."
      exit 5
    } else {
      puts "[format "%02u:%02u:%02u - $cmd_short" [expr ([clock seconds]-$start_time)/3600] [expr (([clock seconds]-$start_time)%3600)/60] [expr (([clock seconds]-$start_time)%3600)%60]]"
      lappend time_log "[format "%02u:%02u:%02u - $cmd_short" [expr ([clock seconds]-$start_time)/3600] [expr (([clock seconds]-$start_time)%3600)/60] [expr (([clock seconds]-$start_time)%3600)%60]]"
    }
  }
}

proc checkpoint {} {
  global tcl_script create_frequent_checkpoint time_log create_script_only stage design_name last_dcp logic_function_stripped
  set start_time [clock seconds]
  if {($create_frequent_checkpoint != 0) || ($stage == "route")} {
    puts $tcl_script "write_checkpoint -force ./${design_name}_${stage}.dcp"
    if {$logic_function_stripped && (($stage == "opt") || ($stage == "route"))} {
      puts $tcl_script "\n# Writing Checkpoint"
      puts $tcl_script "write_checkpoint -logic_function_stripped -force ./${design_name}_logic_function_stripped_${stage}.dcp"
    }
    if {!$create_script_only} {
      puts "Writing checkpoint ${design_name}_${stage}.dcp"
      if {[catch {write_checkpoint -force ./${design_name}_${stage}.dcp} result]} {
        puts "Error: Unable to create checkpoint, ${design_name}_${stage}.dcp."
      }
      if {$logic_function_stripped && (($stage == "opt") || ($stage == "route"))} {
        if {[catch {write_checkpoint -logic_function_stripped -force ./${design_name}_logic_function_stripped_${stage}.dcp}]} {
          puts "Error: Unable to create logic_function_stripped checkpoint, ${design_name}_logic_function_stripped_${stage}.dcp."
	}
      }
      puts "[format "%02u:%02u:%02u - write_checkpoint ${design_name}_${stage}.dcp" [expr ([clock seconds]-$start_time)/3600] [expr (([clock seconds]-$start_time)%3600)/60] [expr (([clock seconds]-$start_time)%3600)%60]]"
      lappend time_log "[format "%02u:%02u:%02u - write_checkpoint ${design_name}_${stage}.dcp" [expr ([clock seconds]-$start_time)/3600] [expr (([clock seconds]-$start_time)%3600)/60] [expr (([clock seconds]-$start_time)%3600)%60]]"
    }
  }
  if {$create_frequent_checkpoint == 2} {
    if {[file exists $last_dcp]} {
      puts $tcl_script "# Deleting prior checkpoint $last_dcp"
      puts $tcl_script "file delete $last_dcp"
      if {!$create_script_only} {
        puts "Deleting prior checkpoint $last_dcp"
        file delete $last_dcp
      }
      set last_dcp ${design_name}_${stage}.dcp
    }
  }
}

proc archive_files {} {
  global archive_logs archive_database argv0 design_name orig_netlist_name cores project_name logic_function_stripped time_log os_info machine_info begin_time device start_date constraints_pre_synthesis constraints_post_synthesis constraints_post_place constraints_post_route

  # Renaming log and journal files since they get clobbered then next time Vivado is opened
  if {[file exists vivado.log]} {
    file copy -force vivado.log ${design_name}.rdi
    file copy -force vivado.jou ${design_name}.jou
  }

  # Creating a runtime log to more easily debug runtime issues  
  set runtime_log [open ${design_name}_vivado_runtime.rpt w]
  puts $runtime_log "## Runtime information for Vivado project $project_name, top-level $design_name targeting $device"
  puts $runtime_log "## Using Vivado version [lrange [version] 1 2] [lrange [version] 3 13]"
  puts $runtime_log "## Run started at $start_date\n"
  puts $runtime_log "## Run completed at [clock format [clock seconds]]"
  puts $runtime_log "$os_info"
  puts $runtime_log "$machine_info"
  catch {exec uptime} uptime
  puts $runtime_log "\tUptime/Load output at end: $uptime\n"
  puts $runtime_log "Entire run took [expr ([clock seconds]-$begin_time)/3600] hour(s), [expr (([clock seconds]-$begin_time)%3600)/60] minute(s) and [expr (([clock seconds]-$begin_time)%3600)%60] second(s)."
  puts $runtime_log "\nRuntime of each command ranked longest to shortest."
  puts $runtime_log "  Time"
  puts $runtime_log "HH:MM:SS - Command"
  puts $runtime_log "======== - ======="
  foreach timeline [lsort -decreasing -index 0 -unique $time_log] {
    set a_tmp [split $timeline :]
    scan [lindex $a_tmp 0] %d b_tmp
    scan [lindex $a_tmp 1] %d c_tmp
    if {[expr $b_tmp + $c_tmp] < 1} {
      lappend one_min [lrange  $timeline 2 end]
    } else {
      puts $runtime_log "$timeline" 
    }
  }
  if {[llength $one_min]} {
    puts $runtime_log "\nThe following commands took less than one minute:\n[join [lsort -unique $one_min] ", "]"
  }
  close $runtime_log

  if {$archive_logs} {
    set log_files "[glob -nocomplain ${design_name}.jou ${design_name}.rdi *.rpt *.drc *.tdr post-*_reports]"
    puts "Archiving log files:\n$log_files"
    if {[llength $log_files]} {
      if {[catch {eval exec zip -r ${design_name}_vivado_log_files.zip $log_files} result]} {
        puts "Unable to zip the log files: $result\nTrying tar..."
        if {[catch {eval exec tar cvzf ${design_name}_vivado_log_files.tgz $log_files} result]} {
	  puts "Unable to tar the log files: $result"
        } else {
	  puts "Created ${design_name}_vivado_log_files.tgz.\nPlease send this file to Xilinx for analysis of the run.\n"
        }
      } else {
        puts "Created ${design_name}_vivado_log_files.zip.\nPlease send this file to Xilinx for analysis of the run.\n"
      }
    } else {
      puts "Could not locate any log files to archive."
    }
  }
  if {$archive_database} {
    if {$logic_function_stripped} {
      foreach database_file "[glob -nocomplain ${design_name}_logic_function_stripped_opted.* ${design_name}_logic_function_stripped_debug.* ${design_name}_logic_function_stripped_routed.* ${design_name}.xpe]" {
        if {[file exists $database_file]} {
          lappend database_files $database_file
        }
      }
      set archive_name ${design_name}_logic_function_stripped_vivado_database_files
    } else {
      set database_files $argv0
      foreach database_file "$orig_netlist_name $cores ${design_name}.ncd ${design_name}.pcf [glob -nocomplain *.dcp] ${design_name}_vivado_rerun.tcl $constraints_pre_synthesis $constraints_post_synthesis $constraints_post_place $constraints_post_route ${design_name}.xpe" {
        if {[file exists $database_file]} {
          lappend database_files $database_file
        }
      }
      set archive_name ${design_name}_vivado_database_files
    }
    puts "Archiving database files:\n$database_files"
    if {[llength $database_files]} {
      if {[catch {eval exec zip -r ${archive_name}.zip $database_files} result]} {
        if {[catch {eval exec tar cvzf ${archive_name}.tgz $database_files} result]} {
	  puts "Unable to archive the database files"
        } else {
	  puts "Created ${archive_name}.tgz.\nPlease send this file to Xilinx for analysis of the run.\n"
        }
      } else {
        puts "Created ${archive_name}.zip.\nPlease send this file to Xilinx for analysis of the run.\n"
      }
    } else {
      puts "Could not locate any database files to archive."
    }
  }
}

proc create_reports {} {
  global create_report_directories tcl_script constraints_used design_name report_types report_dir stage device create_script_only
  if {$create_report_directories && (($report_types==2) || ($stage=="route") || ($stage=="opt"))} {
    set report_dir post-${stage}_reports
    puts $tcl_script "\n# Creating $report_dir report directory"
    puts $tcl_script "file mkdir $report_dir"
    if {!$create_script_only} {
      if {[file exists $report_dir]} {
        file delete -force $report_dir 
      }
      file mkdir $report_dir
    }
  } else {
    set report_dir .
  }
  if {($report_types==2) || ($stage=="route") || ($stage=="opt")} {
    puts $tcl_script "\n# Creating ${stage} reports"
    run_command "report_utilization -file $report_dir/${design_name}_utilization_${stage}.rpt"
    if {[regexp {xc7v[12][50]00t.+} $device] || [regexp {xc7vx1140t.+} $device]} {
      run_command "report_utilization -slr -append -file $report_dir/${design_name}_utilization_${stage}.rpt"
    }
    run_command "report_timing_summary -max_paths 10 -nworst 1 -input_pins -report_unconstrained -file $report_dir/${design_name}_timing_summary_${stage}.rpt"
    run_command "report_drc -file $report_dir/${design_name}_drc_${stage}.rpt"
  }
  if {(($report_types==1) && ($stage=="opt")) || ($report_types==2)} {
    run_command "report_clock_interaction -file $report_dir/${design_name}_clock_interaction_${stage}.rpt"
  }
  if {(($report_types==1) && ($stage=="route")) || ($report_types==2)} {
    run_command "report_io -file $report_dir/${design_name}_io_${stage}.rpt"
  }
  if {$report_types==2} {
    if {$stage=="opt"} {
      run_command "report_transformed_primitives -file $report_dir/${design_name}_transformed_primitives_${stage}.rpt"
    }
    run_command "check_timing -verbose -file $report_dir/${design_name}_check_timing_${stage}.rpt"
    run_command "report_config_timing -file $report_dir/${design_name}_config_timing_${stage}.rpt"
    run_command "report_timing -setup -no_report_unconstrained -max_paths 10 -nworst 1 -input_pins -sort_by group -file $report_dir/${design_name}_timing_setup_${stage}.rpt"
    run_command "report_timing -hold -no_report_unconstrained -max_paths 10 -nworst 1 -input_pins -sort_by group -file $report_dir/${design_name}_timing_setup_${stage}.rpt"
    run_command "report_control_sets -verbose -file $report_dir/${design_name}_control_sets_${stage}.rpt"
    run_command "report_clocks -file $report_dir/${design_name}_clocks_${stage}.rpt"
    run_command "report_clock_networks -file $report_dir/${design_name}_clock_networks_${stage}.rpt"
    run_command "report_clock_utilization -file $report_dir/${design_name}_clock_utilization_${stage}.rpt"
  }
}

proc read_constraints {constraint_files} {
 global constraints_used tcl_script
  if {[llength $constraint_files]} {
    puts $tcl_script "\n# Adding Constraints"
  }
  foreach constraints_file $constraint_files {
    set constraints_used 1
    run_command "read_xdc $constraints_file"
  }
}

# Renamed this variable from a prior script so doing this to make backward compatible
if {[info exists edif_name] && ![info exists netlist_name]} {
  set netlist_name $edif_name
}

# Initialize script variables
set constraints_used 0
set report_dir .
set orig_netlist_name $edif_name
set begin_time [clock seconds]
set start_date [clock format [clock seconds]]
set stage pre-synthesis
set last_dcp "none"

set all_user_vars { {project_name project1} {device xc7v2000tflg1925-1} {synthesis_tool RDS} {hdl_files ""} {netlist_name ./${design_name}.edf} {hdl_files ""} {hdl_dirs ""} {include_dirs ""} {global_include_files ""} {verilog_defines ""} {cores ""} {constraints_pre_synthesis ""} {constraints_post_synthesis ""} {constraints_post_place ""} {constraints_post_route ""} {phys_opt 0} {power_opt 0} {create_bitstream 0} {bitgen_opts "-g UnconstrainedPins:Allow"} {report_types 1} {create_frequent_checkpoint 2} {runtime_parameters ""} {synth_design__options ""} {opt_design__options ""} {power_opt_design__options ""} {place_design__options ""} {phys_opt_design__options ""} {route_design__options ""} {write_bitstream__options ""} {create_report_directories 1} {archive_logs 1} {logic_function_stripped 0} {script_pre_synthesis ""} {script_post_synthesis ""} {script_post_place ""} {script_post_route ""} {create_script_only 0} }

# Check variables and set defaults
foreach var_check $all_user_vars {
  if {![info exists [lindex $var_check 0]]} {
    puts "Setting [lindex $var_check 0] to default [lindex $var_check 1]."
    set [lindex $var_check 0] [lindex $var_check 1]
  }
}

# Start create vivado_rerun.tcl script
set tcl_script [open ${design_name}_vivado_rerun.tcl w]
puts $tcl_script "# Script generated by run_vivado_2012.2.tcl script to reproduce the run performed on $project_name."
puts $tcl_script "# Original run performed on [clock format [clock seconds]]."
puts $tcl_script "# Using Vivado version [lrange [version] 1 2] [lrange [version] 3 13]"
puts $tcl_script "#\n# To run this script invoke: vivado -mode batch -source ${design_name}_vivado_rerun.tcl\n"

# Gathering machine info to help determine runtime parameters and debug any OS-related issues
puts $tcl_script "\n# Gathering machine info to help determine runtime parameters and debug any OS-related issues"
run_command "report_environment -file report_environment.rpt"

set os_info "OS: Unknown"
set machine_info "Machine: Unknown"
set processor_type "Processor Type: Unknown"
set processor_speed "Processor Speed: Unknown"
set processor_cache "Processor Cache: Unknown"
set processor_num "Number of Processors: Unknown"

if {[catch {exec uname}]} {
   if {[catch {exec cmd.exe /c ver}]} {
      puts "Can not determine machine OS."
   } else {
      puts "Windows OS version [string trim [exec cmd.exe /c ver]] on $env(PROCESSOR_IDENTIFIER) with $env(NUMBER_OF_PROCESSORS) processors."
      set os_info "Windows OS $env(OS) version [string trim [exec cmd.exe /c ver]]"
      set machine_info "Machine Name: $env(COMPUTERNAME)\n\tProcessor type: $env(PROCESSOR_IDENTIFIER)\n\tNumber of Processors: $env(NUMBER_OF_PROCESSORS)"
   }
} elseif {[exec uname -o] == "GNU/Linux"} {
   if {[file exists /etc/redhat-release]} {
      puts "Linux RedHat release:\n\t[exec cat /etc/redhat-release]\n\t[exec uname -a]"
      set os_info "Linux RedHat release:\n\t[exec cat /etc/redhat-release]\n\t[exec uname -a]"
   } elseif {[file exists /etc/SuSE-release]} {
      puts "Linux SuSE release:\n\t[exec cat /etc/SuSE-release]\n\t[exec uname -a]"
      set os_info "Linux SuSE release:\n\t[exec cat /etc/SuSE-release]\n\t[exec uname -a]"
   } else {
      puts "Undetermined Linux distribution version [exec uname -s -r]"
      set os_info "Undetermined Linux distribution version [exec uname -s -r]"
   }
   puts "Machine name [lindex [split [info hostname] .] 0] has [lindex [exec free -m] 7] MB memory."
   set proc_file [open /proc/cpuinfo r]                                                                               
   while {[gets $proc_file line] >= 0} {
      if {[lindex $line 0] == "processor"} {
         set num_processors [lindex $line 2]
         incr num_processors
      }
      if {[lrange $line 0 1] == "model name"} {
         set processor_type [lrange $line 3 end]
      }
      if {[lrange $line 0 1] == "cpu MHz"} {
         set processor_speed [lindex $line 3]
      }
      if {[lrange $line 0 1] == "cache size"} {
         set processor_cache [lrange $line 3 end]
      }
   }
   puts "Processor type: $processor_type"
   puts "Number of Processors: $num_processors"
   puts "Processor speed: $processor_speed"
   puts "Processor cache: $processor_cache"
   catch {exec uptime} uptime
   puts "Uptime/Load output: $uptime]"
   set machine_info "Machine Name: [lindex [split [info hostname] .] 0]\n\tPhysical memory: [lindex [exec free -m] 7] MB memory\n\tProcessor type: $processor_type\n\tNumber of Processors: $num_processors\n\tProcessor speed: $processor_speed\n\tProcessor cache: $processor_cache\n\tUptime/Load output st start: $uptime"
} elseif {[exec uname -o] == "Cygwin"} {
   if {[catch {exec cmd.exe /c ver}]} {
      puts "Can not determine machine OS."
      set os_info "Can not determine machine OS."
   } else {
      puts "Windows OS run under Cygwin version [string trim [exec cmd.exe /c ver]] on $env(PROCESSOR_IDENTIFIER) with $env(NUMBER_OF_PROCESSORS) processors."
      set os_info "Windows OS run under Cygwin version [string trim [exec cmd.exe /c ver]]"
      set machine_info "Machine Name: $env(COMPUTERNAME)\nProcessor type: $env(PROCESSOR_IDENTIFIER)\nNumber of Processors: $env(NUMBER_OF_PROCESSORS)"

   }
} else {
  puts "Unknown UNIX operating system detected: [exec uname -a]."
  pset os_info  "Unknown UNIX operating system detected: [exec uname -a]."
}


# Check the user paths eneterd by user for existance
foreach path_check {hdl_dirs include_dirs global_include_files cores constraints_pre_synthesis constraints_post_synthesis constraints_post_place constraints_post_route script_pre_synthesis script_post_synthesis script_post_place script_post_route} {
  foreach path [eval concat \$$path_check] {
    if {![file exists $path]} {
      puts "Error: File or directory $path in $path_check does not exist"
      exit 13
    }
  }
}

# Check if UCF constraints are being applied
foreach ucf_check "$constraints_pre_synthesis $constraints_post_synthesis $constraints_post_place $constraints_post_route" {
  if {[regexp {[.][Uu][Cc][Ff]$}  [file extension $ucf_check]]} {
    puts "Error: Found UCF file, $ucf_check, specified as a constraint file.\n\tUCF files are not supported in the 2012.2 Vivado release."
      exit 15
  }
}


if {![info exists design_name] || ($design_name=="")} {
  puts "ERROR: A design name must be specified.\n\tPlease edit file above and give the appropriate top-level design name for this run."
  exit 17
}
if {($synthesis_tool != "RDS") || ([llength $hdl_files]==0)} {
  if {![file exists $netlist_name]} {
    puts "Error: EDIF/NGC file $netlist_name does not exist.  Can not implment this design without an EDIF."
    if {$synthesis_tool == "RDS"} {
      puts "\tIf using Vivado synthesis and planning to resynthesize, you must specify at least one HDL file in the hdl_files variable."
    }
    puts "Exiting..."
    exit 14
  }
}
if {![info exists hdl_files]} {
  set hdl_files {}
} else {
  for {set i 0} {$i<[llength $hdl_files]} {incr i 2} {
    foreach design_file [lindex $hdl_files [expr $i + 1]] {
      if {![file isfile $design_file]} {
        puts "Error: Could not locate hdl file: $design_file"
        puts "Exiting..."
	exit 19
      }
    }
  }
}

create_project -in_memory -part $device

# Adding IP
if {[llength $ip]} {
  puts $tcl_script "\n# Adding IP"
  set ip_files ""
  foreach i $ip {
    if {[file isdirectory $ip]} {
      set ip_files "$ip_files [glob -nocomplain $i/*.xci]"
    } else {
      set ip_files "$ip_files $i"
    }
  }
  foreach ip_file $ip_files {
    read_ip $ip_file
    generate_target all [get_ips [file rootname [file tail $ip_file]]]
  }
}

# Adding HDL files if running RDS Synthesis
if {$synthesis_tool == "RDS" && [llength $hdl_files]} {  
  if {[llength $hdl_dirs]} {
    puts $tcl_script "\n# Adding HDL directories"
    foreach hdl_dir $hdl_dirs {
      foreach hdl_file [glob -nocomplain $hdl_dir/*.v glob $hdl_dir/*.V $hdl_dir/*.vh $hdl_dir/*.VH] {
        run_command "read_verilog -library work $hdl_file"
      }
      foreach hdl_file [glob -nocomplain $hdl_dir/*.sv glob $hdl_dir/*.SV] {
        run_command "read_verilog -sv -library work $hdl_file"
      }
      foreach hdl_file [glob -nocomplain $hdl_dir/*.vhd glob $hdl_dir/*.VHD $hdl_dir/*.vhdl $hdl_dir/*.VHDL] {
        run_command "read_vhdl -library work $hdl_file"
      }
    }
  }
  puts $tcl_script "\n# Adding HDL files"
  for {set i 0} {$i<[llength $hdl_files]} {incr i 2} {
    set lib_name [lindex $hdl_files $i]
    foreach design_file [lindex $hdl_files [expr $i + 1]] {
      if {[regexp {[.][vV]$}  [file extension $design_file]] || [regexp {[.][vV]_[Bb][Bb]$}  [file extension $design_file]]} {
        run_command "read_verilog -library $lib_name $design_file"
      } elseif {[regexp {[.][sS][vV]$}  [file extension $design_file]] || [regexp {[.][sS][vV]_[Bb][Bb]$}  [file extension $design_file]]} {
        run_command "read_verilog -sv -library $lib_name $design_file"
      } else {
        run_command  "read_vhdl -library $lib_name $design_file"
      }
    }
  }
  if {[llength $include_dirs]} {
    puts $tcl_script "\n# Verilog Include Directories"
    run_command "set_property verilog_dir \{$include_dirs\} \[current_fileset\]"
  }
  if {[llength $global_include_files]} {
    puts $tcl_script "\n# Verilog Global Include Files"
    foreach include_file $global_include_files {
      run_command "set_property is_global_include true \[get_files -of_objects sources_1 $include_file\]"
    }
  }
  if {[llength $verilog_defines]} {
    puts $tcl_script "\n# Verilog Defines"
    run_command "set_property verilog_define \"$verilog_defines\" \[current_fileset\]"
  }
} else {
  if {[regexp {[.][vV][Mm]$}  [file extension $netlist_name]]} {
    puts $tcl_script "\n# Reading Structural Verilog file"
    run_command "read_verilog $netlist_name"
    set design_top "-top $design_name"
  } else {
    puts $tcl_script "\n# Reading EDIF/NGC file"
    run_command "read_edif $netlist_name"
    set design_top "-edif_top_file $netlist_name"
  }
}

# Adding Cores
if {[llength $cores]} {
  puts $tcl_script "\n# Adding Cores"
  set core_files ""
  foreach core $cores {
    if {[file isdirectory $core]} {
      set core_files "$core_files [glob -nocomplain $core/*.edif $core/*.edn $core/*.edf $core/*.ngc]"
    } else {
      set core_files "$core_files $core"
    }
  }
  foreach core_file $core_files {
    run_command  "read_edif $core_file"
  }
}

# Adding Pre-synthesis Constraints
read_constraints "$constraints_pre_synthesis"

# Sourcing any Pre-synthesis scripts
foreach script $script_pre_synthesis {
  run_command "source $script"
}

# Adding additional runtime parameters to design
if {[llength $runtime_parameters]} {
  puts $tcl_script "\n# Adding additional runtime parameter(s)"
  foreach run_param $runtime_parameters {
    run_command "$run_param"
  }
}

# Running RDS synthesis or else opening EDIF/NGD file
if {$synthesis_tool == "RDS" && [llength $hdl_files]} {
  # Running RDS Synthesis
  set stage synthesis
  puts $tcl_script "\n# Running RDS Synthesis"
  run_command "reorder_files -auto -disable_unused"
  run_command "synth_design -part $device -top $top_name $synth_design__options"

  # Creating post-synthesis report directory if requested
  create_reports

  if {[file exists vivado.log]} {
    run_command "file copy -force vivado.log $report_dir/${design_name}.rds"
  }
  checkpoint
  run_command "write_edif -force $netlist_name"
} else {
  # Linking Design
  puts $tcl_script "\n# Linking Design"
  run_command "link_design -part $device $design_top"
}

# Adding Post-synthesis Constraints
read_constraints "$constraints_post_synthesis"

# Sourcing any Post-synthesis scripts
foreach script $script_post_synthesis {
  run_command "source $script"
}

# Running Logic Optimization
set stage opt
puts $tcl_script "\n# Running Logic Optimization"
run_command "opt_design $opt_design__options"
checkpoint


# Creating Post Optimization Reports
create_reports


# Run Power Optimization if requested
if {$power_opt} {
  set stage power_opt
  puts $tcl_script "\n# Run Power Optimization"
  run_command "power_opt_design $power_opt_design__options"
  checkpoint
  create_reports
}


# Placing Design
set stage place
puts $tcl_script "\n# Placing Design"
run_command "place_design $place_design__options"

checkpoint

# Post-place reporting commands
create_reports


# Run Physical Optimization if requested
if {$phys_opt} {
  set stage phys_opt
  puts $tcl_script "\n# Run Physical Optimization"
  run_command "phys_opt_design $phys_opt_design__options"
  checkpoint
  create_reports
}

# Adding Post-place Constraints
read_constraints "$constraints_post_place"

# Sourcing any Post-place scripts
foreach script $script_post_place {
  run_command "source $script"
}

# Routing Design
set stage route
puts $tcl_script "\n# Routing Design"
run_command "route_design $route_design__options"

# Saving Run Post-route
puts $tcl_script "\n# Saving Run"
checkpoint
run_command "write_xdc -force ./${design_name}_route.xdc"
run_command "write_verilog -force ./${design_name}_route.v"

# dump debug probes
run_command "write_debug_probes ./${design_name}.ltx"

# Adding Post-route Constraints
read_constraints "$constraints_post_route"

# Sourcing any Post-route scripts
foreach script $script_post_route {
  run_command "source $script"
}

# Post-route reporting commands
create_reports

# Create bitstream if requested
if {$create_bitstream} {
  puts $tcl_script "\n# Create bitstream"
  run_command "write_bitstream -force $write_bitstream__options ${design_name}.bit"

  puts $tcl_script "\n# Create flash file"
  write_cfgmem -force -format mcs -interface spix4 -size 16 -loadbit "up 0x0 ${design_name}.bit" -file ${design_name}.mcs
}

puts "Vivado run completed successfully on [clock format [clock seconds]]."
puts "\tEntire run took [expr ([clock seconds]-$begin_time)/3600] hour(s), [expr (([clock seconds]-$begin_time)%3600)/60] minute(s) and [expr (([clock seconds]-$begin_time)%3600)%60] second(s)."
if {!$create_script_only} {
  archive_files
}

puts "The script run_vivado_2012.4.tcl completed successfully."
