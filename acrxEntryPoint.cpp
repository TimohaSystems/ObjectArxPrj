#include "StdAfx.h"
#include "resource.h"
#include "CartogramDb.h"
#include "Common.h"
#include "AutoTextReplacer.h"
#include <math.h>

// As the app creates only 2 entities I define only two here
#define MAP_TITLE L"Raster Name"
#define MAP_LINE L"Raster Line"



//-----------------------------------------------------------------------------
// I. e. Guskov Timothy Vyacheslavovich Code
#define szRDS _RXST("GTVC")
//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CTimonAcadToolsApp : public AcRxArxApp {

public:


	CTimonAcadToolsApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// TODO: Load dependencies here

		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;
		
		// TODO: Add your initialization code here

		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		// TODO: Unload dependencies here

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}

    static void GTVCTimonAcadTools_MAP_REQUEST()
    {

        // Prompt user to select a polyline
        ads_name ename;
        ads_point pt;
        int res = acedEntSel(_T("\nВыберите полилинию: "), ename, pt);

        if (res != RTNORM) {
            acutPrintf(_T("\nNo polyline selected or selection failed."));
            return;
        }

        // Convert ads_name to AcDbObjectId
        AcDbObjectId plineId;
        acdbGetObjectId(plineId, ename);

        // Verify the selected entity is a polyline
        AcDbEntity* pEnt = nullptr;
        if (acdbOpenObject(pEnt, plineId, AcDb::kForRead) != Acad::eOk) {
            acutPrintf(_T("\nFailed to open entity."));
            return;
        }

        if (!pEnt->isKindOf(AcDb2dPolyline::desc()) 
            && !pEnt->isKindOf(AcDbPolyline::desc())
            && !pEnt->isKindOf(AcDb3dPolyline::desc())) {
            acutPrintf(_T("\nВыбранный объект не является полилинией"));
            pEnt->close();
            return;
        }
        pEnt->close();

        // Now process the polyline using its ID
        std::vector<std::pair<double, double>> points = getVertexCoordinates(plineId);
        // This set is created to avoid repeated insertion of the square at the drawing
        std::unordered_set<std::pair<double, double>, Common::PointHash, Common::PointEqual> uniqueCoords;

        for (const auto& point : points) {
            double x = point.first - fmod(point.first, 250);
            double y = point.second - fmod(point.second, 250);
            // Check wether point already been used
            if (uniqueCoords.insert({ x, y }).second) {
                //If no -- DB class initialisation
                CartogramDb dbHandler;
                // Getting data from DB
                CartogramDb::Map map = dbHandler.getMapData(x, y);
                if (map.name != L"") {
                    // First create new layer for square
                    createNewLayer(MAP_LINE);
                    // Then draw the polyline at the place under the raster
                    createPolyline(map, MAP_LINE);
                    // Layer for raster title
                    createNewLayer(MAP_TITLE);
                    // Then insert the raster name
                    addTextToPoint(map, MAP_TITLE);
                }
                else {
                    acutPrintf(_T("\nНе найдено карт с этими координатами: (%.2f, %.2f)"), x, y);
                }
            }
        }
    }

    static std::vector<std::pair<double, double>> getVertexCoordinates(AcDbObjectId plineId) {
        std::vector<std::pair<double, double>> vertexCoordinates;

        // First try to open as generic entity
        AcDbEntity* pEnt = nullptr;
        if (acdbOpenObject(pEnt, plineId, AcDb::kForRead) != Acad::eOk) {
            acutPrintf(_T("\nFailed to open entity."));
            return vertexCoordinates;
        }

        // Handle different polyline types
        if (pEnt->isKindOf(AcDb2dPolyline::desc())) {
            AcDb2dPolyline* pPline = AcDb2dPolyline::cast(pEnt);
            if (pPline) {
                AcDbObjectIterator* pVertIter = pPline->vertexIterator();
                acutPrintf(_T("IT'S A 2D PLINE"));
                delete pVertIter;
            }
        }
        else if (pEnt->isKindOf(AcDbPolyline::desc())) {
            AcDbPolyline* pPline = AcDbPolyline::cast(pEnt);
            if (pPline) {
                for (unsigned int i = 0; i < pPline->numVerts(); i++) {
                    AcGePoint3d pt;
                    pPline->getPointAt(i, pt);
                    vertexCoordinates.emplace_back(pt.x, pt.y);
                }
            }
        }
        else if (pEnt->isKindOf(AcDb3dPolyline::desc())) {
            AcDb3dPolyline* pPline = AcDb3dPolyline::cast(pEnt);
            if (pPline) {
                AcDbObjectIterator* pVertIter = pPline->vertexIterator();
                // ... process 3D polyline vertices ...
                delete pVertIter;
            }
        }

        pEnt->close();
        return vertexCoordinates;
    }

    static void addTextToPoint(const CartogramDb::Map& mapData, const wchar_t* layerName)
    {

        double textHeight = 10;

        // Calculate text position (x2+125, y2+125)
        AcGePoint3d textPosition(mapData.x2 + 85, mapData.y2 + 125, 0.0);

        // Create and configure the text entity
        AcDbText* pText = new AcDbText(textPosition, mapData.name);
        pText->setHeight(textHeight);
        pText->setLayer(layerName);

        // Database operations
        AcDbBlockTable* pBlockTable;
        acdbHostApplicationServices() -> workingDatabase() -> getSymbolTable(pBlockTable, AcDb::kForRead);

        AcDbBlockTableRecord* pBlockTableRecord;
        pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForWrite);
        pBlockTable->close();

        // Add to drawing
        AcDbObjectId textId;
        pBlockTableRecord->appendAcDbEntity(textId, pText);
        pBlockTableRecord->close();
        pText->close();
    }
        
    static void createPolyline(const CartogramDb::Map& mapData, const wchar_t* layerName) // const CartogramDb::Map& mapData
    {
        // Set five vertex locations for the pline.
        //
        AcGePoint3dArray ptArr;
        ptArr.setLogicalLength(4);

        // Assign coordinates from Map to vertices (order: 1 → 2 → 3 → 4 → 1)
        ptArr[0].set(mapData.x1, mapData.y1, 0.0);  // First corner
        ptArr[1].set(mapData.x2, mapData.y2, 0.0);  // Second corner
        ptArr[2].set(mapData.x3, mapData.y3, 0.0);  // Third corner
        ptArr[3].set(mapData.x4, mapData.y4, 0.0);  // Fourth corner

        AcDb2dPolyline* pNewPline = new AcDb2dPolyline(
            AcDb::k2dSimplePoly, ptArr, 0.0, Adesk::kTrue);
        pNewPline->setColorIndex(7);
        // Get a pointer to a block table object.
        //
        AcDbBlockTable* pBlockTable;
        acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable, AcDb::kForRead);
        // Get a pointer to the MODEL_SPACE BlockTableRecord.
        //
        AcDbBlockTableRecord* pBlockTableRecord;
        pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord,
            AcDb::kForWrite);
        pBlockTable->close();
        AcDbObjectId plineObjId;
        pBlockTableRecord->appendAcDbEntity(plineObjId,
            pNewPline);
        pBlockTableRecord->close();
        pNewPline->setLayer(layerName);
        pNewPline->close();
    }

    static bool createNewLayer(const wchar_t* layerName) {
        AcDbLayerTable* pLayerTable;
        acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pLayerTable, AcDb::kForRead);

        // Check if layer already exists
        if (pLayerTable->has(layerName)) {
            pLayerTable->close();
            return false; // Layer exists
        }
        pLayerTable->close();

        // Reopen for write
        acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pLayerTable, AcDb::kForWrite);

        AcDbLayerTableRecord* pLayerRecord = new AcDbLayerTableRecord;
        pLayerRecord->setName(layerName);

        Acad::ErrorStatus es = pLayerTable->add(pLayerRecord);
        pLayerTable->close();

        if (es != Acad::eOk) {
            delete pLayerRecord;
            return false;
        }

        pLayerRecord->close();
        return true;
    }
    static void GTVCTimonAcadTools_STRREPLACE() {
        AutoTextReplacer ATRp;
        auto *path = ATRp.FindTxtFile();
        //acutPrintf(L"Функция в точке входа дала результат: ");
        acutPrintf(path);
        ATRp.replaceAllTextStrings(path);
    }

} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CTimonAcadToolsApp)


ACED_ARXCOMMAND_ENTRY_AUTO(CTimonAcadToolsApp, GTVCTimonAcadTools, _MAP_REQUEST, MAP_REQUEST, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTimonAcadToolsApp, GTVCTimonAcadTools, _STRREPLACE, STRREPLACE, ACRX_CMD_MODAL, NULL)
