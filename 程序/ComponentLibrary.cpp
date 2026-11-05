#include "ComponentLibrary.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <cstddef>
#include <string>

#include "Json.h"

namespace
{
    std::vector<CustomGateDefinition>& CustomComponents()
    {
        static std::vector<CustomGateDefinition> components;
        return components;
    }

    wxString FromUtf8(const std::string& text)
    {
        return wxString::FromUTF8(text.c_str());
    }

    std::string ToUtf8(const wxString& text)
    {
        return text.ToStdString(wxConvUTF8);
    }

    bool ValidateDefinition(
        const CustomGateDefinition& definition,
        std::size_t ignoredIndex,
        wxString& error)
    {
        wxString name = definition.name;
        name.Trim(true);
        name.Trim(false);

        if (name.empty())
        {
            error = wxT("元件名称不能为空。");
            return false;
        }

        if (name.length() > 48)
        {
            error = wxT("元件名称不能超过 48 个字符。");
            return false;
        }

        if (definition.inputCount < 1 || definition.inputCount > 4)
        {
            error = wxT("自定义元件暂时只支持 1 到 4 个输入。");
            return false;
        }

        const std::uint32_t validMask =
            (1U << (1U << definition.inputCount)) - 1U;

        if ((definition.truthTable & ~validMask) != 0)
        {
            error = wxT("真值表位数与输入数量不匹配。");
            return false;
        }

        const auto& components = CustomComponents();

        for (std::size_t i = 0; i < components.size(); ++i)
        {
            if (i == ignoredIndex)
            {
                continue;
            }

            if (components[i].name.CmpNoCase(name) == 0)
            {
                error = wxT("已经存在同名的自定义元件。");
                return false;
            }
        }

        return true;
    }

    bool ReadWholeFile(
        const wxString& path,
        std::string& content,
        wxString& error)
    {
        wxFile file(path, wxFile::read);

        if (!file.IsOpened())
        {
            error = wxString::Format(
                wxT("无法读取自定义元件库：%s"), path);
            return false;
        }

        const wxFileOffset length = file.Length();

        if (length <= 0)
        {
            error = wxT("自定义元件库文件为空。");
            return false;
        }

        content.resize(static_cast<std::size_t>(length));
        const ssize_t read = file.Read(&content[0], content.size());

        if (read != static_cast<ssize_t>(content.size()))
        {
            error = wxT("读取自定义元件库时发生错误。");
            return false;
        }

        return true;
    }
}

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
        { LibraryComponentType::XnorGate, "XNOR Gate" },
        { LibraryComponentType::InputSwitch, "输入开关 (INPUT)" },
        { LibraryComponentType::OutputIndicator, "输出指示器 (OUTPUT)" }
    };

    return components;
}

const std::vector<CustomGateDefinition>&
ComponentLibrary::GetCustomComponents()
{
    return CustomComponents();
}

wxString ComponentLibrary::GetCustomLibraryPath()
{
    wxFileName executable(wxStandardPaths::Get().GetExecutablePath());
    return executable.GetPathWithSep() + wxT("custom_components.json");
}

bool ComponentLibrary::LoadCustomComponents(wxString& error)
{
    error.clear();
    CustomComponents().clear();

    const wxString path = GetCustomLibraryPath();

    if (!wxFileExists(path))
    {
        return true;
    }

    std::string content;

    if (!ReadWholeFile(path, content, error))
    {
        return false;
    }

    Json root;
    std::string parseError;

    if (!Json::Parse(content, root, parseError))
    {
        error = wxT("自定义元件库 JSON 解析失败：") +
            FromUtf8(parseError);
        return false;
    }

    if (!root.IsObject() ||
        root["format"].AsString() != "dgate-components")
    {
        error = wxT("自定义元件库缺少 format: dgate-components 标记。");
        return false;
    }

    const Json& array = root["components"];

    if (!array.IsArray())
    {
        error = wxT("自定义元件库缺少 components 数组。");
        return false;
    }

    for (const Json& item : array.Elements())
    {
        if (!item.IsObject())
        {
            error = wxT("自定义元件库中存在无效条目。");
            CustomComponents().clear();
            return false;
        }

        CustomGateDefinition definition;
        definition.name = FromUtf8(item["name"].AsString());
        definition.inputCount = item["inputCount"].AsInt(0);
        definition.truthTable = static_cast<std::uint32_t>(
            item["truthTable"].AsInt(0));

        if (!ValidateDefinition(
                definition, CustomComponents().size(), error))
        {
            CustomComponents().clear();
            return false;
        }

        CustomComponents().push_back(definition);
    }

    return true;
}

bool ComponentLibrary::SaveCustomComponents(wxString& error)
{
    error.clear();

    Json root = Json::MakeObject();
    root.Set("format", Json("dgate-components"));
    root.Set("version", Json(1));

    Json array = Json::MakeArray();

    for (const CustomGateDefinition& definition : CustomComponents())
    {
        Json item = Json::MakeObject();
        item.Set("name", Json(ToUtf8(definition.name)));
        item.Set("inputCount", Json(definition.inputCount));
        item.Set("truthTable", Json(
            static_cast<int>(definition.truthTable)));
        array.Push(std::move(item));
    }

    root.Set("components", std::move(array));

    const std::string content = root.Dump(true) + "\n";
    const wxString path = GetCustomLibraryPath();
    wxFile file(path, wxFile::write);

    if (!file.IsOpened())
    {
        error = wxString::Format(
            wxT("无法写入自定义元件库：%s"), path);
        return false;
    }

    const ssize_t written = file.Write(content.data(), content.size());

    if (written != static_cast<ssize_t>(content.size()))
    {
        error = wxT("写入自定义元件库时发生错误。");
        return false;
    }

    return true;
}

bool ComponentLibrary::AddCustomComponent(
    const CustomGateDefinition& definition,
    wxString& error)
{
    error.clear();

    if (!ValidateDefinition(
            definition, CustomComponents().size(), error))
    {
        return false;
    }

    CustomComponents().push_back(definition);

    if (!SaveCustomComponents(error))
    {
        CustomComponents().pop_back();
        return false;
    }

    return true;
}

bool ComponentLibrary::UpdateCustomComponent(
    std::size_t index,
    const CustomGateDefinition& definition,
    wxString& error)
{
    error.clear();

    if (index >= CustomComponents().size())
    {
        error = wxT("要修改的自定义元件不存在。");
        return false;
    }

    if (!ValidateDefinition(definition, index, error))
    {
        return false;
    }

    const CustomGateDefinition previous = CustomComponents()[index];
    CustomComponents()[index] = definition;

    if (!SaveCustomComponents(error))
    {
        CustomComponents()[index] = previous;
        return false;
    }

    return true;
}

bool ComponentLibrary::DeleteCustomComponent(
    std::size_t index,
    wxString& error)
{
    error.clear();

    if (index >= CustomComponents().size())
    {
        error = wxT("要删除的自定义元件不存在。");
        return false;
    }

    const CustomGateDefinition removed = CustomComponents()[index];
    CustomComponents().erase(
        CustomComponents().begin() +
        static_cast<std::ptrdiff_t>(index));

    if (!SaveCustomComponents(error))
    {
        CustomComponents().insert(
            CustomComponents().begin() +
            static_cast<std::ptrdiff_t>(index),
            removed);
        return false;
    }

    return true;
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

    case LibraryComponentType::InputSwitch:
        return new InputSwitch(position);

    case LibraryComponentType::OutputIndicator:
        return new OutputIndicator(position);

    default:
        return nullptr;
    }
}
