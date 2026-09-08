// SPDX-License-Identifier: GPL-3.0-or-later
// 安装引导小工具：创建桌面快捷方式并启动 CartoonEye.exe
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include <string>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CoInitialize(nullptr);

    wchar_t self[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring selfPath(self);
    size_t pos = selfPath.find_last_of(L"\\/");
    std::wstring dir = (pos == std::wstring::npos) ? L"." : selfPath.substr(0, pos);
    std::wstring exe = dir + L"\\CartoonEye.exe";

    // 创建桌面快捷方式「卡通眼珠」
    IShellLinkW *psl = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void **>(&psl)))) {
        psl->SetPath(exe.c_str());
        psl->SetWorkingDirectory(dir.c_str());
        psl->SetDescription(L"卡通眼珠 - 桌面卡通眼睛，点击学习");
        IPersistFile *ppf = nullptr;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&ppf)))) {
            wchar_t desk[MAX_PATH] = {0};
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desk))) {
                std::wstring lnk = std::wstring(desk) + L"\\卡通眼珠.lnk";
                ppf->Save(lnk.c_str(), TRUE);
            }
            ppf->Release();
        }
        psl->Release();
    }

    // 启动程序
    ShellExecuteW(nullptr, L"open", exe.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL);
    CoUninitialize();
    return 0;
}
