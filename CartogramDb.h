#pragma once
#include "Common.h"
#include <vector>
#include <string>
#include <sqlite3.h>
#include <tchar.h>
#include <windows.h>




class CartogramDb {
private:
    sqlite3* db;
    wchar_t dbPath[MAX_PATH] = { 0 };  // Changed to wide char and dynamic path

public:



    // Structure to hold map data
    struct Map {
        wchar_t name[256] = { 0 };  // Array to store wide character string
        double x1, y1 = 0;  // Changed from float to double
        double x2, y2 = 0;
        double x3, y3 = 0;
        double x4, y4 = 0;
    };

    // Constructor/Destructor
    CartogramDb();
    ~CartogramDb();

    // Database operations
    bool setDatabasePath(const wchar_t* path);
    bool openDatabase();
    void closeDatabase();
    Map getMapData(double x2_filter = -1, double y2_filter = -1);

    // Path persistence
    static void saveDbPathToRegistry(const wchar_t* path);
    //static bool loadDbPathFromRegistry(wchar_t* path, size_t bufferSize);
    static bool loadDbPathFromRegistry(wchar_t* path, DWORD bufferSize);
    static void SET_DB_PATH();
};