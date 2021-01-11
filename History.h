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

const int ci_HistoryHeight				= ci_MainHeight;	/**< ������ ���� �� ���������*/
const int ci_HistoryLTHeaderWidth		= 24;				/**< ������ ������� � ������������ ���� ������*/
const int ci_HistoryNumHeaderWidth		= 190;				/**< ������ ������� � ������� ��������*/
const int ci_HistoryTimeHeaderWidth		= 90;				/**< ������ ������� � �������� ������*/
const int ci_HistoryDateHeaderWidth		= 90;				/**< ������ ������� � ����� ������*/
const int ci_HistoryDurationHeaderWidth = 174;				/**< ������ ������� � ������������� ������*/
const int ci_HistoryWidth = ci_HistoryLTHeaderWidth + ci_HistoryNumHeaderWidth + ci_HistoryTimeHeaderWidth + ci_HistoryDateHeaderWidth + ci_HistoryDurationHeaderWidth + 4*5;			/** ������ ���� �� ���������*/

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
	 * \brief ��������� ������ ������ ��� �������� � �������� ������.
	 */
	struct SHistoryRecord
	{
		enListType enCallType{ -1 };	/**< ��� ������ ��/���/����*/
		wstring wstrCallNumber;			/**< ����� �������� ������ �������*/
		wstring wstrCallBeginTime;		/**< ����� ������ ������*/
		wstring wstrCallDuration;		/**< ������������ ������*/
	};
}

class CHistory
{
public:
	CHistory();
	~CHistory();
	/**
	 * ������ ������.
	 *
	 * \return true - ����� �������/ false - ������ ������� ������
	 */
	bool _Start(void);
	/**
	 *  ������� ������.
	 *
	 * \param time_out - ������� �������� ���������� ������
	 * \return true - ����� ���������� ���������/ false - ����� ���������� �������������
	 */
	bool _Stop(int time_out);
	/**
	 * ������� ���������� ��������� � ������� �����������.
	 *
	 * \param pMsg - ������ �� ���������
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
	/**
	 * �������� ����.
	 *
	 */
	void _Show() 
	{ 
		SetForegroundWindow(m_hHistoryWnd);
		BringWindowToTop(m_hHistoryWnd);
		ShowWindow(m_hHistoryWnd, SW_SHOWNORMAL);
	}
	/**
	 * ������ ����.
	 *
	 */
	void _Hide() { ShowWindow(m_hHistoryWnd, SW_HIDE); }
	/**
	 * \brief ��������� ��������� � ���� ������������.
	 *
	 */
	void _SaveParameters();
	/**
	 * �������� ������ �������.
	 *
	 * \param vList	- ������ �� ������ �� ��������
	 * \param enLT			- ��� ������ ��/���/�����������
	 * \return  - ����� ����� � ������
	 */
	size_t _GetCallList(vector<wstring>& vList, enListType enLT);
	/**
	 * �������� ����� � ������.
	 *
	 * \param wstrCallNum	- ����� ������
	 * \param wstrCallTime	- ����� ������
	 * \param enLT			- ��� ������ ��/���/�����������
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, const wchar_t* wstrCallTime, const wchar_t* wstrCallDate, const wchar_t* wstrCallDur, enListType enLT);
	/**
	 * �������� ����� � ������.
	 *
	 * \param wstrCallNum	- ����� ������
	 * \param tCallTime		- ����� ������
	 * \param enLT			- ��� ������ ��/���/�����������
	 */
	void _AddToCallList(const wchar_t* wstrCallNum, const time_t tCallTime, int iCallDuration, int iAnswerDuration, enListType enLT);
	/**
	 * �������� ��������� �� ����� ������������ ��� ��������� �� ��.
	 *
	 */
	void _LoadParameters();
	void _ListViewSelect(const int iListID);
	void _ListViewClear(bool bClearAll = false);
	void _ListViewCopy2Clipboard();
	//void _SetCurrentSelNumber(const wchar_t*);
	/**
	 * \brief ��������� ����� ������.
	 */
	void _SetButtonStyle(LPDRAWITEMSTRUCT& pDIS);
	void _Resize();
	void _ColumnResize();
	bool _IsEmptyList(HWND hListWnd);
private:
	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< ������� ��� ���������� ������ */
	DWORD m_ThreadID = 0;						/**<  ID ������ � ����� ������ ������������ ������� */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE ������ */
	/**
	 * ������� ������.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief ������� ��������� ����� ������
		\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief ������� ��������� ������ ��� ����������
	\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief ������� ���� ������
	*/
	void LifeLoopOuter();
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief ����������� �����

	������� ������������� ��� ��������� ������ ������, ���������� ���������� ���� ������������� �����.

	\param[out] buffer ����� ���� ����� ������� ���������
	\param[in] eaddr
	\param[in] FrameAddr
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
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
