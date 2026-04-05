#include "stdafx.h"
#include "Common.h"

uint32_t wcslen32(const wchar_t* str) {
    uint32_t len = 0;
    while (str[len] != L'\0') ++len;
    return len;
}
bool Common::selectDbFile(wchar_t* outPath, int bufferSize) {
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = adsw_acadMainWnd();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"SQLite Database (*.db)\0*.db\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Выберите базу данных";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        wcsncpy_s(outPath, bufferSize, szFile, _TRUNCATE);
        return true;
    }
    return false;
}

void Common::DbAlert()
{
    int result = MessageBoxW(
        adsw_acadMainWnd(),
        L"Возможны проблемы с базой данных",
        L"Database Warning",
        MB_ICONERROR | MB_OK | MB_DEFBUTTON1
    );

    if (result == IDOK) {
        CartogramDb::SET_DB_PATH();
    }
}