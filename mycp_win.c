#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>

void dfs(char from[], char to[]) {
    CreateDirectory(to, NULL);
    SetFileAttributes(to, GetFileAttributes(from));

    if (to[strlen(to) - 1] != '\\') strcat(to, "\\");
    if (from[strlen(from) - 1] != '\\') strcat(from, "\\");
    strcat(from, "*");

    WIN32_FIND_DATA file_data;
    HANDLE search;
    search = FindFirstFile(from, &file_data);
    if (search == INVALID_HANDLE_VALUE) {
        FindClose(search);
        return;  // 空文件夹
    }
    char next_from[FILENAME_MAX], next_to[FILENAME_MAX], new_path[FILENAME_MAX],
        now_path[FILENAME_MAX];

    BOOL finished = FALSE;

    from[strlen(from) - 1] = 0;  // 去掉*好拼接
    while (!finished) {
        if (strcmp(".", file_data.cFileName) &&
            strcmp("..", file_data.cFileName)) {
            if (FILE_ATTRIBUTE_DIRECTORY & file_data.dwFileAttributes) {
                sprintf(next_from, "%s%s", from, file_data.cFileName);
                sprintf(next_to, "%s%s", to, file_data.cFileName);

                dfs(next_from, next_to);

                // 根据原文件夹设置新文件夹的修改时间，
                // 因为在新文件夹里复制了别的文件，导致新文件夹修改时间是现在
                FILETIME creationTime, accessTime, writeTime;

                HANDLE nFromHandle =
                    CreateFile(next_from, GENERIC_READ, 0, 0, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, 0);
                if (INVALID_HANDLE_VALUE == nFromHandle)
                    printf("Can't open %s! %d\n", next_from, GetLastError());
                GetFileTime(nFromHandle, &creationTime, &accessTime,
                            &writeTime);

                HANDLE nToHandle =
                    CreateFile(next_to, FILE_WRITE_ATTRIBUTES, 0, 0,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
                if (INVALID_HANDLE_VALUE == nToHandle)
                    printf("Can't open %s! %d\n", next_to, GetLastError());
                SetFileTime(nToHandle, &creationTime, &accessTime, &writeTime);

                CloseHandle(nFromHandle);
                CloseHandle(nToHandle);
            } else {
                sprintf(now_path, "%s%s", from, file_data.cFileName);
                sprintf(new_path, "%s%s", to, file_data.cFileName);
                if (!CopyFile(now_path, new_path, FALSE))
                    printf("Can't copy! %s %d\n", now_path, GetLastError());
            }
        }

        if (!FindNextFile(search, &file_data)) {
            if (ERROR_NO_MORE_FILES == GetLastError()) {
                finished = TRUE;
                FindClose(search);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) puts("Not enough arguments");

    char from[FILENAME_MAX], to[FILENAME_MAX];
    strcpy(from, argv[1]);
    strcpy(to, argv[2]);

    printf("from: %s to: %s\n", from, to);
    dfs(from, to);

    return 0;
}