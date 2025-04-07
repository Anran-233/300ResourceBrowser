#include <Windows.h>
#include <io.h>
#include <string>

int main(int argc, char *argv[])
{
    if (argc < 5) return -1;
    const char *sourceFile = argv[1];
    const char *deleteFile = argv[2];
    const char *destFile = argv[3];
    bool result = false;
    for (int i = 0; i < 10; ++i) {
        Sleep(200);
        if (remove(deleteFile)) continue;
        result = true;
    }
    auto app = std::string() + "\"" + destFile + "\" -update " + (result ? "1" : "-1") + " " + argv[4];
    if (!result) WinExec(app.c_str(), SW_SHOWDEFAULT);
    else if (rename(sourceFile, destFile)) {
        MessageBoxW(NULL, L"无法替换文件！请手动替换新版本文件！", L"更新失败", MB_OK);
        WinExec(std::string("explorer /select, ").append(sourceFile).c_str(), SW_SHOWDEFAULT);
    }
    else WinExec(app.c_str(), SW_SHOWDEFAULT);
    return 0;
}
