#pragma once

#include <wx/wx.h>
#include <vector>
#include "compoent.h"

// ============================================================
// 元件库类型
// ============================================================
enum class LibraryComponentType
{
    AndGate,
    OrGate,
    NotGate,
    NandGate,
    XorGate,
    NorGate,
    XnorGate
};

// ============================================================
// 元件信息
// ============================================================
struct ComponentInfo
{
    LibraryComponentType type;
    wxString name;
};

// ============================================================
// 元件库
// ============================================================
class ComponentLibrary
{
public:
    // 获取元件列表
    static const std::vector<ComponentInfo>& GetComponents();

    // 根据元件类型创建逻辑门
    static Gate* CreateGate(
        LibraryComponentType type,
        const wxPoint& position
    );
};
