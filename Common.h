// Этот класс необходим для общих операций: 
// загрузка файлов, обращение к ОС компа и
// вообще всего, что не хочется выделять в отдельный класс.
// В моём случае это выбор файла БД и его регистрация в реестре Windows
#pragma once

#include <unordered_set>
#include <functional>
#include <aced.h>
#include <Windows.h>
// In some use I need to have my "own" type
// for which I import this library
#include <cstdint>
// For alerts I use acedAlert
#include <acedads.h>
#include "CartogramDb.h"

uint32_t wcslen32(const wchar_t* str);

const double COORD_TOLERANCE = 1e-5;

class Common
{
private:
	
public:
	// Функция предназначена для выбора файла базы данных SQLite
	// с помощью диалогового окна. После выбора путь к БД сохраняется в реестре Windows
	static bool selectDbFile(wchar_t* outPath, int bufferSize);

	static void DbAlert();

	// А эта и следующая структура не дают ввести в БД одинаковые запросы насколько раз подряд
	// за сеанс. Нужно это в ситуациях, когда полилиния лежит в одном квадрате, следовательно
	// из-за особенности алгоритма может быть несколько одинаковых запросов в БД.
	// Чтобы этого не допусть, дубликаты исключаются. Сначала все точки округляются...
	struct PointHash {
		size_t operator()(const std::pair<double, double>& p) const {
			auto h1 = std::hash<double>{}(std::round(p.first / COORD_TOLERANCE));
			auto h2 = std::hash<double>{}(std::round(p.second / COORD_TOLERANCE));
			return h1 ^ (h2 << 1);
		}
	};
	// затем находятся дубликаты
	struct PointEqual {
		bool operator()(const std::pair<double, double>& a, const std::pair<double, double>& b) const {
			return std::fabs(a.first - b.first) < COORD_TOLERANCE &&
				std::fabs(a.second - b.second) < COORD_TOLERANCE;
		}
	};
	

};

