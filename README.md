# DrawGate

DrawGate 是一个 Windows 桌面端组合逻辑电路编辑与仿真工具，使用 C++20 和 wxWidgets 开发。

## 主要功能

- 绘制 7 类组合逻辑门、输入开关、输出指示灯和自定义逻辑元件
- 端口连线、网格吸附、文字标注和画布导出 PNG
- 组合逻辑状态求值与异常拓扑检查
- 工程文件 `.dgate` 的保存、读取和版本校验
- 逻辑网表导出：Verilog、BLIF
- PCB 设计网表导出：KiCad S-expression `.net`

## KiCad PCB 网表

在软件中选择 `文件 -> 导出网表 -> KiCad PCB 网表 (*.net)...`。导出后在 KiCad PCB
编辑器中选择 `文件 -> 导入 -> 网表`，即可进入封装分配和 PCB 布线流程。

导出器会把逻辑元件映射为可采购的 74HC 系列器件，并自动加入 VCC/GND 网络。具体映射、
使用限制和后续扩展方式见 [docs/pcb-netlist.md](docs/pcb-netlist.md)。
