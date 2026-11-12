#include "SceneIO.h"

#include <wx/file.h>

#include <cctype>
#include <cmath>
#include <set>
#include <string>

#include "Json.h"

namespace
{
    // -------- GateType <-> 字符串 --------
    const char* GateTypeToString(GateType type)
    {
        switch (type)
        {
        case AND:       return "AND";
        case OR:        return "OR";
        case NOT:       return "NOT";
        case NAND_GATE: return "NAND";
        case XOR_GATE:  return "XOR";
        case NOR_GATE:  return "NOR";
        case XNOR_GATE: return "XNOR";
        case INPUT_SWITCH: return "INPUT";
        case OUTPUT_INDICATOR: return "OUTPUT";
        case CUSTOM_GATE: return "CUSTOM";
        default:        return "AND";
        }
    }

    bool GateTypeFromString(const std::string& raw, GateType& type)
    {
        std::string text;

        for (char ch : raw)
        {
            if (ch == ' ' || ch == '-' || ch == '_')
            {
                continue;
            }

            text.push_back(static_cast<char>(
                std::toupper(static_cast<unsigned char>(ch))));
        }

        if (text == "AND")       { type = AND;       return true; }
        if (text == "OR")        { type = OR;        return true; }
        if (text == "NOT" || text == "INV" || text == "INVERTER")
        {
            type = NOT;
            return true;
        }
        if (text == "NAND" || text == "NANDGATE") { type = NAND_GATE; return true; }
        if (text == "XOR")                         { type = XOR_GATE;  return true; }
        if (text == "NOR" || text == "NORGATE")   { type = NOR_GATE;  return true; }
        if (text == "XNOR" || text == "XNORGATE") { type = XNOR_GATE; return true; }
        if (text == "INPUT" || text == "INPUTSWITCH") { type = INPUT_SWITCH; return true; }
        if (text == "OUTPUT" || text == "OUTPUTINDICATOR") { type = OUTPUT_INDICATOR; return true; }
        if (text == "CUSTOM" || text == "CUSTOMGATE") { type = CUSTOM_GATE; return true; }

        return false;
    }

    wxString ToWx(const std::string& text)
    {
        return wxString::FromUTF8(text.c_str());
    }

    std::string FromWx(const wxString& text)
    {
        return text.ToStdString(wxConvUTF8);
    }

    bool ReadWholeFile(const wxString& path, std::string& content,
                       wxString& error)
    {
        wxFile file(path, wxFile::read);

        if (!file.IsOpened())
        {
            error = wxString::Format(
                wxT("无法打开文件：%s"), path);
            return false;
        }

        const wxFileOffset length = file.Length();

        if (length < 0)
        {
            error = wxT("无法读取文件大小");
            return false;
        }

        if (length == 0)
        {
            error = wxT("文件是空的");
            return false;
        }

        content.assign(static_cast<std::size_t>(length), '\0');

        const ssize_t read = file.Read(
            content.data(), static_cast<std::size_t>(length));

        if (read < 0)
        {
            error = wxT("读取文件失败");
            return false;
        }

        content.resize(static_cast<std::size_t>(read));
        return true;
    }

    bool RequireNumber(const Json& object, const char* key,
                       double& value, wxString& error)
    {
        const Json& field = object[key];

        if (!field.IsNumber())
        {
            error = wxString::Format(
                wxT("字段 \"%s\" 缺失或不是数字"), key);
            return false;
        }

        value = field.AsDouble();
        return true;
    }
}

// ============================================================
// 创建逻辑门
// ============================================================
Gate* SceneIO::CreateGate(GateType type, const wxPoint& center)
{
    switch (type)
    {
    case AND:       return new ANDGate(center);
    case OR:        return new ORGate(center);
    case NOT:       return new NOTGate(center);
    case NAND_GATE: return new NANDGate(center);
    case XOR_GATE:  return new XORGate(center);
    case NOR_GATE:  return new NORGate(center);
    case XNOR_GATE: return new XNORGate(center);
    case INPUT_SWITCH: return new InputSwitch(center);
    case OUTPUT_INDICATOR: return new OutputIndicator(center);
    case CUSTOM_GATE:
    {
        CustomGateDefinition definition;
        definition.name = wxT("自定义元件");
        definition.inputCount = 2;
        definition.truthTable = 0;
        return new CustomGate(center, definition);
    }
    default:
        return new ANDGate(center);
    }
}

// ============================================================
// 保存
// ============================================================
bool SceneIO::Save(const wxString& path,
                   const std::vector<Gate*>& gates,
                   const std::vector<Wire>& wires,
                   const std::vector<DrawBoard::BoardText>& texts,
                   int gridSize,
                   bool gridVisible,
                   bool snapToGrid,
                   wxString& error)
{
    Json root = Json::MakeObject();

    root.Set("format", Json("dgate"));
    root.Set("version", Json(kFormatVersion));
    root.Set("app", Json("DrawGate"));

    Json canvas = Json::MakeObject();
    canvas.Set("gridSize", Json(gridSize));
    canvas.Set("gridVisible", Json(gridVisible));
    canvas.Set("snapToGrid", Json(snapToGrid));
    root.Set("canvas", std::move(canvas));

    Json gateArray = Json::MakeArray();

    for (const Gate* gate : gates)
    {
        if (gate == nullptr)
        {
            continue;
        }

        Json item = Json::MakeObject();
        item.Set("id", Json(gate->id));
        item.Set("type", Json(GateTypeToString(gate->type)));
        item.Set("x", Json(gate->center.x));
        item.Set("y", Json(gate->center.y));
        item.Set("label", Json(FromWx(gate->label)));
        item.Set("scale", Json(gate->scale));
        item.Set("logicValue", Json(gate->logicValue));
        if (const CustomGate* custom =
                dynamic_cast<const CustomGate*>(gate))
        {
            const CustomGateDefinition& definition =
                custom->GetDefinition();
            Json customObject = Json::MakeObject();
            customObject.Set("name", Json(FromWx(definition.name)));
            customObject.Set("inputCount", Json(definition.inputCount));
            customObject.Set("truthTable", Json(
                static_cast<int>(definition.truthTable)));
            item.Set("custom", std::move(customObject));
        }

        gateArray.Push(std::move(item));
    }

    root.Set("gates", std::move(gateArray));

    Json wireArray = Json::MakeArray();

    for (const Wire& wire : wires)
    {
        Json item = Json::MakeObject();
        item.Set("x1", Json(wire.start.x));
        item.Set("y1", Json(wire.start.y));
        item.Set("x2", Json(wire.end.x));
        item.Set("y2", Json(wire.end.y));
        item.Set("fromGate", Json(wire.startComponentId));
        item.Set("toGate", Json(wire.endComponentId));
        item.Set("fromPort", Json(wire.startPortIndex));
        item.Set("toPort", Json(wire.endPortIndex));

        wireArray.Push(std::move(item));
    }

    root.Set("wires", std::move(wireArray));

    Json textArray = Json::MakeArray();

    for (const DrawBoard::BoardText& item : texts)
    {
        Json object = Json::MakeObject();
        object.Set("x", Json(item.pos.x));
        object.Set("y", Json(item.pos.y));
        object.Set("text", Json(FromWx(item.text)));

        textArray.Push(std::move(object));
    }

    root.Set("texts", std::move(textArray));

    const std::string content = root.Dump(true) + "\n";

    wxFile file(path, wxFile::write);

    if (!file.IsOpened())
    {
        error = wxString::Format(
            wxT("无法写入文件：%s"), path);
        return false;
    }

    const size_t written = file.Write(content.data(), content.size());

    if (written != content.size())
    {
        error = wxT("写入文件时出错（磁盘已满或没有权限）");
        return false;
    }

    return true;
}

// ============================================================
// 读取
// ============================================================
bool SceneIO::Load(const wxString& path, SceneFileData& data,
                   wxString& error)
{
    std::string content;

    if (!ReadWholeFile(path, content, error))
    {
        return false;
    }

    Json root;
    std::string parseError;

    if (!Json::Parse(content, root, parseError))
    {
        error = wxT("JSON 解析失败：") + ToWx(parseError);
        return false;
    }

    if (!root.IsObject() || root["format"].AsString() != "dgate")
    {
        error = wxT("这不是 DrawGate 图纸文件（缺少 format: dgate 标记）");
        return false;
    }

    if (!root["version"].IsNumber())
    {
        error = wxT("文件缺少 version 字段");
        return false;
    }

    const int version = root["version"].AsInt(0);

    if (version <= 0)
    {
        error = wxT("文件版本号非法");
        return false;
    }

    if (version > kFormatVersion)
    {
        error = wxString::Format(
            wxT("文件版本 %d 高于当前程序支持的 %d，请升级 DrawGate"),
            version, kFormatVersion);
        return false;
    }

    SceneFileData result;

    // 读取失败时要释放已经创建的元件
    auto cleanup = [&result]()
    {
        for (Gate* gate : result.gates)
        {
            delete gate;
        }

        result.gates.clear();
    };

    // -------- 画布设置 --------
    const Json& canvas = root["canvas"];

    if (canvas.IsObject())
    {
        result.gridSize = canvas["gridSize"].AsInt(20);
        result.gridVisible = canvas["gridVisible"].AsBool(true);
        result.snapToGrid = canvas["snapToGrid"].AsBool(true);
    }

    result.gridSize = wxMin(wxMax(result.gridSize, 2), 200);

    // -------- 元件 --------
    const Json& gateArray = root["gates"];

    if (!gateArray.IsArray())
    {
        error = wxT("文件缺少 gates 数组");
        return false;
    }

    std::set<int> usedIds;

    for (const Json& item : gateArray.Elements())
    {
        if (!item.IsObject())
        {
            error = wxT("gates 中存在非对象元素");
            cleanup();
            return false;
        }

        const std::string typeText = item["type"].AsString();

        GateType type = AND;

        if (!GateTypeFromString(typeText, type))
        {
            error = wxString::Format(
                wxT("未知的元件类型：%s"), ToWx(typeText));
            cleanup();
            return false;
        }

        double x = 0.0;
        double y = 0.0;

        if (!RequireNumber(item, "x", x, error) ||
            !RequireNumber(item, "y", y, error))
        {
            cleanup();
            return false;
        }

        const int id = item["id"].AsInt(0);

        if (id <= 0)
        {
            error = wxT("元件缺少合法的 id");
            cleanup();
            return false;
        }

        if (usedIds.count(id) != 0)
        {
            error = wxString::Format(
                wxT("元件 id 重复：%d"), id);
            cleanup();
            return false;
        }

        usedIds.insert(id);

        const wxPoint center(
            static_cast<int>(std::lround(x)),
            static_cast<int>(std::lround(y)));

        Gate* gate = nullptr;

        if (type == CUSTOM_GATE)
        {
            const Json& customObject = item["custom"];

            if (!customObject.IsObject())
            {
                error = wxT("自定义元件缺少 custom 定义。");
                cleanup();
                return false;
            }

            CustomGateDefinition definition;
            definition.name =
                ToWx(customObject["name"].AsString());
            definition.inputCount =
                customObject["inputCount"].AsInt(0);
            definition.truthTable =
                static_cast<std::uint32_t>(
                    customObject["truthTable"].AsInt(0));

            definition.name.Trim(true);
            definition.name.Trim(false);

            const std::uint32_t validMask =
                definition.inputCount >= 1 &&
                definition.inputCount <= 4
                    ? (1U << (1U << definition.inputCount)) - 1U
                    : 0U;

            if (definition.name.empty() ||
                definition.inputCount < 1 ||
                definition.inputCount > 4 ||
                (definition.truthTable & ~validMask) != 0)
            {
                error = wxString::Format(
                    wxT("自定义元件的名称、输入数量或真值表无效。")
                    wxT("（name=%s, inputs=%d, truth=0x%X, mask=0x%X）"),
                    definition.name,
                    definition.inputCount,
                    static_cast<unsigned int>(definition.truthTable),
                    static_cast<unsigned int>(validMask));
                cleanup();
                return false;
            }

            gate = new CustomGate(center, definition);
        }
        else
        {
            gate = CreateGate(type, center);
        }

        gate->id = id;
        gate->label = ToWx(item["label"].AsString());

        const double scale = item["scale"].AsDouble(1.0);
        gate->scale = wxMin(wxMax(scale, 0.2), 5.0);
        gate->logicValue = item["logicValue"].AsBool(false);
        gate->logicValid = gate->type == INPUT_SWITCH;

        result.gates.push_back(gate);
    }

    // -------- 连线 --------
    const Json& wireArray = root["wires"];

    if (wireArray.IsArray())
    {
        for (const Json& item : wireArray.Elements())
        {
            if (!item.IsObject())
            {
                error = wxT("wires 中存在非对象元素");
                cleanup();
                return false;
            }

            Wire wire;

            wire.start.x = item["x1"].AsInt(0);
            wire.start.y = item["y1"].AsInt(0);
            wire.end.x = item["x2"].AsInt(0);
            wire.end.y = item["y2"].AsInt(0);

            wire.startComponentId = item["fromGate"].AsInt(-1);
            wire.endComponentId = item["toGate"].AsInt(-1);
            wire.startPortIndex = item["fromPort"].AsInt(-1);
            wire.endPortIndex = item["toPort"].AsInt(-1);
            wire.isActive = false;

            if (wire.startComponentId >= 0 &&
                usedIds.count(wire.startComponentId) == 0)
            {
                error = wxString::Format(
                    wxT("连线引用了不存在的元件 #%d"),
                    wire.startComponentId);
                cleanup();
                return false;
            }

            if (wire.endComponentId >= 0 &&
                usedIds.count(wire.endComponentId) == 0)
            {
                error = wxString::Format(
                    wxT("连线引用了不存在的元件 #%d"),
                    wire.endComponentId);
                cleanup();
                return false;
            }

            result.wires.push_back(wire);
        }
    }

    // -------- 文字 --------
    const Json& textArray = root["texts"];

    if (textArray.IsArray())
    {
        for (const Json& item : textArray.Elements())
        {
            if (!item.IsObject())
            {
                continue;
            }

            DrawBoard::BoardText text;
            text.pos.x = item["x"].AsInt(0);
            text.pos.y = item["y"].AsInt(0);
            text.text = ToWx(item["text"].AsString());

            result.texts.push_back(text);
        }
    }

    int maxId = 0;

    for (int id : usedIds)
    {
        maxId = wxMax(maxId, id);
    }

    result.nextGateId = maxId + 1;

    data = std::move(result);
    return true;
}
