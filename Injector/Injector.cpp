// injector/src/main.cpp
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

DWORD FindProcessId(const std::wstring& procName) {
    PROCESSENTRY32W entry{ sizeof(entry) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32FirstW(snap, &entry)) {
        do {
            if (std::wstring(entry.szExeFile) == procName) { CloseHandle(snap); return entry.th32ProcessID; }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap); return 0;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage: injector <procname|pid> <full_path_to_dll>\n"; return 1;
    }
    DWORD pid = 0;
    std::wstring first = argv[1];
    if (iswdigit(first[0])) pid = (DWORD)_wtoi(first.c_str());
    else pid = FindProcessId(first);
    std::wstring dllPath = argv[2];
    if (!pid) { std::wcout << L"Proc not found\n"; return 1; }
    HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) { std::wcout << L"OpenProcess failed\n"; return 1; }
    size_t size = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProc, NULL, size, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProc, remoteMem, dllPath.c_str(), size, NULL);
    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLib = GetProcAddress(hKernel, "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)loadLib, remoteMem, 0, NULL);
    WaitForSingleObject(hThread, 5000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    if (exitCode == 0) {
        std::wcout << L"FAILED! Remote LoadLibraryW returned NULL.\n";
        std::wcout << L"Likely cause: File path incorrect or missing dependencies.\n";
    }
    else {
        std::wcout << L"SUCCESS! DLL loaded at address: " << (void*)exitCode << L"\n";
    }
    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread); CloseHandle(hProc);
    std::wcout << L"Injected.\n";
    return 0;
}
