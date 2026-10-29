#include "ComponentLibrary.h"

// ============================================================
// 获取元件库列表
// ============================================================
const std::vector<ComponentInfo>& ComponentLibrary::GetComponents()
{
    static const std::vector<ComponentInfo> components =
    {
        { LibraryComponentType::AndGate,  "AND Gate" },
        { LibraryComponentType::OrGate,   "OR Gate" },
        { LibraryComponentType::NotGate,  "NOT Gate" },
        { LibraryComponentType::NandGate, "NAND Gate" },
        { LibraryComponentType::XorGate,  "XOR Gate" },
        { LibraryComponentType::NorGate,  "NOR Gate" },
        { LibraryComponentType::XnorGate, "XNOR Gate" }
    };

    return components;
}

// ============================================================
// 根据类型创建逻辑门
// ============================================================
Gate* ComponentLibrary::CreateGate(
    LibraryComponentType type,
    const wxPoint& position
)
{
    switch (type)
    {
    case LibraryComponentType::AndGate:
        return new ANDGate(position);

    case LibraryComponentType::OrGate:
        return new ORGate(position);

    case LibraryComponentType::NotGate:
        return new NOTGate(position);

    case LibraryComponentType::NandGate:
        return new NANDGate(position);

    case LibraryComponentType::XorGate:
        return new XORGate(position);

    case LibraryComponentType::NorGate:
        return new NORGate(position);

    case LibraryComponentType::XnorGate:
        return new XNORGate(position);

    default:
        return nullptr;
    }
}
