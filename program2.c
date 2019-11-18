#include <stdio.h>
#include <windows.h>

int main(int argc, char * argv[]) {
    puts("begin to sleep");
    Sleep(atoi(argv[1]) * 1000);
    puts("sleep finished");
    return 0;
}