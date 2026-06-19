# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\Users\senur\Desktop\HDMI_Output_Test\Zybo-Z7-20-HDMI_system\_ide\scripts\systemdebugger_zybo-z7-20-hdmi_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\Users\senur\Desktop\HDMI_Output_Test\Zybo-Z7-20-HDMI_system\_ide\scripts\systemdebugger_zybo-z7-20-hdmi_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent Zybo Z7 210351B1062EA" && level==0 && jtag_device_ctx=="jsn-Zybo Z7-210351B1062EA-23727093-0"}
fpga -file C:/Users/senur/Desktop/HDMI_Output_Test/Zybo-Z7-20-HDMI/_ide/bitstream/hdmi_out_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/Users/senur/Desktop/HDMI_Output_Test/design_1_wrapper/export/design_1_wrapper/hw/hdmi_out_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source C:/Users/senur/Desktop/HDMI_Output_Test/Zybo-Z7-20-HDMI/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow C:/Users/senur/Desktop/HDMI_Output_Test/Zybo-Z7-20-HDMI/Debug/Zybo-Z7-20-HDMI.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
