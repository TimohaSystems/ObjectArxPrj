#include "stdafx.h"
#include <locale>
#include <acutads.h>
#include <codecvt>
#include <fstream>
#include <cassert>
#include "CartogramDb.h"


// Registry key constants
const wchar_t* REG_KEY_PATH = L"Software\\MapHandler";
const wchar_t* REG_VALUE_NAME = L"DatabasePath";

CartogramDb::CartogramDb() : db(nullptr) {

    // dbPath[0] = L'\0';
    // Try loading path from registry on construction
    if (!loadDbPathFromRegistry(dbPath, MAX_PATH)) {
        TCHAR response[2] = { 0 }; // Stores 'Y'/'N' + null terminator
        if (acedGetString(0, _T("\nNo database configured. Set path now? [Y/N]: "), response) == RTNORM) {
            if (towupper(response[0]) == 'Y') {
                SET_DB_PATH();
            }
        }
        return;
    }
    openDatabase();
}

CartogramDb::~CartogramDb() {
    closeDatabase();
}

// Improved registry handling
void CartogramDb::saveDbPathToRegistry(const wchar_t* path) {
    HKEY hKey;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER,
        REG_KEY_PATH,
        0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

    size_t requiredSize = (wcslen(path) + 1) * sizeof(wchar_t);
    assert(requiredSize <= MAXDWORD && "Path exceeds registry size limits");
    DWORD size = static_cast<DWORD>(requiredSize);

    RegSetValueExW(hKey, REG_VALUE_NAME, 0, REG_SZ,
        (const BYTE*)path, size);
    RegCloseKey(hKey);
}

bool CartogramDb::loadDbPathFromRegistry(wchar_t* path, DWORD bufferSize) {
    HKEY hKey;
    //DWORD size = static_cast<DWORD>(bufferSize * sizeof(wchar_t));
    DWORD size = bufferSize * sizeof(wchar_t);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        LONGLONG result = RegQueryValueExW(hKey, REG_VALUE_NAME, NULL, NULL, (LPBYTE)path, &size);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }
    return false;
}

bool CartogramDb::openDatabase() {
    if (dbPath[0] == L'\0') {
        acutPrintf(_T("\nDatabase path not set!"));
        return false;
    }

    // Convert to UTF-8 for SQLite
    char narrowPath[MAX_PATH] = { 0 };
    WideCharToMultiByte(CP_UTF8, 0, dbPath, -1,
        narrowPath, MAX_PATH, NULL, NULL);

    if (sqlite3_open_v2(narrowPath, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        acutPrintf(_T("\nFailed to open database: %s"), sqlite3_errmsg(db));
        //Common::DbAlert();
        return false;
    }
    return true;
}

void CartogramDb::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool CartogramDb::setDatabasePath(const wchar_t* path) {
    if (!path || wcslen(path) == 0) return false;

    if (wcsncpy_s(dbPath, path, _TRUNCATE) == 0) {
        saveDbPathToRegistry(path); // Persist the new path
        closeDatabase(); // Close existing connection if any
        return openDatabase(); // Reopen with new path
    }
    return false;
}

// For test and debugging DB work
static const wchar_t* charToWChar(const char* text) { 
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    static std::wstring wideStr;
    wideStr = converter.from_bytes(text);
    return wideStr.c_str();
}

CartogramDb::Map CartogramDb::getMapData(double x2_filter, double y2_filter) {
    // 
    Map map;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT square_name, x1, y1, x2, y2, x3, y3, x4, y4 FROM cartogram_squares WHERE ABS(x2 - ?) < 0.01 AND ABS(y2 - ?) < 0.01 LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        acutPrintf(L"\nFailed to prepare statement\n");
        Common::DbAlert();
        //acutPrintf(charToWChar(sql));
        //acutPrintf(_T("\nWhat's gotten for input (%.2f, %.2f)"), x2_filter, y2_filter);
        return map;
    }

    sqlite3_bind_double(stmt, 1, x2_filter);
    sqlite3_bind_double(stmt, 2, y2_filter);

    int stepResult = sqlite3_step(stmt);
    if (stepResult == SQLITE_ROW) {
        // Convert UTF-8 name to wchar_t
        const unsigned char* nameUtf8 = sqlite3_column_text(stmt, 0);
        if (nameUtf8) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::wstring wname = converter.from_bytes(reinterpret_cast<const char*>(nameUtf8));
            wcsncpy(map.name, wname.c_str(), sizeof(map.name) / sizeof(wchar_t) - 1);
            map.name[sizeof(map.name) / sizeof(wchar_t) - 1] = L'\0';  // Ensure null-termination
        }
        else {
            map.name[0] = L'\0';
        }

        // Assign coordinates
        map.x1 = sqlite3_column_double(stmt, 1);
        map.y1 = sqlite3_column_double(stmt, 2);
        map.x2 = sqlite3_column_double(stmt, 3);
        map.y2 = sqlite3_column_double(stmt, 4);
        map.x3 = sqlite3_column_double(stmt, 5);
        map.y3 = sqlite3_column_double(stmt, 6);
        map.x4 = sqlite3_column_double(stmt, 7);
        map.y4 = sqlite3_column_double(stmt, 8);
    }
    else if (stepResult != SQLITE_DONE) {
        acutPrintf(L"\nSQL step error: %hs", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    return map;
}

void CartogramDb::SET_DB_PATH()
{
    wchar_t chosenPath[MAX_PATH];
    if (Common::selectDbFile(chosenPath, MAX_PATH)) {
        CartogramDb db;
        if (db.setDatabasePath(chosenPath)) {
            acutPrintf(_T("\nDatabase path set to: %s"), chosenPath);
        }
        else {
            acutPrintf(_T("\nFailed to set database path!"));
        }
    }
    else {
        acutPrintf(_T("\nNo file selected."));
    }
}
