#include "stdafx.h"
#include <dbents.h>
#include <dbsymtb.h>
#include <dbmtext.h>
#include <map>
#include <fstream>
#include <sstream>
#include <tchar.h>
#include <codecvt>
#include "AutoTextReplacer.h"

#include <rxregsvc.h>
#include <aced.h>
#include "acdb.h"

std::map<AcString, AcString> AutoTextReplacer::readReplacementPairs(const AcString& filePath) {
    std::map<AcString, AcString> replacements;

    // Open file with UTF-8 encoding
    std::wifstream file(filePath.constPtr());

    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));

    if (!file.is_open()) {
        acutPrintf(L"\nFailed to open file: %s", filePath.constPtr());
        return replacements;
    }

    std::wstring line;
    AcString key, value;
    bool isKeyLine = true;

    while (std::getline(file, line)) {
        // Trim whitespace and CR/LF
        line.erase(line.find_last_not_of(L"\r\n") + 1);

        if (isKeyLine) {
            key = line.c_str();
            isKeyLine = false;
        }
        else {
            value = line.c_str();
            replacements[key] = value;
            isKeyLine = true;
        }
    }

    file.close();
    return replacements;
}


void AutoTextReplacer::replaceAllTextStrings(AcString filePath)
{
    // Read replacement pairs from file
    std::map<AcString, AcString> replacements = readReplacementPairs(filePath);

    if (replacements.empty())
    {
        acutPrintf(L"\nNo valid replacement pairs found in file.");
        return;
    }

    // Log the replacements being used
    acutPrintf(L"\nLoaded %d replacement pairs:", replacements.size());
    for (const auto& pair : replacements)
    {
        acutPrintf(L"\n\"%s\" → \"%s\"", pair.first.constPtr(), pair.second.constPtr());
    }

    // Process all entities in the drawing
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* pBlockTable;
    if (pDb->getBlockTable(pBlockTable, AcDb::kForRead) != Acad::eOk)
    {
        acutPrintf(L"\nFailed to access block table.");
        return;
    }

    int modifiedMTextCount = 0;
    int modifiedTextCount = 0;

    // Iterate through all block table records (model space and all layouts)
    AcDbBlockTableIterator* pBlockIterator;
    if (pBlockTable->newIterator(pBlockIterator) != Acad::eOk)
    {
        pBlockTable->close();
        acutPrintf(L"\nFailed to create block table iterator.");
        return;
    }

    for (pBlockIterator->start(); !pBlockIterator->done(); pBlockIterator->step())
    {
        AcDbBlockTableRecord* pBlockTableRecord;
        if (pBlockIterator->getRecord(pBlockTableRecord, AcDb::kForRead) != Acad::eOk)
        {
            continue;
        }

        // Process each block table record (model space and layouts)
        AcDbBlockTableRecordIterator* pEntityIterator;
        if (pBlockTableRecord->newIterator(pEntityIterator) != Acad::eOk)
        {
            pBlockTableRecord->close();
            continue;
        }

        for (pEntityIterator->start(); !pEntityIterator->done(); pEntityIterator->step())
        {
            AcDbEntity* pEnt;
            if (pEntityIterator->getEntity(pEnt, AcDb::kForRead) == Acad::eOk)
            {
                // Handle MText entities
                if (pEnt->isKindOf(AcDbMText::desc()))
                {
                    AcDbMText* pMText = AcDbMText::cast(pEnt);
                    AcString contents = pMText->contents();
                    bool wasModified = false;

                    // Apply all replacements
                    for (const auto& replacement : replacements)
                    {
                        if (contents.find(replacement.first) != -1)
                        {
                            contents.replace(replacement.first, replacement.second);
                            wasModified = true;
                        }
                    }

                    if (wasModified)
                    {
                        if (pMText->upgradeOpen() == Acad::eOk)
                        {
                            pMText->setContents(contents);
                            modifiedMTextCount++;
                        }
                    }
                }
                // Handle regular Text entities
                else if (pEnt->isKindOf(AcDbText::desc()))
                {
                    AcDbText* pText = AcDbText::cast(pEnt);
                    AcString textString = pText->textString();
                    bool wasModified = false;

                    // Apply all replacements
                    for (const auto& replacement : replacements)
                    {
                        if (textString.find(replacement.first) != -1)
                        {
                            textString.replace(replacement.first, replacement.second);
                            wasModified = true;
                        }
                    }

                    if (wasModified)
                    {
                        if (pText->upgradeOpen() == Acad::eOk)
                        {
                            pText->setTextString(textString);
                            modifiedTextCount++;
                        }
                    }
                }
                pEnt->close();
            }
        }

        delete pEntityIterator;
        pBlockTableRecord->close();
    }

    delete pBlockIterator;
    pBlockTable->close();

    acutPrintf(L"\n\nModified %d MText entities and %d Text entities across all spaces.", modifiedMTextCount, modifiedTextCount);
}

const ACHAR* AutoTextReplacer::FindTxtFile() {
    const ACHAR* FName;
    AcDbDatabase* DB = acdbHostApplicationServices()->workingDatabase();

    if (DB->getFilename(FName) != Acad::eOk) {
        acutPrintf(L"\nFailed to get drawing file path.");
        return L"";
    }

    std::wstring path(FName);
    size_t pos = path.find_last_of(L".");
    if (pos != std::wstring::npos) {
        path.replace(pos + 1, 3, L"txt");
    }

    // Return a persistent string (caution: memory leak potential)
    ACHAR* result = new ACHAR[path.size() + 1];
    wcscpy(result, path.c_str());
    return result;

}