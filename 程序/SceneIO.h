// ============================================================
// SceneIO.h —— .dgate 工程文件（JSON）的读写
// ============================================================
#pragma once

#include <wx/string.h>

#include <vector>

#include "DrawBoard.h"

// 从文件中读出的完整图纸内容
struct SceneFileData
{
    std::vector<Gate*> gates;
    std::vector<Wire> wires;
    std::vector<DrawBoard::BoardText> texts;

    int nextGateId = 1;
    int gridSize = 20;
    bool gridVisible = true;
    bool snapToGrid = true;
};

namespace SceneIO
{
    // 当前文件格式版本（写入时使用）
    const int kFormatVersion = 2;

    // 保存图纸；失败时 error 带原因
    bool Save(const wxString& path,
              const std::vector<Gate*>& gates,
              const std::vector<Wire>& wires,
              const std::vector<DrawBoard::BoardText>& texts,
              int gridSize,
              bool gridVisible,
              bool snapToGrid,
              wxString& error);

    // 读取图纸；失败时 data 中已创建的元件会被释放，error 带原因
    bool Load(const wxString& path, SceneFileData& data, wxString& error);

    // 按类型创建逻辑门（返还所有权）
    Gate* CreateGate(GateType type, const wxPoint& center);
}
