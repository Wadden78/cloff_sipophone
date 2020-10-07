#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <tchar.h>
#include <process.h>
#include <delayimp.h>
#include <stdlib.H>
#include <signal.h>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <mmsystem.h>
#include "Config.h"
#include "Main.h"

using namespace std;

const int ci_HistoryHeight = ci_MainHeight;	/**< Высота окна по умолчанию*/
const int ci_HistoryWidth = 348;			/** Ширина окна по умолчанию*/
const int ci_HistoryLTHeaderWidth = 24;		/** Ширина колонки с пиктограммой типа вызова*/
const int ci_HistoryNumHeaderWidth = 190;	/** Ширина колонки с номером абонента*/

#define ID_HISTBUTTON_ALL	100
#define ID_HISTBUTTON_OUT	101
#define ID_HISTBUTTON_IN	102
#define ID_HISTBUTTON_MISS	103
#define ID_HISTLISTV_ALL	110
#define ID_HISTLISTV_OUT	111
#define ID_HISTLISTV_IN		112
#define ID_HISTLISTV_MISS	113

//LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//bool CreateHistory();

enum class enListType {enLTIn, enLTOut, enLTMiss };

class CHistory
{
public:
	CHistory();
	~CHistory();

	/**
	 * Показать окно.
	 * 
	 */
	void _Show() { ShowWindow(m_hHistoryWnd, SW_SHOWNORMAL); }
	/**
	 * Скрыть окно.
	 * 
	 */
	void _Hide() { ShowWindow(m_hHistoryWnd, SW_HIDE); }
	/**
	 * получить список вызовов.
	 * 
	 * \param vList	- ссылка на вектор со строками
	 * \param enLT			- тип вызова вх/исх/пропущенный
	 * \return  - число строк в списке
	 */
	size_t _GetCallList(vector<wstring>& vList, enListType enLT);
	/**
	 * Добавить вызов в список.
	 * 
	 * \param wstrCallNum	- номер вызова
	 * \param tCallTime		- время вызова
	 * \param enLT			- тип вызова вх/исх/пропущенный
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, const time_t tCallTime, enListType enLT);
	/**
	 * Добавить вызов в список.
	 * 
	 * \param wstrCallNum	- номер вызова со временем вызова
	 * \param enLT			- тип вызова вх/исх/пропущенный
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, enListType enLT);
	/**
	 * Сохранить параметры в файл конфигурации.
	 * 
	 * \param pCfg - ссылка на объект работы с файлом конфигурации
	 */
	void _SaveParameters(unique_ptr<CConfigFile>& pCfg);
	/**
	 * Получить параметры из файла конфигурации.
	 *
	 * \param pCfg - ссылка на объект работы с файлом конфигурации
	 */
	void _LoadParameters(unique_ptr<CConfigFile>& pCfg);
	void _ListViewSelect(const int iListID);
	void _ListViewClear(bool bClearAll=false);
	void _ListViewCopy2Clipboard();
	//void _SetCurrentSelNumber(const wchar_t*);
private:

	HWND m_hHistoryWnd{ 0 };
	HWND m_hAllListWnd{ 0 };
	HWND m_hOutListWnd{ 0 };
	HWND m_hInListWnd{ 0 };
	HWND m_hMissListWnd{ 0 };
	HWND m_hwndButtonAll{ 0 };
	HWND m_hwndButtonIn{ 0 };
	HWND m_hwndButtonOut{ 0 };
	HWND m_hwndButtonMiss{ 0 };
	HIMAGELIST m_hImageList{ 0 };
	HBITMAP m_hbmALL_Act{ 0 };
	HBITMAP m_hbmALL_Back{ 0 };
	HBITMAP m_hbmOUT_Act{ 0 };
	HBITMAP m_hbmOUT_Back{ 0 };
	HBITMAP m_hbmIN_Act{ 0 };
	HBITMAP m_hbmIN_Back{ 0 };
	HBITMAP m_hbmMISS_Act{ 0 };
	HBITMAP m_hbmMISS_Back{ 0 };

	bool CreateHistory();
	static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	size_t List2Wstring(HWND& hwndList, wstring& wstrRes);
	void GetCallInfoList(HWND& hwndList, const wchar_t* wszSource, enListType enLT);
	void SetMainComboNumber(HWND& hLV);
	HWND CreateListView(HWND hwndParent, const int iLVCodeID);
};
