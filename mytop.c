#include <stdio.h>
#include <stdlib.h>
// clang-format off
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
// clang-format on

void system_info() {
    puts(
        "\n===\n"
        "系统信息\n"
        "---\n");

    SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);

    printf("页大小%luB\n", sysinfo.dwPageSize);
    printf("进程最低内存地址0x%p\n", sysinfo.lpMinimumApplicationAddress);
    printf("进程最高内存地址0x%p\n", sysinfo.lpMaximumApplicationAddress);
    printf("分配粒度%luB\n", sysinfo.dwAllocationGranularity);
    printf("处理器数%lu\n", sysinfo.dwNumberOfProcessors);

    puts("");
}

void performance_info() {
    puts(
        "\n===\n"
        "性能信息\n"
        "---\n");

    PERFORMANCE_INFORMATION perfinfo;
    GetPerformanceInfo(&perfinfo, sizeof perfinfo);

    printf("已被提交的页数%d\n", perfinfo.CommitTotal);
    printf("可提交页数极限%d\n", perfinfo.CommitLimit);
    printf("同时处于提交态的页数峰值%d\n", perfinfo.CommitPeak);
    printf("物理页数%d\n", perfinfo.PhysicalTotal);
    printf("可用物理页数%d\n", perfinfo.PhysicalAvailable);
    printf("系统缓存页数%d\n", perfinfo.SystemCache);
    printf("内核页总数%d\n", perfinfo.KernelTotal);
    printf("内核已分页数%d\n", perfinfo.KernelPaged);
    printf("内核未分页数%d\n", perfinfo.PageSize);
    printf("页大小%dB\n", perfinfo.PageSize);
    printf("Handle数量%d\n", perfinfo.HandleCount);
    printf("处理器数量%d\n", perfinfo.ProcessCount);
    printf("线程数量%d\n", perfinfo.ThreadCount);

    puts("");
}

void memory_info() {
    const int div = 1024;
    const int width = 15;
    puts(
        "\n===\n"
        "当前内存状态\n"
        "---\n");

    MEMORYSTATUSEX meminfo;
    meminfo.dwLength = sizeof meminfo;
    GlobalMemoryStatusEx(&meminfo);

    printf("当前内存已使用 %*lu%%\n", width, meminfo.dwMemoryLoad);
    printf("物理内存总量 %*llu KB\n", width, meminfo.ullTotalPhys / div);
    printf("物理内存剩余 %*llu KB\n", width, meminfo.ullAvailPhys / div);
    printf("分页文件总量 %*llu KB\n", width, meminfo.ullTotalPageFile / div);
    printf("分页文件剩余 %*llu KB\n", width, meminfo.ullAvailPageFile / div);
    printf("虚拟内存总量 %*llu KB\n", width, meminfo.ullTotalVirtual / div);
    printf("虚拟内存剩余 %*llu KB\n", width, meminfo.ullAvailVirtual / div);
    printf("扩展内存剩余 %*llu KB\n", width,
           meminfo.ullAvailExtendedVirtual / div);

    puts("");
}

void process_info() {
    printf(
        "\n===\n"
        "%所有进程信息\n"
        "---\n");

    HANDLE process_snap;
    HANDLE process;
    PROCESSENTRY32 pe32;

    process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snap == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot %d.\n", GetLastError());
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(process_snap, &pe32)) {
        printf("Process32First %d.\n", GetLastError());
        CloseHandle(process_snap);  // clean the snapshot object
        return;
    }

    do {
        printf("进程名: %s\n", pe32.szExeFile);
        printf("\t进程ID: %lu\n", pe32.th32ProcessID);
        printf("\t线程数 %lu\n", pe32.cntThreads);
        printf("\t父进程ID %lu\n\n", pe32.th32ParentProcessID);
    } while (Process32Next(process_snap, &pe32));

    CloseHandle(process_snap);
    puts("");
}

void query_process_info() {
    DWORD pid;
    printf("请输入要查询的进程ID: ");
    scanf("%lu", &pid);

    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (process == NULL) {
        printf("%lu号进程不存在!\n", pid);
        return;
    }

    printf(
        "\n===\n"
        "%lu号进程虚拟内存状态\n"
        "---\n",
        pid);

    SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);

    MEMORY_BASIC_INFORMATION mem_b_info;

    LPCVOID addr = (LPVOID)sysinfo.lpMinimumApplicationAddress;
    while (addr < sysinfo.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(process, addr, &mem_b_info, sizeof(mem_b_info))) {
            printf("0x%p\t状态：", addr);

            printf(MEM_COMMIT == mem_b_info.State
                       ? "提交"
                       : MEM_FREE == mem_b_info.State
                             ? "空闲"
                             : MEM_RESERVE == mem_b_info.State ? "预留"
                                                               : "其他");

            printf("\t保护：");
            // 不知道为什么，如果ap是1的话PAGE_NOACCESS == ap总是判断不等于
            DWORD ap = mem_b_info.AllocationProtect;
            printf(PAGE_EXECUTE == ap
                       ? "执行"
                       : PAGE_EXECUTE_READ == ap
                             ? "执行/读"
                             : PAGE_EXECUTE_READWRITE == ap
                                   ? "执行/读/写"
                                   : PAGE_EXECUTE_WRITECOPY == ap
                                         ? "执行/写时复制"
                                         : PAGE_NOACCESS == ap
                                               ? "禁止访问"
                                               : PAGE_READONLY == ap
                                                     ? "只读"
                                                     : PAGE_READWRITE == ap
                                                           ? "读写"
                                                           : PAGE_WRITECOPY ==
                                                                     ap
                                                                 ? "写时复制"
                                                                 : "其他");

            printf("\t类型：");
            // Type为0是什么鬼？https://docs.microsoft.com/zh-cn/windows/win32/api/winnt/ns-winnt-memory_basic_information
            printf(MEM_IMAGE == mem_b_info.Type
                       ? "映像"
                       : MEM_MAPPED == mem_b_info.Type
                             ? "被映射"
                             : MEM_PRIVATE == mem_b_info.Type ? "私有"
                                                              : "其他");

            puts("");

            addr = (PBYTE)addr + mem_b_info.RegionSize;
        }
    }
}

int main() {
    system("chcp 65001");
    for (;;) {
        puts(
            "\n1 展示系统信息\n"
            "2 展示性能信息\n"
            "3 展示内存状态\n"
            "4 列出所有进程信息\n"
            "5 查询某进程虚拟地址\n"
            "0 退出\n");

        int op;
        scanf("%d", &op);

        0 == op
            ? exit(EXIT_SUCCESS)
            : 1 == op
                  ? system_info()
                  : 2 == op
                        ? performance_info()
                        : 3 == op ? memory_info()
                                  : 4 == op ? process_info()
                                            : 5 == op ? query_process_info()
                                                      : puts("Invalid option!");
    }
}