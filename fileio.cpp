#include "stdafx.h"
#include "fileio.h"

__int64 FileSize(TCHAR *name)
{
    HANDLE hFile = CreateFile(name, GENERIC_READ, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile==INVALID_HANDLE_VALUE)
        return -1; 

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size))
    {
        CloseHandle(hFile);
        return -1; 
    }

    CloseHandle(hFile);
    return size.QuadPart;
}
