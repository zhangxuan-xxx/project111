// ============================================================
// SelfTest.h —— 无界面的自检入口（test.exe --selftest）
// 用于验证 .dgate 读写、PNG 导出、网表导出是否正常
// ============================================================
#pragma once

#include <wx/string.h>

// 运行全部自检，把结果写入 logPath；返回 0 表示全部通过
int RunSelfTest(const wxString& logPath);
