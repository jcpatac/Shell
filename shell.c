#include <stdio.h>
#include <afxres.h>
#include <tchar.h>
#include <ctype.h>
#include <io.h>

// ========================================== CONSTANTS ==========================================

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#define LINE_BUFFER 1024
#define TOKEN_BUFFER 64
#define TOKEN_DELIMITERS_WS " \"\a\n\r\t/"
#define TOKEN_DELIMITERS_WOS "\"\a\n\r\t/"
#define COMMANDS_SIZE 18
#define BUFFER_SIZE MAX_PATH
TCHAR CurrentDirBuffer[BUFFER_SIZE + 1];

int reset = 0;

// ========================================== FUNCTIONS ==========================================

int functionCD(char **args) {
    GetCurrentDirectory(BUFFER_SIZE, CurrentDirBuffer);
    if (args[1] == NULL) {
        printf("%s\n", CurrentDirBuffer);
    }else if (!SetCurrentDirectory(args[1])) {
        printf("The system cannot find the path specified.\n");
    }
    return 1;
}

int functionCHDIR(char **args) {
    functionCD(args);
}

int functionCLS() {
    CONSOLE_SCREEN_BUFFER_INFO scrInformation;
    HANDLE outHandle;
    DWORD cnt;
    DWORD scrChars;
    COORD firstCell = {0, 0};

    outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(outHandle, &scrInformation);
    scrChars = (DWORD) (scrInformation.dwSize.X * scrInformation.dwSize.Y);
    FillConsoleOutputCharacter(outHandle, ' ', scrChars, firstCell, &cnt);
    SetConsoleCursorPosition(outHandle, firstCell);
    return 1;
}

int functionCMD() {
    reset = 1;
    return 0;
}

int functionCOPY(char **args) {
    WIN32_FIND_DATA findData;
    WIN32_FIND_DATA dataDest;
    TCHAR dest_path[MAX_PATH];
    HANDLE findHandle;
    HANDLE destHandle;
    int fileCtr = 0;

    if (args[1] == NULL) {
        printf("\nInvalid syntax!");
    }
    findHandle = FindFirstFile(args[1], &findData);
    destHandle = FindFirstFile(args[2], &dataDest);
    if (dataDest.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
        strcpy(dest_path, args[2]);
        strcat(dest_path, "\\");
    }else {
        strcpy(dest_path, args[2]);
    }

    do{
        TCHAR temp[MAX_PATH];
        strcpy(temp, dest_path);
        if (dataDest.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            strcat(temp, findData.cFileName);
            if (CopyFile(findData.cFileName, temp, 0) == 0) {
                printf("Error while copying to folder %s", temp);
                return 1;
            }
        }else {
            GetFullPathName(dest_path, BUFFER_SIZE, dest_path, NULL);
            if (CopyFileA(findData.cFileName, args[2], 0) == 0) {
                printf("Error while copying to file!");
                return 1;
            }
        }
        fileCtr++;
    }while (FindNextFile(findHandle, &findData) != 0);
    printf("\t%d file(s) copied.\n", fileCtr);
    FindClose(findHandle);
    FindClose(destHandle);
    return 1;
}

int functionDATE(char **args) {
    char *days[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    SYSTEMTIME systemTime;

    if (args[1] == NULL) {
        GetSystemTime(&systemTime);
        int y = systemTime.wYear;
        int m = systemTime.wMonth;
        int d = systemTime.wDay;
        int date;

        date = ((d += (m < 3 ? y -- :(y - 2))), (23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400)) % 7;

        printf("The current date is: %s, %d/%d/%d\n", days[date], systemTime.wMonth, systemTime.wDay, systemTime.wYear);
    }
    char *newDate[10];
    printf("Enter the new date: (mm-dd-yyyy) ");
    scanf("%s" ,newDate);
    SetLocalTime((const SYSTEMTIME *) newDate);
    return 1;
}

int functionRMDIR(char **args) {
    LPCSTR dirPath;
    dirPath = args[1];

    if (RemoveDirectory(dirPath) == 0) {
        printf("The directory is not empty.\n");
    }
    return 1;
}

int delSingleFire(WIN32_FIND_DATA findData) {
    HANDLE fileHandle;
    LPCSTR targetFile = findData.cFileName;

    fileHandle = CreateFile(targetFile, GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | SECURITY_IMPERSONATION, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        printf("Could not open '%s'!", (char *) targetFile);
    }
    if (CloseHandle(fileHandle) == 0) {
        printf("\nCloseHandle() for %s file failed!", (char *) targetFile);
    }
    return DeleteFile(targetFile);
}

int functionDEL(char **args) {
    WIN32_FIND_DATA findData;
    TCHAR targetFolder[BUFFER_SIZE];
    HANDLE findHandle;

    if (args[1] == NULL) {
        printf("The syntax of the command is incorrect.\n");
    }

    findHandle = FindFirstFileA(args[1], &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        printf("Could Not Find '%s'", targetFolder);
        return 1;
    }
    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        strcat(targetFolder, TEXT("\\"));
    }

    do{
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            printf("Folder Found, using \"rmdir<space>[target_folder]\" instead.");
            functionRMDIR(args);
            return 1;
        }else {
            delSingleFire(findData);
        }
    } while (FindNextFile(findHandle, &findData) != 0);
    FindClose(findHandle);
    return 1;
}

int functionDIR(char **args) {
    WIN32_FIND_DATA findData;
    LARGE_INTEGER totalBytes;
    LARGE_INTEGER fileSize;
    TCHAR targetFolder[BUFFER_SIZE];
    HANDLE findHandle;

    if (args[1] == NULL) {
        args[1] = ".";
    }

    totalBytes.HighPart = 0;
    totalBytes.LowPart = 0;
    findHandle = FindFirstFile(args[1], &findData);

    if (findHandle == INVALID_HANDLE_VALUE) {
        printf("File Not Found.\n");
        return 1;
    }else {
        _tprintf(TEXT("\nDisplaying results for: %s\n\n"), args[1]);
    }

    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {\
        strcpy(targetFolder, args[1]);
        strcat(targetFolder, TEXT("\\*"));
        findHandle = FindFirstFileA(targetFolder, &findData);
    }else {
        strcpy(targetFolder, args[1]);
        findHandle = FindFirstFileA(targetFolder, &findData);
    }
    int folderCtr = 0;
    int fileCtr = 0;

    do{
        FILETIME filetime = findData.ftLastWriteTime;
        SYSTEMTIME st;
        FileTimeToLocalFileTime(&filetime, &filetime);
        FileTimeToSystemTime(&filetime, &st);
        char localDate[255];
        char localTime[255];
        GetDateFormat(LOCALE_IDATE, DATE_SHORTDATE, &st, NULL, localDate, 255);
        GetTimeFormat(LOCALE_ITIME, 2, &st, NULL, localTime, 255);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            _tprintf("%s   %s\t<DIR>\t\t%s\n", localDate, localTime, findData.cFileName);
            folderCtr++;
        }else {
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalBytes.LowPart += findData.nFileSizeLow;
            totalBytes.HighPart += findData.nFileSizeHigh;
            printf("%s   %s\t\t %llu", localDate, localTime, fileSize.QuadPart);
            printf(" %s\n", findData.cFileName);
            fileCtr++;
        }
    } while (FindNextFile(findHandle, &findData) != 0);

    printf("\t\t%d File(s)\t  %llu bytes\n\t\t%d Dir(s)\n", fileCtr,  totalBytes.QuadPart, folderCtr);
    FindClose(findHandle);
    return 1;
}

int functionMKDIR(char **args) {
    LPCSTR dirPath;
    dirPath = args[1];

    if (!CreateDirectory(dirPath, NULL)) {
        printf("The syntax of the command is incorrect.\n");
        return 1;
    }
}

int functionMOVE(char **args) {
    WIN32_FIND_DATA findData;
    WIN32_FIND_DATA dataDest;
    TCHAR destFolder[MAX_PATH];
    HANDLE findHandle;
    HANDLE destHandle;
    int fileCtr = 0;

    if (args[1] == NULL) {
        printf("The syntax of the command is incorrect.\n");
    }

    findHandle = FindFirstFile(args[1], &findData);
    destHandle = FindFirstFile(args[2], &dataDest);
    if (dataDest.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
        strcpy(destFolder, args[2]);
        strcat(destFolder, "\\");
    }else {
        strcpy(destFolder, args[2]);
    }

    do{
        TCHAR tempDest[MAX_PATH];
        strcpy(tempDest, destFolder);
        if (dataDest.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            strcat(tempDest, findData.cFileName);
            if (MoveFileA(findData.cFileName, tempDest) == 0) {
                printf("Error while moving folder '%s'", tempDest);
                return 1;
            }
            delSingleFire(findData);
        }else {
//            _tprintf("\nMoving '%s' to '%s'\n\n", findData.cFileName, destFolder);
            GetFullPathName(destFolder, BUFFER_SIZE, destFolder, NULL);
            if (MoveFileA(findData.cFileName, destFolder) == 0) {
                printf("Error while moving file!");
                return 1;
            }
            delSingleFire(findData);
        }
        fileCtr++;
    }while (FindNextFile(findHandle, &findData) != 0);

    printf("\t%d file(s) moved.\n", fileCtr);
    FindClose(findHandle);
    FindClose(destHandle);
    return 1;
}

int functionRENAME(char **args) {
    WIN32_FIND_DATA findData;
    HANDLE findHandle;
    if (args[1] == NULL) {
        printf("The syntax of the command is incorrect.\n");
        return 1;
    }
    findHandle = FindFirstFileA(args[1], &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        printf("The system cannot find the file specified.\n");
        FindClose(findHandle);
        return 1;
    }
    if (rename(args[1], args[2]) != 0) {
        printf("A duplicate file name exists, or the file cannot be found.\n");
    }
    FindClose(findHandle);
    return 1;
}

int functionEXIT() {
    return 0;
}

int functionTIME(char **args) {
    SYSTEMTIME systemTime;

    if (args[1] == NULL) {
        GetLocalTime(&systemTime);
        printf("The current time is: %02d:%02d:%02d.%02d\n", systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
    }
    char *newTime[10];
    printf("Enter the new time: ");
    scanf("%s", newTime);
    SetLocalTime((const SYSTEMTIME *) newTime);
    return 1;
}

int functionTYPE(char **args) {
    FILE *targetFile;
    char ch;

    targetFile = fopen(args[1], "r");
    if (targetFile == NULL) {
        printf("Access is denied.\n");
        return 1;
    }

    ch = (char) fgetc(targetFile);
    while (ch != EOF) {
        printf("%c", ch);
        ch = (char) fgetc(targetFile);
    }

    fclose(targetFile);
    return 1;
}

int functionIPCONFIG(char **args) {
    system("C:\\Windows\\System32\\ipconfig");
    return 1;
}

int functionCOLOR(char **args) {
    char color[10];
    strcat(color, "color ");
    strcat(color, args[1]);
    system(color);
    return 1;
}

int functionSTART(char **args) {
    system(args[1]);
    return 1;
}

// ========================================== DECLARATIONS ==========================================

char *commands[] = {
        "cd",
        "chdir",
        "cls",
        "cmd",
        "exit",
        "copy",
        "date",
        "del",
        "dir",
        "mkdir",
        "move",
        "rename",
        "rmdir",
        "time",
        "type",
        "ipconfig",
        "color",
        "start"
};

int (*pointers[]) (char **) = {
        &functionCD,
        &functionCHDIR,
        (int (*)(char **)) &functionCLS,
        (int (*)(char **)) &functionCMD,
        (int (*)(char **)) &functionEXIT,
        &functionCOPY,
        &functionDATE,
        &functionDEL,
        &functionDIR,
        &functionMKDIR,
        &functionMOVE,
        &functionRENAME,
        &functionRMDIR,
        &functionTIME,
        &functionTYPE,
        &functionIPCONFIG,
        &functionCOLOR,
        &functionSTART
};

// ========================================== SHELL SETTINGS ==========================================

char *readLine() {
    int bufferSize = LINE_BUFFER;
    int pos = 0;
    char *buffer = malloc(sizeof(char) * bufferSize);
    int c;

    if (!buffer) {
        fprintf(stderr, "Unable to allocate!\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();
        c = tolower(c);

        if (c == EOF || c == '\n') {
            buffer[pos] = '\0';
            return buffer;
        }else {
            buffer[pos] = (char) c;
        }
        pos++;

        if (pos >= bufferSize) {
            bufferSize += LINE_BUFFER;
            buffer = realloc(buffer, (size_t) bufferSize);
            if (!buffer) {
                fprintf(stderr, "Unable to allocate!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **tokenize(char *userIn) {
    int bufferSize = TOKEN_BUFFER;
    int pos = 0;
    char **tokens = malloc(sizeof(char*) * bufferSize);
    char *token;

    if (!tokens) {
        fprintf(stderr, "Unable to allocate!\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(userIn, TOKEN_DELIMITERS_WS);
//    token = strtok(userIn, " ");
    while (token != NULL) {
        tokens[pos] = token;
        pos++;

        if (pos >= bufferSize) {
            bufferSize += TOKEN_BUFFER;
            tokens = realloc(tokens, sizeof(char*) * bufferSize);
            if (!tokens) {
                fprintf(stderr, "Unable to reallocate!\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, TOKEN_DELIMITERS_WS);
    }
    tokens[pos] = NULL;
    return tokens;
}

int execute(TCHAR *args[]) {
    if (args[0] == NULL) {
        return 1;
    }
    int i;
    for (i = 0; i < COMMANDS_SIZE; i++) {
        if (strcmp(args[0], "quit") == 0) {
            args[0] = "exit";
        }
        if (strcmp(args[0], "cd..") == 0) {
            args[0] = "cd";
            args[1] = "..";
        }
        if (strcmp(args[0], "cd\\") == 0) {
            args[0] = "cd";
            args[1] = "\\";
        }
        if (strcmp(args[0], commands[i]) == 0) {
            return (*pointers[i])(args);
        }
    }
    printf("'%s' is not recognized as an internal or external command,\noperable program or batch file.\n", args[0]);
    return 1;
}

void mainLoop() {
    char *userIn;
    char **args;
    int continueRun;

//    rl_bind_key('\t', rl_complete);

    printf("SHELL [Version 1.0]\n(c) 2018 John Caesar Patac. All rights reserved.\n");
    do {
        GetCurrentDirectory(BUFFER_SIZE, CurrentDirBuffer);
        printf("\n%s>", CurrentDirBuffer);
        userIn = readLine();
        args = tokenize(userIn);
        continueRun = execute(args);

        free(userIn);
        free(args);

    }while (continueRun != 0);
}

// ========================================== MAIN ==========================================

int main() {
    do {
        reset = 0;
        mainLoop();
        if (reset == 0) {
            return EXIT_SUCCESS;
        }
    }while (reset == 1);
    return 0;
}