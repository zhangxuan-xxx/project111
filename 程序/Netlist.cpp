#include "Netlist.h"

#include <wx/datetime.h>
#include <wx/file.h>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
    // -------- 网表内部结构 --------
    struct Node
    {
        int x = 0;
        int y = 0;
    };

    struct PortRef
    {
        int gate = -1;
        int port = -1;
        bool output = false;
    };

    struct Net
    {
        std::string name;
        bool primaryInput = false;
        bool primaryOutput = false;
        int driverGate = -1;
        std::vector<std::pair<int, int>> sinks; // (gate, port)
        int minX = INT_MAX;
        int minY = INT_MAX;
    };

    std::string FromWx(const wxString& text)
    {
        return text.ToStdString(wxConvUTF8);
    }

    wxString ToWx(const std::string& text)
    {
        return wxString::FromUTF8(text.c_str());
    }

    bool WriteTextFile(const wxString& path, const std::string& content,
                       wxString& error)
    {
        wxFile file(path, wxFile::write);

        if (!file.IsOpened())
        {
            error = wxString::Format(wxT("无法写入文件：%s"), path);
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

    // 把标号转成合法的 Verilog / BLIF 标识符
    std::string SanitizeIdentifier(const wxString& raw,
                                   const std::string& fallback)
    {
        const std::string source = FromWx(raw);
        std::string result;
        bool hasAlphaNumeric = false;

        for (unsigned char ch : source)
        {
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9'))
            {
                result.push_back(static_cast<char>(ch));
                hasAlphaNumeric = true;
            }
            else if (ch == '_' || ch == '$')
            {
                result.push_back(static_cast<char>(ch));
            }
            else
            {
                // 中文等非 ASCII 字符不能出现在 Verilog 标识符里
                result.push_back('_');
            }
        }

        // 全是下划线（例如中文标号）时退回默认名字
        if (!hasAlphaNumeric)
        {
            return fallback;
        }

        if (result[0] >= '0' && result[0] <= '9')
        {
            result.insert(result.begin(), '_');
        }

        return result;
    }

    const char* PrimitiveName(GateType type)
    {
        switch (type)
        {
        case AND:       return "and";
        case OR:        return "or";
        case NOT:       return "not";
        case NAND_GATE: return "nand";
        case XOR_GATE:  return "xor";
        case NOR_GATE:  return "nor";
        case XNOR_GATE: return "xnor";
        default:        return "and";
        }
    }

    // ========================================================
    // 连通性构建：把「线段 + 端口」合并成电气网络
    // ========================================================
    class Builder
    {
    public:
        Builder(const std::vector<Gate*>& gates,
                const std::vector<Wire>& wires)
            : m_sourceGates(gates), m_sourceWires(wires)
        {
        }

        bool Build(wxString& error)
        {
            indexGates();

            if (m_gates.empty())
            {
                error = wxT("画布上没有可导出的元件");
                return false;
            }

            addPortNodes();
            addWireNodes();
            connectTouchingPoints();
            collectNets();

            if (m_nets.empty())
            {
                error = wxT("没有识别到任何网络");
                return false;
            }

            assignNames();
            return true;
        }

        std::size_t GateCount() const { return m_gates.size(); }
        const Gate& GateAt(std::size_t index) const { return *m_gates[index]; }
        const std::string& RefAt(std::size_t index) const { return m_refs[index]; }
        const std::vector<int>& InputNets(std::size_t index) const
        {
            return m_inNets[index];
        }
        int OutputNet(std::size_t index) const { return m_outNet[index]; }

        const std::vector<Net>& Nets() const { return m_nets; }
        int WarningCount() const { return m_warnings; }
        const std::vector<std::string>& WarningsText() const
        {
            return m_warningText;
        }

    private:
        void indexGates()
        {
            std::set<std::string> usedRefs;

            for (Gate* gate : m_sourceGates)
            {
                if (gate == nullptr)
                {
                    continue;
                }

                const std::string fallback =
                    "U" + std::to_string(gate->id);

                std::string ref = SanitizeIdentifier(gate->label, fallback);

                if (usedRefs.count(ref) != 0)
                {
                    const std::string base = ref;
                    int suffix = 2;

                    while (usedRefs.count(ref) != 0)
                    {
                        ref = base + "_" + std::to_string(suffix++);
                    }
                }

                usedRefs.insert(ref);
                m_refs.push_back(ref);
                m_gates.push_back(gate);
            }
        }

        int addNode(int x, int y)
        {
            const std::pair<int, int> key(x, y);
            const auto found = m_coords.find(key);

            if (found != m_coords.end())
            {
                return found->second;
            }

            const int id = static_cast<int>(m_nodes.size());
            Node node;
            node.x = x;
            node.y = y;

            m_nodes.push_back(node);
            m_parent.push_back(id);
            m_coords[key] = id;
            return id;
        }

        int find(int node)
        {
            while (m_parent[node] != node)
            {
                m_parent[node] = m_parent[m_parent[node]];
                node = m_parent[node];
            }

            return node;
        }

        void unite(int first, int second)
        {
            first = find(first);
            second = find(second);

            if (first != second)
            {
                m_parent[second] = first;
            }
        }

        void addPortNodes()
        {
            m_outNode.resize(m_gates.size(), -1);
            m_inNodes.resize(m_gates.size());

            for (std::size_t i = 0; i < m_gates.size(); ++i)
            {
                const Gate* gate = m_gates[i];

                const wxPoint out = gate->GetOutputPort();
                m_outNode[i] = addNode(out.x, out.y);

                const int inputs = gate->GetInputCount();
                m_inNodes[i].resize(static_cast<std::size_t>(inputs), -1);

                for (int k = 0; k < inputs; ++k)
                {
                    const wxPoint port = gate->GetInputPort(k);
                    m_inNodes[i][k] = addNode(port.x, port.y);
                }
            }
        }

        void addWireNodes()
        {
            for (const Wire& wire : m_sourceWires)
            {
                const int start = addNode(wire.start.x, wire.start.y);
                const int end = addNode(wire.end.x, wire.end.y);

                unite(start, end);
                m_wireNodes.emplace_back(start, end);
            }
        }

        // 线段的某一点落在另一条线上时（T 型连接），视为同一网络。
        // 与 KiCad 的行为一致：线头搭在另一条线上会自动形成连接点。
        static bool OnSegment(const Node& point, const wxPoint& first,
                              const wxPoint& second)
        {
            const int tolerance = 1;

            if (first.y == second.y)
            {
                return std::abs(point.y - first.y) <= tolerance &&
                    point.x >= std::min(first.x, second.x) - tolerance &&
                    point.x <= std::max(first.x, second.x) + tolerance;
            }

            if (first.x == second.x)
            {
                return std::abs(point.x - first.x) <= tolerance &&
                    point.y >= std::min(first.y, second.y) - tolerance &&
                    point.y <= std::max(first.y, second.y) + tolerance;
            }

            return false;
        }

        void connectTouchingPoints()
        {
            for (std::size_t index = 0; index < m_nodes.size(); ++index)
            {
                const Node& point = m_nodes[index];

                for (const auto& wire : m_wireNodes)
                {
                    if (static_cast<int>(index) == wire.first ||
                        static_cast<int>(index) == wire.second)
                    {
                        continue;
                    }

                    const Node& a = m_nodes[wire.first];
                    const Node& b = m_nodes[wire.second];

                    const wxPoint first(a.x, a.y);
                    const wxPoint last(b.x, b.y);

                    const wxPoint midX(
                        (first.x + last.x) / 2, first.y);
                    const wxPoint midY(
                        (first.x + last.x) / 2, last.y);

                    const wxPoint points[4] = { first, midX, midY, last };

                    for (int segment = 0; segment < 3; ++segment)
                    {
                        if (OnSegment(point, points[segment],
                                      points[segment + 1]))
                        {
                            unite(static_cast<int>(index), wire.first);
                            break;
                        }
                    }
                }
            }
        }

        void collectNets()
        {
            // 先按并查集根汇总节点范围
            std::map<int, Net> nets;

            for (std::size_t i = 0; i < m_nodes.size(); ++i)
            {
                Net& net = nets[find(static_cast<int>(i))];
                net.minX = std::min(net.minX, m_nodes[i].x);
                net.minY = std::min(net.minY, m_nodes[i].y);
            }

            std::map<int, int> driverCount;

            auto addPort = [&nets, &driverCount](int node, int gate, int port,
                                                 bool output)
            {
                const int root = node;
                Net& net = nets[root];

                if (output)
                {
                    ++driverCount[root];

                    if (net.driverGate < 0)
                    {
                        net.driverGate = gate;
                    }
                }
                else
                {
                    net.sinks.emplace_back(gate, port);
                }
            };

            // 端口节点先求根再挂到对应网络
            for (std::size_t i = 0; i < m_gates.size(); ++i)
            {
                const int outRoot = find(m_outNode[i]);
                addPort(outRoot, static_cast<int>(i), -1, true);

                for (std::size_t k = 0; k < m_inNodes[i].size(); ++k)
                {
                    const int inRoot = find(m_inNodes[i][k]);
                    addPort(inRoot, static_cast<int>(i), static_cast<int>(k),
                            false);
                }
            }

            // 过滤掉没有任何端口的自由连线
            for (auto it = nets.begin(); it != nets.end();)
            {
                const int root = it->first;

                if (driverCount[root] == 0 && it->second.sinks.empty())
                {
                    ++m_freeWireCount;
                    it = nets.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            for (const auto& pair : nets)
            {
                const int root = pair.first;
                const Net& net = pair.second;

                if (driverCount[root] > 1)
                {
                    ++m_warnings;
                    m_warningText.push_back(
                        "网络上有 " + std::to_string(driverCount[root]) +
                        " 个输出端直接相连，网表中将出现多驱动");
                }

                Net copy = net;
                copy.primaryInput = (driverCount[root] == 0);
                copy.primaryOutput = (driverCount[root] > 0 &&
                                      copy.sinks.empty());

                m_rootToIndex[root] = static_cast<int>(m_nets.size());
                m_nets.push_back(std::move(copy));
            }

            if (m_freeWireCount > 0)
            {
                ++m_warnings;
                m_warningText.push_back(
                    "有 " + std::to_string(m_freeWireCount) +
                    " 条自由连线没有接到任何端口，已忽略");
            }

            // 每个门的输入 / 输出网络
            m_outNet.resize(m_gates.size(), -1);
            m_inNets.resize(m_gates.size());

            for (std::size_t i = 0; i < m_gates.size(); ++i)
            {
                m_outNet[i] = m_rootToIndex[find(m_outNode[i])];

                m_inNets[i].resize(m_inNodes[i].size(), -1);

                for (std::size_t k = 0; k < m_inNodes[i].size(); ++k)
                {
                    m_inNets[i][k] = m_rootToIndex[find(m_inNodes[i][k])];
                }
            }
        }

        void assignNames()
        {
            std::vector<int> inputs;
            std::vector<int> outputs;
            std::vector<int> internals;

            for (std::size_t i = 0; i < m_nets.size(); ++i)
            {
                if (m_nets[i].primaryInput)
                {
                    inputs.push_back(static_cast<int>(i));
                }
                else if (m_nets[i].primaryOutput)
                {
                    outputs.push_back(static_cast<int>(i));
                }
                else
                {
                    internals.push_back(static_cast<int>(i));
                }
            }

            // 按从上到下、从左到右排序，导出的端口顺序更直观
            auto byPosition = [this](int first, int second)
            {
                const Net& a = m_nets[first];
                const Net& b = m_nets[second];

                if (a.minY != b.minY)
                {
                    return a.minY < b.minY;
                }

                if (a.minX != b.minX)
                {
                    return a.minX < b.minX;
                }

                return first < second;
            };

            std::sort(inputs.begin(), inputs.end(), byPosition);
            std::sort(outputs.begin(), outputs.end(), byPosition);
            std::sort(internals.begin(), internals.end(), byPosition);

            int index = 1;

            for (int net : inputs)
            {
                m_nets[net].name = "in" + std::to_string(index++);
            }

            index = 1;

            for (int net : outputs)
            {
                m_nets[net].name = "out" + std::to_string(index++);
            }

            index = 1;

            for (int net : internals)
            {
                m_nets[net].name = "n" + std::to_string(index++);
            }
        }

        const std::vector<Gate*>& m_sourceGates;
        const std::vector<Wire>& m_sourceWires;

        std::vector<const Gate*> m_gates;
        std::vector<std::string> m_refs;
        std::vector<std::vector<int>> m_inNets;
        std::vector<int> m_outNet;

        std::vector<Node> m_nodes;
        std::vector<int> m_parent;
        std::map<std::pair<int, int>, int> m_coords;
        std::vector<std::pair<int, int>> m_wireNodes;

        std::vector<int> m_outNode;
        std::vector<std::vector<int>> m_inNodes;

        std::vector<Net> m_nets;
        std::map<int, int> m_rootToIndex;

        int m_warnings = 0;
        int m_freeWireCount = 0;
        std::vector<std::string> m_warningText;
    };

    std::string BuildHeader(const char* title, const std::string& moduleName,
                            const Builder& builder)
    {
        const wxString time =
            wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");

        std::string text;
        text += "// ============================================================\n";
        text += "// ";
        text += title;
        text += "\n";
        text += "// 由 DrawGate 自动生成 —— 请勿手工修改后当作工程文件使用\n";
        text += "// 模块: ";
        text += moduleName;
        text += "\n// 时间: ";
        text += FromWx(time);
        text += "\n// 元件: " + std::to_string(builder.GateCount());
        text += "  网络: " + std::to_string(builder.Nets().size());
        text += "\n// ============================================================\n\n";

        return text;
    }
}

// ============================================================
// Verilog 导出
// ============================================================
bool Netlist::ExportVerilog(const wxString& path,
                            const std::vector<Gate*>& gates,
                            const std::vector<Wire>& wires,
                            const wxString& moduleNameRaw,
                            NetlistSummary& summary,
                            wxString& error)
{
    Builder builder(gates, wires);

    if (!builder.Build(error))
    {
        return false;
    }

    const std::string moduleName = SanitizeIdentifier(
        moduleNameRaw, "drawgate_top");

    std::vector<int> inputs;
    std::vector<int> outputs;
    std::vector<int> internals;

    for (std::size_t i = 0; i < builder.Nets().size(); ++i)
    {
        const Net& net = builder.Nets()[i];

        if (net.primaryInput)
        {
            inputs.push_back(static_cast<int>(i));
        }
        else if (net.primaryOutput)
        {
            outputs.push_back(static_cast<int>(i));
        }
        else
        {
            internals.push_back(static_cast<int>(i));
        }
    }

    // 按名字排序（in1, in2, ..., n1, n2, ...），让导出顺序更自然
    auto byName = [&builder](int first, int second)
    {
        const std::string& a = builder.Nets()[first].name;
        const std::string& b = builder.Nets()[second].name;

        if (a.size() != b.size())
        {
            return a.size() < b.size();
        }

        return a < b;
    };

    std::sort(inputs.begin(), inputs.end(), byName);
    std::sort(outputs.begin(), outputs.end(), byName);
    std::sort(internals.begin(), internals.end(), byName);

    std::string text = BuildHeader("DrawGate Verilog 结构网表",
                                   moduleName, builder);

    if (inputs.empty() && outputs.empty())
    {
        text += "module " + moduleName + ";\n";
    }
    else
    {
        text += "module " + moduleName + " (\n";

        std::vector<std::string> ports;

        for (int net : inputs)
        {
            ports.push_back("    input  wire " + builder.Nets()[net].name);
        }

        for (int net : outputs)
        {
            ports.push_back("    output wire " + builder.Nets()[net].name);
        }

        for (std::size_t i = 0; i < ports.size(); ++i)
        {
            text += ports[i];

            if (i + 1 < ports.size())
            {
                text += ",";
            }

            text += "\n";
        }

        text += ");\n";
    }

    if (!internals.empty())
    {
        text += "\n";

        for (std::size_t i = 0; i < internals.size(); ++i)
        {
            text += "    wire " + builder.Nets()[internals[i]].name + ";\n";
        }
    }

    text += "\n";

    for (std::size_t i = 0; i < builder.GateCount(); ++i)
    {
        const Gate& gate = builder.GateAt(i);

        text += "    ";
        text += PrimitiveName(gate.type);
        text += " ";
        text += builder.RefAt(i);
        text += " (";

        std::string terminals;

        const int outNet = builder.OutputNet(i);

        if (outNet >= 0)
        {
            terminals += builder.Nets()[outNet].name;
        }

        for (int net : builder.InputNets(i))
        {
            terminals += ", ";

            if (net >= 0)
            {
                terminals += builder.Nets()[net].name;
            }
        }

        text += terminals;
        text += ");\n";
    }

    text += "\nendmodule\n";

    if (!WriteTextFile(path, text, error))
    {
        return false;
    }

    summary.instances = static_cast<int>(builder.GateCount());
    summary.nets = static_cast<int>(builder.Nets().size());
    summary.inputs = static_cast<int>(inputs.size());
    summary.outputs = static_cast<int>(outputs.size());
    summary.warnings = builder.WarningCount();
    return true;
}

// ============================================================
// BLIF 导出
// ============================================================
namespace
{
    // 每种门的积之和（.names 的真值表行）
    std::vector<std::string> CoverRows(GateType type)
    {
        switch (type)
        {
        case AND:       return { "11 1" };
        case NAND_GATE: return { "11 0" };
        case OR:        return { "1- 1", "-1 1" };
        case NOR_GATE:  return { "1- 0", "-1 0" };
        case XOR_GATE:  return { "10 1", "01 1" };
        case XNOR_GATE: return { "10 0", "01 0" };
        case NOT:       return { "0 1" };
        default:        return { "11 1" };
        }
    }
}

bool Netlist::ExportBlif(const wxString& path,
                         const std::vector<Gate*>& gates,
                         const std::vector<Wire>& wires,
                         const wxString& moduleNameRaw,
                         NetlistSummary& summary,
                         wxString& error)
{
    Builder builder(gates, wires);

    if (!builder.Build(error))
    {
        return false;
    }

    const std::string moduleName = SanitizeIdentifier(
        moduleNameRaw, "drawgate_top");

    std::string text;
    text += "# ============================================================\n";
    text += "# DrawGate BLIF 网表\n";
    text += "# 模块: " + moduleName + "\n";
    text += "# 时间: " +
        FromWx(wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S")) + "\n";
    text += "# 元件: " + std::to_string(builder.GateCount()) +
        "  网络: " + std::to_string(builder.Nets().size()) + "\n";
    text += "# ============================================================\n\n";

    text += ".model " + moduleName + "\n";

    std::string inputLine;
    std::string outputLine;

    for (const Net& net : builder.Nets())
    {
        if (net.primaryInput)
        {
            inputLine += " " + net.name;
        }
    }

    for (const Net& net : builder.Nets())
    {
        if (net.primaryOutput)
        {
            outputLine += " " + net.name;
        }
    }

    if (!inputLine.empty())
    {
        text += ".inputs" + inputLine + "\n";
    }

    if (!outputLine.empty())
    {
        text += ".outputs" + outputLine + "\n";
    }

    text += "\n";

    for (std::size_t i = 0; i < builder.GateCount(); ++i)
    {
        const Gate& gate = builder.GateAt(i);
        const int outNet = builder.OutputNet(i);

        if (outNet < 0)
        {
            continue;
        }

        text += ".names";

        for (int net : builder.InputNets(i))
        {
            if (net >= 0)
            {
                text += " " + builder.Nets()[net].name;
            }
        }

        text += " " + builder.Nets()[outNet].name + "\n";

        for (const std::string& row : CoverRows(gate.type))
        {
            text += row + "\n";
        }
    }

    text += "\n.end\n";

    if (!WriteTextFile(path, text, error))
    {
        return false;
    }

    summary.instances = static_cast<int>(builder.GateCount());
    summary.nets = static_cast<int>(builder.Nets().size());
    summary.inputs = 0;
    summary.outputs = 0;

    for (const Net& net : builder.Nets())
    {
        if (net.primaryInput)
        {
            ++summary.inputs;
        }

        if (net.primaryOutput)
        {
            ++summary.outputs;
        }
    }

    summary.warnings = builder.WarningCount();
    return true;
}
