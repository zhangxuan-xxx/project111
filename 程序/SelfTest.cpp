#include "SelfTest.h"

#include <wx/filename.h>
#include <wx/frame.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ComponentLibrary.h"
#include "DrawBoard.h"
#include "Netlist.h"
#include "SceneIO.h"

namespace
{
    struct Report
    {
        std::vector<std::string> lines;
        bool ok = true;

        void Check(bool condition, const std::string& what)
        {
            lines.push_back(
                std::string(condition ? "[PASS] " : "[FAIL] ") + what);

            if (!condition)
            {
                ok = false;
            }
        }
    };

    std::string ToUtf8(const wxString& text)
    {
        return text.ToStdString(wxConvUTF8);
    }

    wxString TempFile(const wxString& name)
    {
        return wxFileName::GetTempDir() + wxT("\\") + name;
    }

    std::string ReadFile(const wxString& path)
    {
        std::ifstream file(ToUtf8(path).c_str(), std::ios::binary);

        if (!file.is_open())
        {
            return std::string();
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

int RunSelfTest(const wxString& logPath)
{
    Report report;

    const wxString dgatePath = TempFile(wxT("drawgate-selftest.dgate"));
    const wxString pngPath = TempFile(wxT("drawgate-selftest.png"));
    const wxString verilogPath = TempFile(wxT("drawgate-selftest.v"));
    const wxString blifPath = TempFile(wxT("drawgate-selftest.blif"));

    // --------------------------------------------------------
    // 1. JSON 写入
    // --------------------------------------------------------
    std::vector<Gate*> gates;

    Gate* andGate = SceneIO::CreateGate(AND, wxPoint(200, 160));
    andGate->id = 1;
    andGate->label = wxT("U1");

    Gate* notGate = SceneIO::CreateGate(NOT, wxPoint(360, 160));
    notGate->id = 2;

    Gate* orGate = SceneIO::CreateGate(OR, wxPoint(520, 160));
    orGate->id = 3;
    orGate->label = wxT("U3");

    gates.push_back(andGate);
    gates.push_back(notGate);
    gates.push_back(orGate);

    std::vector<Wire> wires(2);

    wires[0].start = andGate->GetOutputPort();
    wires[0].end = notGate->GetInputPort(0);
    wires[0].startComponentId = 1;
    wires[0].endComponentId = 2;
    wires[0].endPortIndex = 0;

    wires[1].start = notGate->GetOutputPort();
    wires[1].end = orGate->GetInputPort(0);
    wires[1].startComponentId = 2;
    wires[1].endComponentId = 3;
    wires[1].endPortIndex = 0;

    std::vector<DrawBoard::BoardText> texts(1);
    texts[0].pos = wxPoint(100, 60);
    texts[0].text = wxT("测试标注");

    wxString error;

    const bool saved = SceneIO::Save(
        dgatePath, gates, wires, texts, 50, false, false, error);

    report.Check(saved, "Save .dgate: " + ToUtf8(error));

    for (Gate* gate : gates)
    {
        delete gate;
    }

    gates.clear();

    // --------------------------------------------------------
    // 2. JSON 读回并比对
    // --------------------------------------------------------
    SceneFileData data;
    const bool loaded = SceneIO::Load(dgatePath, data, error);

    report.Check(loaded, "Load .dgate: " + ToUtf8(error));

    if (loaded)
    {
        report.Check(data.gates.size() == 3, "gate count == 3");
        report.Check(data.wires.size() == 2, "wire count == 2");

        report.Check(
            data.texts.size() == 1 &&
            data.texts[0].text == wxT("测试标注"),
            "Chinese text round-trip (UTF-8)");

        report.Check(
            data.gridSize == 50 && !data.gridVisible && !data.snapToGrid,
            "canvas settings round-trip");

        report.Check(data.nextGateId == 4, "nextGateId == 4");

        if (data.gates.size() == 3)
        {
            report.Check(
                data.gates[0]->type == AND &&
                data.gates[0]->center == wxPoint(200, 160) &&
                data.gates[0]->label == wxT("U1"),
                "gate 1 type / position / label");

            report.Check(
                data.gates[1]->type == NOT &&
                data.gates[1]->label.empty(),
                "gate 2 type / empty label");

            report.Check(
                data.gates[2]->type == OR &&
                data.gates[2]->label == wxT("U3"),
                "gate 3 type / label");
        }

        if (data.wires.size() == 2)
        {
            report.Check(
                data.wires[0].startComponentId == 1 &&
                data.wires[0].endComponentId == 2 &&
                data.wires[0].endPortIndex == 0,
                "wire references round-trip");
        }
    }

    // --------------------------------------------------------
    // 3. 网表导出
    // --------------------------------------------------------
    if (loaded)
    {
        NetlistSummary summary;
        wxString netlistError;

        const bool verilogOk = Netlist::ExportVerilog(
            verilogPath, data.gates, data.wires,
            wxT("selftest_top"), summary, netlistError);

        report.Check(verilogOk, "Export Verilog: " + ToUtf8(netlistError));

        if (verilogOk)
        {
            report.Check(summary.instances == 3, "Verilog instances == 3");
            report.Check(summary.inputs == 3, "Verilog primary inputs == 3");
            report.Check(summary.outputs == 1, "Verilog primary outputs == 1");

            const std::string text = ReadFile(verilogPath);

            report.Check(
                text.find("module selftest_top") != std::string::npos,
                "Verilog module header");

            report.Check(
                text.find("and U1") != std::string::npos &&
                text.find("not U2") != std::string::npos &&
                text.find("or U3") != std::string::npos,
                "Verilog gate instances");
        }

        NetlistSummary blifSummary;
        wxString blifError;

        const bool blifOk = Netlist::ExportBlif(
            blifPath, data.gates, data.wires,
            wxT("selftest_top"), blifSummary, blifError);

        report.Check(blifOk, "Export BLIF: " + ToUtf8(blifError));

        if (blifOk)
        {
            const std::string text = ReadFile(blifPath);

            report.Check(
                text.find(".names") != std::string::npos &&
                text.find(".end") != std::string::npos,
                "BLIF contains .names and .end");

            report.Check(
                blifSummary.instances == 3, "BLIF instances == 3");
        }
    }

    // --------------------------------------------------------
    // 3.1 自定义元件：真值表求值、工程往返、仿真及网表导出
    // --------------------------------------------------------
    {
        wxString libraryError;
        const bool libraryLoaded =
            ComponentLibrary::LoadCustomComponents(libraryError);

        report.Check(
            libraryLoaded,
            "Load custom component library: " +
                ToUtf8(libraryError));

        const wxString libraryTestName =
            wxT("__DrawGate自检元件__");

        for (std::size_t i = 0;
             i < ComponentLibrary::GetCustomComponents().size();
             ++i)
        {
            if (ComponentLibrary::GetCustomComponents()[i].name ==
                libraryTestName)
            {
                ComponentLibrary::DeleteCustomComponent(i, libraryError);
                break;
            }
        }

        CustomGateDefinition definition;
        definition.name = wxT("自定义异或");
        definition.inputCount = 2;
        definition.truthTable = 0x6; // 01 和 10 时输出 1

        CustomGateDefinition libraryDefinition = definition;
        libraryDefinition.name = libraryTestName;
        libraryError.clear();
        const bool libraryAdded =
            ComponentLibrary::AddCustomComponent(
                libraryDefinition, libraryError);
        report.Check(
            libraryAdded,
            "Save custom component library: " +
                ToUtf8(libraryError));

        libraryError.clear();
        const bool libraryReloaded =
            ComponentLibrary::LoadCustomComponents(libraryError);
        bool foundInLibrary = false;

        if (libraryReloaded)
        {
            for (const CustomGateDefinition& item :
                 ComponentLibrary::GetCustomComponents())
            {
                if (item.name == libraryTestName &&
                    item.inputCount == 2 &&
                    item.truthTable == 6)
                {
                    foundInLibrary = true;
                    break;
                }
            }
        }

        report.Check(
            libraryReloaded && foundInLibrary,
            "Custom component library round-trip");

        for (std::size_t i = 0;
             i < ComponentLibrary::GetCustomComponents().size();
             ++i)
        {
            if (ComponentLibrary::GetCustomComponents()[i].name ==
                libraryTestName)
            {
                ComponentLibrary::DeleteCustomComponent(i, libraryError);
                break;
            }
        }

        CustomGate standalone(wxPoint(100, 100), definition);
        bool output = false;
        bool evaluateOk = true;

        for (int a = 0; a <= 1; ++a)
        {
            for (int b = 0; b <= 1; ++b)
            {
                std::vector<bool> inputs = {
                    a != 0, b != 0
                };

                if (!standalone.Evaluate(inputs, output) ||
                    output != (a != b))
                {
                    evaluateOk = false;
                }
            }
        }

        report.Check(
            evaluateOk,
            "custom gate truth table evaluation (XOR)");

        std::vector<Gate*> customGates;
        Gate* savedCustom =
            new CustomGate(wxPoint(240, 180), definition);
        savedCustom->id = 201;
        savedCustom->label = wxT("C1");
        customGates.push_back(savedCustom);

        const wxString customPath =
            TempFile(wxT("drawgate-custom-selftest.dgate"));
        wxString customError;

        const bool customSaved = SceneIO::Save(
            customPath, customGates, {},
            {}, 20, true, true, customError);

        report.Check(
            customSaved,
            "Save custom .dgate: " + ToUtf8(customError));

        delete savedCustom;
        customGates.clear();

        SceneFileData customData;
        customError.clear();
        const bool customLoaded =
            SceneIO::Load(customPath, customData, customError);

        report.Check(
            customLoaded,
            "Load custom .dgate: " + ToUtf8(customError));

        if (customLoaded && customData.gates.size() == 1)
        {
            const CustomGate* loadedCustom =
                dynamic_cast<const CustomGate*>(
                    customData.gates[0]);

            report.Check(
                loadedCustom != nullptr &&
                loadedCustom->GetDefinition().name ==
                    definition.name &&
                loadedCustom->GetDefinition().inputCount == 2 &&
                loadedCustom->GetDefinition().truthTable == 6,
                "custom definition round-trip");
        }

        for (Gate* gate : customData.gates)
        {
            delete gate;
        }

        // ---- 自定义元件的动态仿真 ----
        wxFrame* customFrame = new wxFrame(
            nullptr, wxID_ANY, wxT("Custom Gate SelfTest"));
        DrawBoard* customBoard = new DrawBoard(customFrame);

        Gate* inputA = new InputSwitch(wxPoint(100, 160));
        inputA->id = 301;

        Gate* inputB = new InputSwitch(wxPoint(100, 260));
        inputB->id = 302;

        Gate* simulatedCustom =
            new CustomGate(wxPoint(320, 210), definition);
        simulatedCustom->id = 303;

        Gate* outputGate = new OutputIndicator(wxPoint(520, 210));
        outputGate->id = 304;

        std::vector<Wire> customWires(3);

        customWires[0].start = inputA->GetOutputPort();
        customWires[0].end = simulatedCustom->GetInputPort(0);
        customWires[0].startComponentId = 301;
        customWires[0].endComponentId = 303;
        customWires[0].endPortIndex = 0;

        customWires[1].start = inputB->GetOutputPort();
        customWires[1].end = simulatedCustom->GetInputPort(1);
        customWires[1].startComponentId = 302;
        customWires[1].endComponentId = 303;
        customWires[1].endPortIndex = 1;

        customWires[2].start = simulatedCustom->GetOutputPort();
        customWires[2].end = outputGate->GetInputPort(0);
        customWires[2].startComponentId = 303;
        customWires[2].endComponentId = 304;
        customWires[2].endPortIndex = 0;

        customBoard->SetScene(
            { inputA, inputB, simulatedCustom, outputGate },
            customWires, {}, 305, 20, true, true);

        bool simulationOk = true;

        for (int a = 0; a <= 1; ++a)
        {
            for (int b = 0; b <= 1; ++b)
            {
                Gate* currentA = nullptr;
                Gate* currentB = nullptr;
                Gate* currentOutput = nullptr;

                for (Gate* gate : customBoard->GetGates())
                {
                    if (gate->id == 301) currentA = gate;
                    if (gate->id == 302) currentB = gate;
                    if (gate->id == 304) currentOutput = gate;
                }

                if (currentA == nullptr ||
                    currentB == nullptr ||
                    currentOutput == nullptr)
                {
                    simulationOk = false;
                    break;
                }

                currentA->logicValue = a != 0;
                currentB->logicValue = b != 0;

                wxString simulationError;

                if (!customBoard->RecalculateCircuit(
                        simulationError) ||
                    currentOutput->logicValue != (a != b))
                {
                    simulationOk = false;
                }
            }
        }

        report.Check(
            simulationOk,
            "custom gate dynamic propagation");

        NetlistSummary customSummary;
        wxString customNetlistError;
        const wxString customVerilogPath =
            TempFile(wxT("drawgate-custom-selftest.v"));
        const wxString customBlifPath =
            TempFile(wxT("drawgate-custom-selftest.blif"));

        const bool customVerilogOk = Netlist::ExportVerilog(
            customVerilogPath,
            customBoard->GetGates(),
            customBoard->GetWires(),
            wxT("custom_selftest"), customSummary,
            customNetlistError);

        report.Check(
            customVerilogOk,
            "Export custom Verilog: " +
                ToUtf8(customNetlistError));

        if (customVerilogOk)
        {
            const std::string text = ReadFile(customVerilogPath);
            report.Check(
                text.find("assign ") != std::string::npos,
                "Verilog contains custom assign");
        }

        customNetlistError.clear();
        const bool customBlifOk = Netlist::ExportBlif(
            customBlifPath,
            customBoard->GetGates(),
            customBoard->GetWires(),
            wxT("custom_selftest"), customSummary,
            customNetlistError);

        report.Check(
            customBlifOk,
            "Export custom BLIF: " +
                ToUtf8(customNetlistError));

        if (customBlifOk)
        {
            const std::string text = ReadFile(customBlifPath);
            report.Check(
                text.find("01 1") != std::string::npos &&
                text.find("10 1") != std::string::npos,
                "BLIF contains custom truth-table rows");
        }

        delete customFrame;
    }

    // --------------------------------------------------------
    // 3.5 T 型连接：线头搭在另一条线的中间，应当并入同一网络
    // --------------------------------------------------------
    {
        Gate* first = SceneIO::CreateGate(AND, wxPoint(200, 160));
        first->id = 101;

        Gate* second = SceneIO::CreateGate(OR, wxPoint(400, 160));
        second->id = 102;

        std::vector<Gate*> junctionGates = { first, second };

        std::vector<Wire> junctionWires(2);

        junctionWires[0].start = first->GetOutputPort();
        junctionWires[0].end = second->GetInputPort(0);
        junctionWires[0].startComponentId = 101;
        junctionWires[0].endComponentId = 102;
        junctionWires[0].endPortIndex = 0;

        // 起点落在第一条线的水平段中间（x = 260..360 的中点 310）
        junctionWires[1].start = wxPoint(310, 160);
        junctionWires[1].end = second->GetInputPort(1);
        junctionWires[1].endComponentId = 102;
        junctionWires[1].endPortIndex = 1;

        wxString junctionError;
        NetlistSummary junctionSummary;

        const bool junctionOk = Netlist::ExportVerilog(
            TempFile(wxT("drawgate-selftest-tj.v")),
            junctionGates, junctionWires,
            wxT("t_junction"), junctionSummary, junctionError);

        report.Check(junctionOk,
            "T-junction export: " + ToUtf8(junctionError));

        report.Check(
            junctionSummary.inputs == 2 && junctionSummary.outputs == 1,
            "T-junction merges net (inputs == 2)");

        for (Gate* gate : junctionGates)
        {
            delete gate;
        }
    }

    // --------------------------------------------------------
    // 3.6 坏文件应当被拒绝，并给出可读的错误
    // --------------------------------------------------------
    {
        const wxString badPath = TempFile(wxT("drawgate-bad.dgate"));

        {
            std::ofstream bad(ToUtf8(badPath).c_str(), std::ios::binary);
            bad << "{\"format\":\"dgate\",\"version\":1,\"gates\":[";
        }

        SceneFileData badData;
        wxString badError;

        const bool badOk = SceneIO::Load(badPath, badData, badError);

        report.Check(
            !badOk && !badError.empty(),
            "broken JSON is rejected with a message");
    }

    // --------------------------------------------------------
    // 4. PNG 导出（隐藏窗口 + 内存位图）
    // --------------------------------------------------------
    if (loaded)
    {
        wxFrame* frame = new wxFrame(
            nullptr, wxID_ANY, wxT("DrawGate SelfTest"));

        DrawBoard* board = new DrawBoard(frame);

        board->SetScene(
            std::move(data.gates),
            std::move(data.wires),
            data.texts,
            data.nextGateId,
            data.gridSize,
            data.gridVisible,
            data.snapToGrid);

        wxString pngError;
        const bool pngOk = board->ExportPng(pngPath, pngError);

        report.Check(pngOk, "Export PNG: " + ToUtf8(pngError));

        if (pngOk)
        {
            std::ifstream file(
                ToUtf8(pngPath).c_str(), std::ios::binary);

            unsigned char signature[8] = {};

            file.read(
                reinterpret_cast<char*>(signature),
                sizeof(signature));

            report.Check(
                signature[0] == 0x89 && signature[1] == 'P' &&
                signature[2] == 'N' && signature[3] == 'G',
                "PNG file signature");

            file.seekg(0, std::ios::end);
            const std::streamoff size = file.tellg();

            report.Check(size > 200, "PNG size > 200 bytes");
        }

        delete frame;
    }

    // --------------------------------------------------------
    // 5. 写出报告
    // --------------------------------------------------------
    std::string output;

    for (const std::string& line : report.lines)
    {
        output += line;
        output += "\n";
    }

    output += report.ok ? "RESULT: PASS\n" : "RESULT: FAIL\n";

    std::ofstream file(logPath.ToStdString(wxConvUTF8).c_str(),
                       std::ios::binary);

    if (file.is_open())
    {
        file << output;
    }

    return report.ok ? 0 : 1;
}
