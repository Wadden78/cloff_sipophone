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
#include "windowsx.h"
#include <rapidjson/document.h>
#include "rapidjson/filereadstream.h"
#include "id_list.h"

using namespace std;

const int ci_HistoryHeight				= ci_MainHeight;	/**< Высота окна по умолчанию*/
const int ci_HistoryLTHeaderWidth		= 24;				/**< Ширина колонки с пиктограммой типа вызова*/
const int ci_HistoryNumHeaderWidth		= 190;				/**< Ширина колонки с номером абонента*/
const int ci_HistoryTimeHeaderWidth		= 90;				/**< Ширина колонки с временем вызова*/
const int ci_HistoryDateHeaderWidth		= 90;				/**< Ширина колонки с датой вызова*/
const int ci_HistoryDurationHeaderWidth = 174;				/**< Ширина колонки с длительностью вызова*/
const int ci_HistoryWidth = ci_HistoryLTHeaderWidth + ci_HistoryNumHeaderWidth + ci_HistoryTimeHeaderWidth + ci_HistoryDateHeaderWidth + ci_HistoryDurationHeaderWidth + 4*5;			/** Ширина окна по умолчанию*/

#define HISTORY_OBJ_NAME  "history"
#define NUM_HD_WIDTH_NAME "num_header_w"
#define TIM_HD_WIDTH_NAME "time_header_w"
#define DAT_HD_WIDTH_NAME "date_header_w"
#define DUR_HD_WIDTH_NAME "dur_header_w"
#define INC_ARRAY_NAME	  "IN"
#define OUT_ARRAY_NAME	  "OUT"
#define MIS_ARRAY_NAME	  "MISS"
#define NUM_FLD_NAME	  "number"
#define TIM_FLD_NAME	  "time"
#define DAT_FLD_NAME	  "date"
#define DUR_FLD_NAME	  "dur"

namespace nsHistory
{
	/**
	 * \brief Структура записи вызова для передачи в записную книжку.
	 */
	struct SHistoryRecord
	{
		enListType enCallType{ -1 };	/**< тип вызова вх/исх/проп*/
		wstring wstrCallNumber;			/**< номер телефона другой стороны*/
		wstring wstrCallBeginTime;		/**< время начала вызова*/
		wstring wstrCallDuration;		/**< длительность вызова*/
	};
}

class CHistory
{
public:
	CHistory();
	~CHistory();
	/**
	 * Запуск потока.
	 *
	 * \return true - поток запущен/ false - ошибка запуска потока
	 */
	bool _Start(void);
	/**
	 *  Останов потока.
	 *
	 * \param time_out - таймаут ожидания завершения потока
	 * \return true - поток остановлен нормально/ false - поток остановлен принудительно
	 */
	bool _Stop(int time_out);
	/**
	 * Функция отправляет сообщение в очередь обработчика.
	 *
	 * \param pMsg - ссылка на сообщение
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
	/**
	 * Показать окно.
	 *
	 */
	void _Show() 
	{ 
		SetForegroundWindow(m_hHistoryWnd);
		BringWindowToTop(m_hHistoryWnd);
		ShowWindow(m_hHistoryWnd, SW_SHOWNORMAL);
	}
	/**
	 * Скрыть окно.
	 *
	 */
	void _Hide() { ShowWindow(m_hHistoryWnd, SW_HIDE); }
	/**
	 * \brief Сохранить параметры в файл конфигурации.
	 *
	 */
	void _SaveParameters();
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
	 * \param wstrCallTime	- время вызова
	 * \param enLT			- тип вызова вх/исх/пропущенный
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, const wchar_t* wstrCallTime, const wchar_t* wstrCallDate, const wchar_t* wstrCallDur, enListType enLT);
	/**
	 * Добавить вызов в список.
	 *
	 * \param wstrCallNum	- номер вызова
	 * \param tCallTime		- время вызова
	 * \param enLT			- тип вызова вх/исх/пропущенный
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, const time_t tCallTime, int iCallDuration, int iAnswerDuration, enListType enLT);
	/**
	 * Получить параметры из файла конфигурации или загрузить из БД.
	 *
	 */
	void _LoadParameters();
	void _ListViewSelect(const int iListID);
	void _ListViewClear(bool bClearAll = false);
	void _ListViewCopy2Clipboard();
	//void _SetCurrentSelNumber(const wchar_t*);
	/**
	 * \brief Изменение стиля кнопок.
	 */
	void _SetButtonStyle(LPDRAWITEMSTRUCT& pDIS);
	void _Resize();
	void _ColumnResize();
	bool _IsEmptyList(HWND hListWnd);
private:
	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< Событие для завершения потока */
	DWORD m_ThreadID = 0;						/**<  ID потока с точки зрения операционной системы */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE потока */
	/**
	 * Функция потока.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief Функция основного цикла потока
		\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief Функция обработки ошибки при исключении
	\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief Внешний цикл потока
	*/
	void LifeLoopOuter();
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief Трассировка стека

	Функция предназначена для получения адреса модуля, вызвавшего исключение путём раскручивания стека.

	\param[out] buffer Буфер куда будет помещён результат
	\param[in] eaddr
	\param[in] FrameAddr
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	template<std::size_t SIZE> wchar_t* StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr);
	DWORD GetTickCountDifference(DWORD nPrevTick);

	bool m_bCloffClient{ false };
	enListType m_ActiveList{ enListType::enLTAll };

	HWND m_hHistoryWnd{ 0 };
	HWND m_hAllListWnd{ 0 };
	HWND m_hOutListWnd{ 0 };
	HWND m_hInListWnd{ 0 };
	HWND m_hMissListWnd{ 0 };
	HWND m_hwndButtonAll{ 0 };
	HWND m_hwndButtonIn{ 0 };
	HWND m_hwndButtonOut{ 0 };
	HWND m_hwndButtonMiss{ 0 };
	HWND m_hwndProgressBar{ 0 };
	HIMAGELIST m_hImageList{ 0 };
	HBITMAP m_hbmALL_Act{ 0 };
	HBITMAP m_hbmALL_GR{ 0 };
	HBITMAP m_hbmALL_HVR{ 0 };
	HBITMAP m_hbmOUT_Act{ 0 };
	HBITMAP m_hbmOUT_GR{ 0 };
	HBITMAP m_hbmOUT_HVR{ 0 };
	HBITMAP m_hbmIN_Act{ 0 };
	HBITMAP m_hbmIN_GR{ 0 };
	HBITMAP m_hbmIN_HVR{ 0 };
	HBITMAP m_hbmMISS_Act{ 0 };
	HBITMAP m_hbmMISS_GR{ 0 };
	HBITMAP m_hbmMISS_HVR{ 0 };

	static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ListSave(enListType enT, rapidjson::Document& doc);
	void SetMainComboNumber(HWND& hLV);
	HWND CreateListView(HWND hwndParent, const int iLVCodeID, const int iNumHDWidth, const int iTimeHDWidth, const int iDateHDWidth, const int iDurHDWidth);
	void HTTPParser(const char* pszText);
	void DrawButton(HWND hCurrentButton, HBITMAP hBitmap);
	void ParseJSONArray(enListType enT, rapidjson::Value::Object& list_obj);
};
