# ===============================
# 预定义时钟参数
# ===============================

# WISHBONE 总线时钟周期 (50MHz -> 20ns)
set wb_clk_period 200
# VGA 像素时钟周期 (25.175MHz -> 40ns)
set vga_clk_period 400

# I/O 延迟百分比
set wb_io_pct 0.25   ;# WISHBONE I/O 延迟占比 25%
set vga_io_pct 0.20  ;# VGA I/O 延迟占比 20%

# 计算 I/O 延迟
set wb_io_delay  [expr $wb_clk_period * $wb_io_pct]
set vga_io_delay [expr $vga_clk_period * $vga_io_pct]

# ===============================
# 预定义信号接口
# ===============================

# WISHBONE 时钟 & 复位
set wb_clk [get_ports wb_clk_i]
set wb_rst [get_ports wb_rst_i]

# WISHBONE Slave 输入 (来自 WISHBONE Master)
set wb_slave_inputs [get_ports {wbs_adr_i wbs_dat_i wbs_sel_i wbs_we_i wbs_stb_i wbs_cyc_i}]
# WISHBONE Slave 输出 (返回给 WISHBONE Master)
set wb_slave_outputs [get_ports {wbs_dat_o wbs_ack_o wbs_rty_o wbs_err_o}]

# WISHBONE Master 输出 (发送到 WISHBONE 从设备)
set wb_master_outputs [get_ports {wbm_adr_o wbm_dat_i wbm_cti_o wbm_bte_o wbm_sel_o wbm_we_o wbm_stb_o wbm_cyc_o}]
# WISHBONE Master 输入 (接收从设备的返回数据)
set wb_master_inputs [get_ports {wbm_ack_i wbm_err_i}]

# VGA 时钟
set vga_clk [get_ports clk_p_i]
set vga_clk_out [get_ports clk_p_o]

# VGA 信号 (同步 & 视频数据)
set vga_sync [get_ports {hsync_pad_o vsync_pad_o csync_pad_o blank_pad_o}]
set vga_data [get_ports {r_pad_o g_pad_o b_pad_o}]

# ===============================
# 创建时钟
# ===============================

# WISHBONE 时钟 (50MHz)
create_clock -name wb_clk -period $wb_clk_period $wb_clk

# VGA 像素时钟 (25.175MHz)
create_clock -name vga_clk -period $vga_clk_period $vga_clk

# ===============================
# 设置输入延迟
# ===============================

# WISHBONE 总线输入延迟 (按时钟周期的 wb_io_pct 计算)
set_input_delay $wb_io_delay -clock wb_clk $wb_slave_inputs
set_input_delay $wb_io_delay -clock wb_clk $wb_master_inputs

# VGA 输入时钟延迟 (按时钟周期的 vga_io_pct 计算)
set_input_delay $vga_io_delay -clock vga_clk $vga_clk

# ===============================
# 设置输出延迟
# ===============================

# WISHBONE 总线输出延迟 (按时钟周期的 wb_io_pct 计算)
set_output_delay $wb_io_delay -clock wb_clk $wb_slave_outputs
set_output_delay $wb_io_delay -clock wb_clk $wb_master_outputs

# VGA 输出延迟 (按时钟周期的 vga_io_pct 计算)
set_output_delay $vga_io_delay -clock vga_clk $vga_sync
set_output_delay $vga_io_delay -clock vga_clk $vga_data

# ===============================
# 设定跨时钟域路径
# ===============================

# WISHBONE ↔ VGA 跨时钟域，防止时序误判
set_false_path -from [get_clocks wb_clk] -to [get_clocks vga_clk]
set_false_path -from [get_clocks vga_clk] -to [get_clocks wb_clk]

# ===============================
# 设定多周期路径 (如需要)
# ===============================

# VGA 控制信号可能更新较慢，假设 2 个时钟周期
# set_multicycle_path 2 -setup -from $vga_clk_out -to $vga_sync
# set_multicycle_path 1 -hold  -from $vga_clk_out -to $vga_sync
# set_multicycle_path 2 -setup -from $vga_clk_out -to $vga_data
# set_multicycle_path 1 -hold  -from $vga_clk_out -to $vga_data
