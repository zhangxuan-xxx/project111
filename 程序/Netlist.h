// ============================================================
// Netlist.h —— 从画布提取连通性，导出 Verilog / BLIF 网表
// ============================================================
#pragma once

#include <wx/string.h>

#include <vector>

#include "compoent.h"

// 导出结果的统计信息，用于状态栏提示
struct NetlistSummary
{
    int instances = 0;
    int nets = 0;
    int inputs = 0;
    int outputs = 0;
    int warnings = 0;
};

namespace Netlist
{
    // 导出 Verilog 结构网表（IEEE 1364 门级原语）
    bool ExportVerilog(const wxString& path,
                       const std::vector<Gate*>& gates,
                       const std::vector<Wire>& wires,
                       const wxString& moduleName,
                       NetlistSummary& summary,
                       wxString& error);

    // 导出 Berkeley BLIF 网表（.names 真值表）
    bool ExportBlif(const wxString& path,
                    const std::vector<Gate*>& gates,
                    const std::vector<Wire>& wires,
                    const wxString& moduleName,
                    NetlistSummary& summary,
                    wxString& error);
}
