#include <Windows.h>
#include <TlHelp32.h>
#include <io.h>
#include <string>

std::wstring AsciiToWide(const std::string& _strSrc)
{
    int wideLen = MultiByteToWideChar(CP_ACP, 0, _strSrc.c_str(), _strSrc.size(), 0, 0);
    wchar_t *pWide = (wchar_t*)malloc(sizeof(wchar_t)*wideLen);
    MultiByteToWideChar(CP_ACP, 0, _strSrc.c_str(), -1, pWide, wideLen);
    std::wstring ret_str(pWide);
    free(pWide);
    return ret_str;
}

bool KillProcess(const std::wstring& appName)
{
    bool result = false;
    PROCESSENTRY32 pe;
    pe.dwSize=sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return result;
    if(!Process32First(hSnapshot, &pe)) return result;
    do {
        if (pe.szExeFile == appName) {
            HANDLE handlePro = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (handlePro == NULL) break;
            result = TerminateProcess(handlePro, 0) == 0;
            break;
        }
    } while (Process32Next(hSnapshot, &pe));
    CloseHandle(hSnapshot);
    return result;
}

bool IsAppRunning(const std::wstring& appName)
{
    bool result = false;
    PROCESSENTRY32 pe;
    pe.dwSize=sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(!Process32First(hSnapshot, &pe)) return result;
    do {
        if(pe.szExeFile == appName) {
            result = pe.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &pe));
    CloseHandle(hSnapshot);
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 5) return -1;
    int result = -1;
    const std::string& sourceFilePath = argv[1];
    const std::string& deleteFilePath = argv[2];
    const std::string& destFilePath = argv[3];
    while(true) {
        size_t pos = deleteFilePath.rfind('\\');
        if (pos == std::string::npos) break;
        const std::wstring& appName = AsciiToWide(deleteFilePath.substr(pos + 1, deleteFilePath.size() - pos - 1));
        for (int i = 0; i < 5; ++i) {
            if (IsAppRunning(appName)) Sleep(200);
            else break;
        }
        KillProcess(appName);
        if (remove(deleteFilePath.c_str())) break;
        if (rename(sourceFilePath.c_str(), destFilePath.c_str())) break;
        result = 1;
        break;
    }
    WinExec(("\"" + destFilePath + "\" -update " + std::to_string(result) + " " + argv[4]).c_str(), SW_SHOWDEFAULT);
    return 0;
}
