#include <stdio.h>
#include <windows.h>

int main(int argc, char * argv[]){
    // use `chcp 65001` first
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof si);
    memset(&pi, 0, sizeof pi);
    si.cb = sizeof si;

    if (argc < 2) {
        puts("not enough argument");
        return 0;
    }

    for (int i = 2; i < argc; i++) {
        strcat(argv[1], " ");
        strcat(argv[1], argv[i]);
    }

    if(!CreateProcess(NULL, argv[1], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        puts("create process failed");
        return 0;
    }
    SYSTEMTIME start, end;
    GetSystemTime(&start);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetSystemTime(&end);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (end.wMilliseconds < start.wMilliseconds) {
        end.wSecond -= 1;
        end.wMilliseconds += 1000;
    }
    if (end.wSecond < start.wSecond) {
        end.wMinute -= 1;
        end.wSecond += 60;
    }
    if (end.wMinute < start.wMinute) {
        end.wHour -= 1;
        end.wMinute += 60;
    }

    printf("%d小时%d分钟%d秒%d毫秒\n", end.wHour-start.wHour, end.wMinute-start.wMinute, end.wSecond-start.wSecond, end.wMilliseconds-start.wMilliseconds);
    return 0;
}
