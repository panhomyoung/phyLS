set clk_name  wb_clk_i
set clk_port  [get_ports wb_clk_i]
set clk_period 480
set clk_io_pct 0.2
create_clock -name $clk_name -period $clk_period $clk_port

set pci_clk_name pci_clk_i
set pci_clk_port [get_ports pci_clk_i]
set pci_clk_period 800
create_clock -name $pci_clk_name -period $pci_clk_period $pci_clk_port

# 获取所有输入端口
set all_inputs [all_inputs]

# 1. 由 clk_i 采样的输入端口
set clk_input_ports [lsearch -inline -all -not -exact $all_inputs $clk_port]
set clk_input_ports [lsearch -inline -all -not -exact $clk_input_ports $pci_clk_port]

# 2. 由 bit_clk_pad_i 采样的输入端口（假设 pci_data_i 由比特时钟采样）
set pci_input_ports [get_ports {pci_data_i}]

# # 只对每组输入信号设置一次 input delay
# set_input_delay  [expr $clk_period * $clk_io_pct] -clock $clk_name $clk_input_ports
# set_input_delay  [expr $pci_clk_period * $clk_io_pct] -clock $pci_clk_name $pci_input_ports

# # 设置输出延迟（通常由 clk_i 控制）
# set_output_delay [expr $clk_period * $clk_io_pct] -clock $clk_name [all_outputs]
# set_output_delay [expr $pci_clk_period * $clk_io_pct] -clock $clk_name [all_outputs]

# # 设置时钟组，避免交叉时钟域问题
# set_clock_groups -name core_clock -logically_exclusive \
#  -group [get_clocks $clk_name] \
#  -group [get_clocks $pci_clk_name]
