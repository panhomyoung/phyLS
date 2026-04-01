#########################################
# 1. 创建时钟
#########################################

# Wishbone 总线主时钟 (假设 50MHz)
create_clock -name wb_clk -period 20 [get_ports wb_clk_i]

# MII/RMII 发送时钟 (假设 2.5MHz, 可调整)
create_clock -name mtx_clk -period 400 [get_ports mtx_clk_pad_i]

# MII/RMII 接收时钟 (假设 2.5MHz, 可调整)
create_clock -name mrx_clk -period 400 [get_ports mrx_clk_pad_i]

set clk_io_pct 0.2

#########################################
# 2. 设定输入/输出延迟 (假设 clk_io_pct = 0.2)
#########################################

# Wishbone 输入信号 (20% 延迟)
set_input_delay [expr 20 * $clk_io_pct] -clock wb_clk [get_ports {wb_dat_i wb_adr_i wb_sel_i wb_we_i wb_cyc_i wb_stb_i}]
set_output_delay [expr 20 * $clk_io_pct] -clock wb_clk [get_ports {wb_dat_o wb_ack_o wb_err_o}]

# Wishbone master 端口
set_input_delay [expr 20 * $clk_io_pct] -clock wb_clk [get_ports {m_wb_dat_i m_wb_ack_i m_wb_err_i}]
set_output_delay [expr 20 * $clk_io_pct] -clock wb_clk [get_ports {m_wb_dat_o m_wb_adr_o m_wb_sel_o m_wb_we_o m_wb_cyc_o m_wb_stb_o}]

# MII 发送 (TX) 端口
set_output_delay [expr 400 * $clk_io_pct] -clock mtx_clk [get_ports {mtxd_pad_o mtxen_pad_o mtxerr_pad_o}]

# MII 接收 (RX) 端口
set_input_delay [expr 400 * $clk_io_pct] -clock mrx_clk [get_ports {mrxd_pad_i mrxdv_pad_i mrxerr_pad_i mcoll_pad_i mcrs_pad_i}]

# MIIM 端口 (假设 2.5MHz, mtx_clkns)
set_output_delay [expr 400 * $clk_io_pct] -clock wb_clk [get_ports {mdc_pad_o md_pad_o md_padoe_o}]
set_input_delay  [expr 400 * $clk_io_pct] -clock wb_clk [get_ports {md_pad_i}]