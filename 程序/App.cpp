#include "App.h"
#include "cMain.h"
#include "SelfTest.h"

#include <wx/filename.h>

#include <cstdlib>

wxIMPLEMENT_APP(cApp);

bool cApp::OnInit()
{
	// 注册 PNG 等图片格式（导出 PNG 需要）
	wxInitAllImageHandlers();

	// 自检模式：test.exe --selftest [日志路径]
	// 不开窗口，跑完直接退出，供命令行验证使用
	if (argc > 1)
	{
		const wxString first = argv[1];

		if (first == wxT("--selftest"))
		{
			const wxString logPath = argc > 2
				? wxString(argv[2])
				: wxFileName::GetTempDir() + wxT("\\drawgate-selftest.log");

			const int code = RunSelfTest(logPath);
			std::exit(code);
		}
	}

	m_frame1 = new cMain();

	// 命令行给出的图纸文件：启动后直接打开
	if (argc > 1)
	{
		const wxString path = argv[1];

		if (wxFileName::FileExists(path))
		{
			m_frame1->OpenDocument(path);
		}
	}

	m_frame1->Show();
	return true;
}
