#pragma once

#include <wx/wx.h>
#include <cstddef>
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
    XnorGate,
    InputSwitch,
    OutputIndicator
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

    // 用户自定义组合逻辑元件
    static const std::vector<CustomGateDefinition>& GetCustomComponents();
    static bool LoadCustomComponents(wxString& error);
    static bool SaveCustomComponents(wxString& error);
    static bool AddCustomComponent(
        const CustomGateDefinition& definition,
        wxString& error);
    static bool UpdateCustomComponent(
        std::size_t index,
        const CustomGateDefinition& definition,
        wxString& error);
    static bool DeleteCustomComponent(
        std::size_t index,
        wxString& error);
    static wxString GetCustomLibraryPath();
};
