#include "History.h"
#include "resource.h"
#include <commctrl.h>
#include <time.h>
#include <sys/timeb.h>
#include <ws2tcpip.h>
#include "CCloffRequest.h"
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

CHistory::CHistory()
{
	m_hEvtStop = CreateEvent(NULL, FALSE, FALSE, NULL);

	int x{ 0 }, y{ 0 };
	int w{ ci_HistoryWidth }, h{ ci_HistoryHeight };
	int iNumHeaderWidth{ ci_HistoryNumHeaderWidth };		///< ширина поля номера
	int iTimeHeaderWidth{ ci_HistoryTimeHeaderWidth };		///< ширина поля времени вызова
	int iDateHeaderWidth{ ci_HistoryDateHeaderWidth };		///< ширина поля даты вызова
	int iDurHeaderWidth{ ci_HistoryDurationHeaderWidth };	///< ширина поля длительности вызова

	RECT rMainWindowRect{ 0 };
	GetWindowRect(hDlgPhoneWnd, &rMainWindowRect);
	x = rMainWindowRect.left;
	y = rMainWindowRect.top;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL) m_Log._LogWrite(L"   History: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
	else
	{
		fseek(fCfgFile, 0, SEEK_END);
		auto fSize = ftell(fCfgFile);
		fseek(fCfgFile, 0, SEEK_SET);
		vector<char> vConfig;
		vConfig.resize(fSize);

		rapidjson::FileReadStream is(fCfgFile, vConfig.data(), vConfig.size());

		rapidjson::Document cfg;
		cfg.ParseStream(is);
		if(!cfg.HasParseError() && cfg.IsObject())
		{
			if(cfg.HasMember(HISTORY_OBJ_NAME) && cfg[HISTORY_OBJ_NAME].IsObject())
			{
				auto hist = cfg[HISTORY_OBJ_NAME].GetObject();
				if(hist.HasMember(X_COORD_NAME) && hist[X_COORD_NAME].IsInt()) x = hist[X_COORD_NAME].GetInt();
				if(hist.HasMember(Y_COORD_NAME) && hist[Y_COORD_NAME].IsInt()) y = hist[Y_COORD_NAME].GetInt();
				if(hist.HasMember(W_WIDTH_NAME) && hist[W_WIDTH_NAME].IsInt()) w = hist[W_WIDTH_NAME].GetInt();
				if(hist.HasMember(H_HIGH_NAME) && hist[H_HIGH_NAME].IsInt()) h = hist[H_HIGH_NAME].GetInt();
				if(x < 0 ) x = rMainWindowRect.left;
				if(y < 0 ) y = rMainWindowRect.top;
				if(w < 10) w = ci_HistoryWidth;
				if(h < 10) h = ci_HistoryHeight;

				if(hist.HasMember(NUM_HD_WIDTH_NAME) && hist[NUM_HD_WIDTH_NAME].IsInt())  iNumHeaderWidth = hist[NUM_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(TIM_HD_WIDTH_NAME) && hist[TIM_HD_WIDTH_NAME].IsInt()) iTimeHeaderWidth = hist[TIM_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(DAT_HD_WIDTH_NAME) && hist[DAT_HD_WIDTH_NAME].IsInt()) iDateHeaderWidth = hist[DAT_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(DUR_HD_WIDTH_NAME) && hist[DUR_HD_WIDTH_NAME].IsInt())  iDurHeaderWidth = hist[DUR_HD_WIDTH_NAME].GetInt();
			}
		}
		else m_Log._LogWrite(L"   History: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
		::fclose(fCfgFile);
	}


// Register the window class.
	const wchar_t CLASS_NAME[] = L"History Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = HistoryWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(216, 216, 216)); //(HBRUSH)(COLOR_WINDOW + 1);

	RegisterClass(&wc);

	// Create the window.

	m_hHistoryWnd = CreateWindowEx(0, CLASS_NAME, L"История вызовов", WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU, x, y, w, h, 0, NULL, hInstance, this);

	m_hwndButtonAll = CreateWindow(WC_BUTTON, L"Все", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 2, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_ALL, hInstance, NULL);
	m_hbmALL_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ALL));
	m_hbmALL_GR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ALL_GR));
	m_hbmALL_HVR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ALL_HVR));

	m_hwndButtonOut = CreateWindow(WC_BUTTON, L"Out", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 84, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_OUT, hInstance, NULL);
	m_hbmOUT_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUTCALL));
	m_hbmOUT_GR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUT_GR));
	m_hbmOUT_HVR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUT_HVR));

	m_hwndButtonIn = CreateWindow(WC_BUTTON, L"In", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 166, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_IN, hInstance, NULL);
	m_hbmIN_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_INCALL));
	m_hbmIN_GR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_IN_GR));
	m_hbmIN_HVR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_IN_HVR));

	m_hwndButtonMiss = CreateWindow(WC_BUTTON, L"Miss", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 248, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_MISS, hInstance, NULL);
	m_hbmMISS_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISSCALL));
	m_hbmMISS_GR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISS_GR));
	m_hbmMISS_HVR = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISS_HVR));

	m_hImageList = ImageList_Create(24, 24, ILC_COLOR24, 3, 1);
	auto hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUT_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);
	hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_IN_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);
	hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISS_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);

	m_hAllListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_ALL, iNumHeaderWidth, iTimeHeaderWidth, iDateHeaderWidth, iDurHeaderWidth);
	ShowWindow(m_hAllListWnd, SW_SHOW);
	m_hOutListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_OUT, iNumHeaderWidth, iTimeHeaderWidth, iDateHeaderWidth, iDurHeaderWidth);
	ShowWindow(m_hOutListWnd, SW_HIDE);
	m_hInListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_IN, iNumHeaderWidth, iTimeHeaderWidth, iDateHeaderWidth, iDurHeaderWidth);
	ShowWindow(m_hInListWnd, SW_HIDE);
	m_hMissListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_MISS, iNumHeaderWidth, iTimeHeaderWidth, iDateHeaderWidth, iDurHeaderWidth);
	ShowWindow(m_hMissListWnd, SW_HIDE);

	ShowWindow(m_hHistoryWnd, SW_SHOWNORMAL);
	m_Log._LogWrite(L"   History: Constructor");
}
CHistory::~CHistory()
{
	ImageList_Destroy(m_hImageList);
	DestroyWindow(m_hHistoryWnd);
	m_hHistoryWnd = NULL;
	CloseHandle(m_hEvtStop);
	m_Log._LogWrite(L"   History: Destructor");
}
bool CHistory::_Start(void)
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	return m_hThread == NULL ? false : true;
}

bool CHistory::_Stop(int time_out)
{
	m_Log._LogWrite(L"   History: Stop");
	if(m_hThread != NULL)
	{
		SetEvent(m_hEvtStop);
		if(WaitForSingleObject(m_hThread, time_out) == WAIT_TIMEOUT) TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return true;
}
// Функция вывода сообщения о сбое
void CHistory::ErrorFilter(LPEXCEPTION_POINTERS ep)
{
	std::array<wchar_t, 1024> szBuf;
	swprintf_s(szBuf.data(), szBuf.size(), L"   History: ПЕРЕХВАЧЕНА ОШИБКА ПО АДРЕСУ 0x%p", ep->ExceptionRecord->ExceptionAddress);
	m_Log._LogWrite(szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"   History: %s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	m_Log._LogWrite(szBuf.data());
}

// Функция потока
unsigned __stdcall CHistory::s_ThreadProc(LPVOID lpParameter)
{
	((CHistory*)lpParameter)->LifeLoopOuter();
	return 0;
}

// Внешний цикл потока
void CHistory::LifeLoopOuter()
{
	int nCriticalErrorCounter = 0;
	int iExitCode = 0;
	unsigned long nTick;
	BOOL repeat = TRUE;
	std::array<wchar_t, 256> chMsg;
	while(repeat)
	{
		iExitCode = 0;
		__try
		{
			LifeLoop(nCriticalErrorCounter != 0);
			repeat = FALSE;
		}
		__except(ErrorFilter(GetExceptionInformation()), 1)
		{
			swprintf_s(chMsg.data(), chMsg.size(), L"   History: %S", "Last message buffer is empty!");
			m_Log._LogWrite(chMsg.data());
			if(nCriticalErrorCounter == 0) nTick = GetTickCount();
			nCriticalErrorCounter++;
			if(nCriticalErrorCounter > 4)
			{
				nCriticalErrorCounter = 0;
				nTick = GetTickCountDifference(nTick);
				if(nTick < 60000)
				{
					m_Log._LogWrite(L"   History: 5 СБОЕВ НА ПОТОКЕ В ТЕЧЕНИИ МИНУТЫ !!!!  ПОТОК ОСТАНОВЛЕН");
					repeat = FALSE;
					break;
				}
			}
			Sleep(1000);
			iExitCode = 1;
		}
	}
	_endthreadex(iExitCode);
}

DWORD CHistory::GetTickCountDifference(DWORD nPrevTick)
{
	auto nTick = GetTickCount64();
	if(nTick >= nPrevTick)	nTick -= nPrevTick;
	else					nTick += ULONG_MAX - nPrevTick;
	return (DWORD)nTick;
}

//Трассировка стека
template<std::size_t SIZE> wchar_t* CHistory::StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr)
{
	int pos = swprintf_s(buffer.data(), buffer.size(), L"   History: СТЕК: ");
	pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"0x%08X ", (DWORD)eaddr);
	__try
	{
		for(int i = 0; i < 16 && FrameAddr; i++)
		{
			pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"0x%08X ", *(DWORD*)(FrameAddr + 4));
			FrameAddr = *(DWORD*)(FrameAddr);
		}
	}
	__except(1)
	{
		pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"****");
		FrameAddr = 0;
	}
	if(FrameAddr) swprintf_s(&buffer[pos], buffer.size() - pos, L"...");

	return buffer.data();
}

void CHistory::LifeLoop(BOOL bRestart)
{
	vector<HANDLE> events;
	events.push_back(m_hEvtStop);

	m_Log._LogWrite(L"   History: LifeLoop");

	_LoadParameters();

	bool bRetry = true;
	while(bRetry)
	{
		DWORD evtno = WaitForMultipleObjects(events.size(), events.data(), FALSE, INFINITE);
		if(evtno == WAIT_TIMEOUT)	continue;
		if(evtno == WAIT_FAILED)
		{
			m_Log._LogWrite(L"   History: LifeLoop ERROR = %d", GetLastError());
			bRetry = false;
			break;
		}
		evtno -= WAIT_OBJECT_0;
		switch(evtno)
		{
			case 0:	//Stop
			{
				bRetry = false;
				m_Log._LogWrite(L"   History: Поток останавливается...");
				break;
			}
		}
	}
}

void CHistory::_AddToCallList(const wchar_t* wstrCallNum, const wchar_t* wstrCallTime, const wchar_t* wstrCallDate, const wchar_t* wstrCallDur, enListType enLT)
{
	LVITEM lvI;
	lvI.mask = LVIF_IMAGE | LVIF_TEXT;
	lvI.stateMask = 0;
	lvI.state = 0;
	lvI.iItem = 0;
	lvI.iImage = 0;

	lvI.iSubItem = 0;
	lvI.pszText = nullptr;
	// Insert items into the list.
	HWND* phwndList = &m_hOutListWnd;
	switch(enLT)
	{
		case enListType::enLTOut:	lvI.iImage = 0;	phwndList = &m_hOutListWnd;		break;
		case enListType::enLTIn:	lvI.iImage = 1;	phwndList = &m_hInListWnd;		break;
		case enListType::enLTMiss:	lvI.iImage = 2;	phwndList = &m_hMissListWnd;	break;
		case enListType::enLTAll:	lvI.iImage = 2;	phwndList = &m_hAllListWnd;	break;
	}
	auto iCount = SendMessage(*phwndList, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR && iCount == 1)
	{
		array<wchar_t, 256> wszText;
		ListView_GetItemText(*phwndList, 0, 1, wszText.data(), wszText.size());
		if(!wcscmp(wszText.data(), L"Записи не найдены ...")) ListView_DeleteItem(*phwndList, 0);
	}
	else if(iCount != LB_ERR && iCount >= ci_HistoryMAX) ListView_DeleteItem(*phwndList, iCount - 1);

	iCount = SendMessage(m_hAllListWnd, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR && iCount == 1)
	{
		array<wchar_t, 256> wszText;
		ListView_GetItemText(m_hAllListWnd, 0, 1, wszText.data(), wszText.size());
		if((wstrCallTime && wstrCallDate) && !wcscmp(wszText.data(), L"Записи не найдены ...")) ListView_DeleteItem(m_hAllListWnd, 0);
	}
	else if(iCount != LB_ERR && iCount >= ci_HistoryMAX * 2) ListView_DeleteItem(m_hAllListWnd, iCount - 1);

	ListView_InsertItem(*phwndList, &lvI);
	if(wstrCallTime && wstrCallDate) ListView_InsertItem(m_hAllListWnd, &lvI);

	ListView_SetItemText(*phwndList, 0, 1, (LPWSTR)wstrCallNum);
	if(wstrCallTime && wstrCallDate) ListView_SetItemText(m_hAllListWnd, 0, 1, (LPWSTR)wstrCallNum);
	ListView_SetItemText(*phwndList, 0, 2, (LPWSTR)wstrCallTime);
	if(wstrCallTime && wstrCallDate) ListView_SetItemText(m_hAllListWnd, 0, 2, (LPWSTR)wstrCallTime);
	ListView_SetItemText(*phwndList, 0, 3, (LPWSTR)wstrCallDate);
	if(wstrCallTime && wstrCallDate) ListView_SetItemText(m_hAllListWnd, 0, 3, (LPWSTR)wstrCallDate);
	ListView_SetItemText(*phwndList, 0, 4, (LPWSTR)wstrCallDur);
	if(wstrCallTime && wstrCallDate) ListView_SetItemText(m_hAllListWnd, 0, 4, (LPWSTR)wstrCallDur);

}

void CHistory::_AddToCallList(const wchar_t* wstrCallNum, const time_t tCallTime, int iCallDuration, int iAnswerDuration, enListType enLT)
{
	auto tCallBegin = *localtime(&tCallTime);
	array<wchar_t, 32> wszTime;
	swprintf_s(wszTime.data(), wszTime.size(), L"%02d:%02d:%02d", tCallBegin.tm_hour, tCallBegin.tm_min, tCallBegin.tm_sec);
	array<wchar_t, 32> wszDate;
	swprintf_s(wszDate.data(), wszDate.size(), L"%02d.%02d.%04d", tCallBegin.tm_mday, tCallBegin.tm_mon + 1, tCallBegin.tm_year + 1900);
	array<wchar_t, 256> wszDuration;
	swprintf_s(wszDuration.data(), wszDuration.size(), L"%dмин:%02dсек / %dмин:%02dсек", iCallDuration / 60, iCallDuration % 60, iAnswerDuration / 60, iAnswerDuration % 60);

	_AddToCallList(wstrCallNum, wszTime.data(), wszDate.data(), wszDuration.data(), enLT);
}

void CHistory::_ListViewSelect(const int iListID)
{
	ShowWindow(m_hAllListWnd, iListID == ID_HISTBUTTON_ALL ? SW_SHOW : SW_HIDE);
	ShowWindow(m_hOutListWnd, iListID == ID_HISTBUTTON_OUT ? SW_SHOW : SW_HIDE);
	ShowWindow(m_hInListWnd, iListID == ID_HISTBUTTON_IN ? SW_SHOW : SW_HIDE);
	ShowWindow(m_hMissListWnd, iListID == ID_HISTBUTTON_MISS ? SW_SHOW : SW_HIDE);
}

void CHistory::_ListViewClear(bool bClearAll)
{
	if(IsWindowVisible(m_hAllListWnd) || bClearAll) 
	{
		ListView_DeleteAllItems(m_hAllListWnd);
		_AddToCallList(L"Записи не найдены ...",nullptr, nullptr, nullptr,enListType::enLTAll);
	}
	if(IsWindowVisible(m_hOutListWnd) || bClearAll)
	{
		ListView_DeleteAllItems(m_hOutListWnd);
		_AddToCallList(L"Записи не найдены ...", nullptr, nullptr, nullptr, enListType::enLTOut);
	}
	if(IsWindowVisible(m_hInListWnd) || bClearAll) 
	{
		ListView_DeleteAllItems(m_hInListWnd);
		_AddToCallList(L"Записи не найдены ...", nullptr, nullptr, nullptr, enListType::enLTIn);
	}
	if(IsWindowVisible(m_hMissListWnd) || bClearAll) 
	{
		ListView_DeleteAllItems(m_hMissListWnd);
		_AddToCallList(L"Записи не найдены ...", nullptr, nullptr, nullptr, enListType::enLTMiss);
	}
}

void CHistory::_ListViewCopy2Clipboard()
{
	HWND hwndCurrentList = nullptr;
	if(IsWindowVisible(m_hAllListWnd))  hwndCurrentList = m_hAllListWnd;
	if(IsWindowVisible(m_hOutListWnd))  hwndCurrentList = m_hOutListWnd;
	if(IsWindowVisible(m_hInListWnd))  hwndCurrentList = m_hInListWnd;
	if(IsWindowVisible(m_hMissListWnd)) hwndCurrentList = m_hMissListWnd;

	if(hwndCurrentList)
	{
		int iPos = ListView_GetNextItem(hwndCurrentList, -1, LVNI_SELECTED);

		array<wchar_t, 1024> wsString;
		ListView_GetItemText(hwndCurrentList, iPos, 1, wsString.data(), wsString.size());

		LPTSTR  lptstrCopy;
		HGLOBAL hglbCopy;
		// Open the clipboard, and empty it. 
		if(!OpenClipboard(hDlgPhoneWnd)) return;
		EmptyClipboard();

		// Allocate a global memory object for the text. 
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (wcslen(wsString.data()) + 1) * sizeof(wchar_t));
		if(hglbCopy == NULL)
		{
			CloseClipboard();
			return;
		}
		// Lock the handle and copy the text to the buffer. 
		lptstrCopy = static_cast<LPTSTR>(GlobalLock(hglbCopy));
		if(lptstrCopy) memcpy(lptstrCopy, wsString.data(), (wcslen(wsString.data()) + 1) * sizeof(wchar_t));
		GlobalUnlock(hglbCopy);
		// Place the handle on the clipboard. 
		SetClipboardData(CF_UNICODETEXT, hglbCopy);

		CloseClipboard();
	}
}

void CHistory::SetMainComboNumber(HWND& hLV)
{
	auto iLine = SendMessage(hLV, LVM_GETSELECTEDCOUNT, 0, 0);
	array<wchar_t, 1024> wsString;
	ListView_GetItemText(hLV, iLine, 1, wsString.data(), wsString.size());
	SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
}
/*******************************************************/
HWND CHistory::CreateListView(HWND hwndParent, const int iLVCodeID, const int iNumHDWidth, const int iTimeHDWidth, const int iDateHDWidth, const int iDurHDWidth)
{
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.
	GetClientRect(hwndParent, &rcClient);
	// Create the list-view window in report view with label editing enabled.
	HWND hWndListView = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 2, 42, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 46, hwndParent, (HMENU)iLVCodeID, hInstance, NULL);

	auto hFnt = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	SendMessage(hWndListView, WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

	ListView_SetImageList(hWndListView, m_hImageList, LVSIL_SMALL);

	auto iListWiewWidth = rcClient.right - rcClient.left - 4;
	LVCOLUMN lvc;
	// Initialize the LVCOLUMN structure.
	// The mask specifies that the format, width, text,
	// and subitem members of the structure are valid.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_IMAGE | LVCF_SUBITEM;

	// Add the columns.
	lvc.iSubItem = 0;
	lvc.pszText = NULL;
	lvc.cx = ci_HistoryLTHeaderWidth; // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 0, &lvc);

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	// Add the columns.
	lvc.iSubItem = 1;
	lvc.pszText = (LPWSTR)L"Номер";
	lvc.cx = iNumHDWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 1, &lvc);
	// Add the columns.
	lvc.iSubItem = 2;
	lvc.pszText = (LPWSTR)L"Время";
	lvc.cx = iTimeHDWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 2, &lvc);
	// Add the columns.
	lvc.iSubItem = 3;
	lvc.pszText = (LPWSTR)L"Дата";
	lvc.cx = iDateHDWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 3, &lvc);
	// Add the columns.
	lvc.iSubItem = 4;
	lvc.pszText = (LPWSTR)L"Длительность";
	lvc.cx = iDurHDWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 4, &lvc);

	ListView_SetExtendedListViewStyleEx(hWndListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	return (hWndListView);
}

LRESULT CALLBACK CHistory::HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CHistory* pThis;

	if(uMsg == WM_NCCREATE)
	{
		pThis = static_cast<CHistory*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

		SetLastError(0);
		if(!SetWindowLongPtr(hWnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(pThis)) && GetLastError()) return -1;
	}
	else pThis = reinterpret_cast<CHistory*>(GetWindowLongPtr(hWnd, GWL_USERDATA));

	switch(uMsg)
	{
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case ID__MHL_COPY:			if(pThis) pThis->_ListViewCopy2Clipboard(); break;
				case ID__MHL_CLEAR:			if(pThis) pThis->_ListViewClear();			break;
				case ID__MHL_CLEAR_ALL:		if(pThis) pThis->_ListViewClear(true);		break;
				case ID__MHL_REFRESH_ALL:	if(pThis) pThis->_LoadParameters();			break;
				default:
				{
					switch(HIWORD(wParam))
					{
						case BN_CLICKED:
						{
							if(pThis) pThis->_ListViewSelect(LOWORD(wParam));
							break;
						}
						default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			switch(wParam)
			{
				case VK_F5:
				{
					if(pThis)
					{
						pThis->_LoadParameters();
						return true;
					}
					break;
				}
			}
			break;
		}
		case WM_SIZE:
		{
			if(pThis) pThis->_Resize();
			break;
		}
		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->code)
			{
				case LVN_KEYDOWN:
				{
					switch(((LPNMLVKEYDOWN)lParam)->wVKey)
					{
						case VK_F5:
						{
							if(pThis) pThis->_LoadParameters();
							break;
						}
						case VK_DELETE:
						{
							if(pThis) pThis->_ListViewClear();
							break;
						}
						case VK_SPACE:
						{
							POINT pt;
							GetCursorPos(&pt);
							auto hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_HLIST));
							hMenu = GetSubMenu(hMenu, 0);
							auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
							TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
							break;
						}
						case VK_RETURN:
						{
							auto lpnmitem = (LPNMITEMACTIVATE)lParam;
							LVHITTESTINFO lvh;
							lvh.pt.x = lpnmitem->ptAction.x;
							lvh.pt.y = lpnmitem->ptAction.y;
							ListView_SubItemHitTest(((LPNMHDR)lParam)->hwndFrom, &lvh);
							if(lvh.iItem != -1)
							{
								array<wchar_t, 1024> wsString;
								ListView_GetItemText(((LPNMHDR)lParam)->hwndFrom, lvh.iItem, 1, wsString.data(), wsString.size());

								auto pDog = wcschr(wsString.data(), L'@');
								if(pDog) *pDog = 0;

								SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
								SetForegroundWindow(hDlgPhoneWnd);
								BringWindowToTop(hDlgPhoneWnd);
								ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
								SendMessage(hBtDial, BM_CLICK, 0, 0);
							}
							break;
						}
					}
					break;
				}
				case NM_DBLCLK:
				{
					auto lpnmitem = (LPNMITEMACTIVATE)lParam;
					LVHITTESTINFO lvh;
					lvh.pt.x = lpnmitem->ptAction.x;
					lvh.pt.y = lpnmitem->ptAction.y;
					ListView_SubItemHitTest(((LPNMHDR)lParam)->hwndFrom, &lvh);
					if(lvh.iItem != -1)
					{
						array<wchar_t, 1024> wsString;
						ListView_GetItemText(((LPNMHDR)lParam)->hwndFrom, lvh.iItem, 1, wsString.data(), wsString.size());

						auto pDog = wcschr(wsString.data(), L'@');
						if(pDog) *pDog = 0;

						SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
						SetForegroundWindow(hDlgPhoneWnd);
						BringWindowToTop(hDlgPhoneWnd);
						ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
						SendMessage(hBtDial, BM_CLICK, 0, 0);
						//SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
						//SetForegroundWindow(hDlgPhoneWnd);
					}
					break;
				}
				case NM_RCLICK:
				{
					POINT pt;
					GetCursorPos(&pt);
					auto hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_HLIST));
					hMenu = GetSubMenu(hMenu, 0);
					auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
					break;
				}
				case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					switch(lplvcd->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:
							if(pThis) pThis->_ColumnResize();
							return CDRF_NOTIFYITEMDRAW;
							break;
						case CDDS_ITEMPREPAINT:
							if(((int)lplvcd->nmcd.dwItemSpec % 2) == 0)
							{
								lplvcd->clrText = RGB(0, 0, 0);
								lplvcd->clrTextBk = RGB(255, 255, 255);

								auto iCount = ListView_GetItemCount(((LPNMHDR)lParam)->hwndFrom);
								if(iCount != LB_ERR && iCount == 1)
								{
									array<wchar_t, 256> wszText;
									ListView_GetItemText(((LPNMHDR)lParam)->hwndFrom, 0, 1, wszText.data(), wszText.size());
									if(!wcscmp(wszText.data(), L"Записи не найдены ...")) lplvcd->clrText = RGB(200, 200, 200);
								}
							}
							else
							{
								lplvcd->clrText = RGB(0, 0, 0);
								lplvcd->clrTextBk = RGB(216, 216, 216);
							}
							return CDRF_NEWFONT;
							break;
							//There would be some bits here for subitem drawing but they don't seem neccesary as you seem to want a full row color only
						case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
							return CDRF_NEWFONT;
							break;
					}
					return TRUE;
					break;
				}
				default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
			if(pThis) pThis->_SetButtonStyle(pDIS);
			return TRUE;
		}
		case WM_CLOSE:
		{
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		case WM_DESTROY:
		{
			if(pThis) pThis->_SaveParameters();
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void CHistory::_SaveParameters()
{
	auto hPrevCursor = SetCursor(LoadCursor(hInstance, IDC_WAIT));

	rapidjson::Document cfg;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL)
	{
		m_Log._LogWrite(L"CFG: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
	}
	else
	{
		fseek(fCfgFile, 0, SEEK_END);
		auto fSize = ftell(fCfgFile);
		fseek(fCfgFile, 0, SEEK_SET);
		vector<char> vConfig;
		vConfig.resize(fSize);

		rapidjson::FileReadStream is(fCfgFile, vConfig.data(), vConfig.size());

		cfg.ParseStream(is);

		if(cfg.HasParseError())
		{
			m_Log._LogWrite(L"CFG: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
			cfg.Clear();
		}
		fclose(fCfgFile);
	}
	auto& allocator = cfg.GetAllocator();

	if(!cfg.IsObject()) cfg.SetObject();
	if(!cfg.HasMember(HISTORY_OBJ_NAME))
	{
		rapidjson::Value hist_obj(rapidjson::kObjectType);
		cfg.AddMember(HISTORY_OBJ_NAME, hist_obj, allocator);
	}

	RECT sCoord{ 0 };
	GetWindowRect(m_hHistoryWnd, &sCoord);

	rapidjson::Value vVal;

	if(!cfg[HISTORY_OBJ_NAME].HasMember(X_COORD_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(X_COORD_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][X_COORD_NAME].SetInt(sCoord.left);

	if(!cfg[HISTORY_OBJ_NAME].HasMember(Y_COORD_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(Y_COORD_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][Y_COORD_NAME].SetInt(sCoord.top);

	if(!cfg[HISTORY_OBJ_NAME].HasMember(W_WIDTH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(W_WIDTH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][W_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	if(!cfg[HISTORY_OBJ_NAME].HasMember(H_HIGH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(H_HIGH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][H_HIGH_NAME].SetInt(sCoord.bottom - sCoord.top);

	ListView_GetSubItemRect(m_hOutListWnd, 0, 1, LVIR_LABEL, &sCoord);
	if(!cfg[HISTORY_OBJ_NAME].HasMember(NUM_HD_WIDTH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(NUM_HD_WIDTH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][NUM_HD_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	ListView_GetSubItemRect(m_hOutListWnd, 0, 2, LVIR_LABEL, &sCoord);
	if(!cfg[HISTORY_OBJ_NAME].HasMember(TIM_HD_WIDTH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(TIM_HD_WIDTH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][TIM_HD_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	ListView_GetSubItemRect(m_hOutListWnd, 0, 3, LVIR_LABEL, &sCoord);
	if(!cfg[HISTORY_OBJ_NAME].HasMember(DAT_HD_WIDTH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(DAT_HD_WIDTH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][DAT_HD_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	ListView_GetSubItemRect(m_hOutListWnd, 0, 4, LVIR_LABEL, &sCoord);
	if(!cfg[HISTORY_OBJ_NAME].HasMember(DUR_HD_WIDTH_NAME)) cfg[HISTORY_OBJ_NAME].AddMember(DUR_HD_WIDTH_NAME, vVal, allocator);
	cfg[HISTORY_OBJ_NAME][DUR_HD_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	ListSave(enListType::enLTIn, cfg);
	ListSave(enListType::enLTOut, cfg);
	ListSave(enListType::enLTMiss, cfg);

	auto fFile = fopen("cloff_sip_phone.cfg", "w");
	if(fFile)
	{
		std::vector<char> writeBuffer;
		writeBuffer.resize(1048000);
		rapidjson::FileWriteStream os(fFile, writeBuffer.data(), writeBuffer.size());

		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

		cfg.Accept(writer);

		fclose(fFile);
	}
	else m_Log._LogWrite(L"   History: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);

	SetCursor(hPrevCursor);
}

void CHistory::ListSave(enListType enT, rapidjson::Document& doc)
{
	HWND hwndList{ m_hAllListWnd };
	const char* pListName{ nullptr };
	switch(enT)
	{
		case enListType::enLTIn:   hwndList = m_hInListWnd;   pListName = INC_ARRAY_NAME; break;
		case enListType::enLTOut:  hwndList = m_hOutListWnd;  pListName = OUT_ARRAY_NAME; break;
		case enListType::enLTMiss: hwndList = m_hMissListWnd; pListName = MIS_ARRAY_NAME; break;
	}

	if(!doc[HISTORY_OBJ_NAME].HasMember(pListName))
	{
		rapidjson::Value arr(rapidjson::kArrayType);
		doc[HISTORY_OBJ_NAME].AddMember(rapidjson::StringRef(pListName), arr, doc.GetAllocator());
	}
	else doc[HISTORY_OBJ_NAME][pListName].Clear();

	auto iCount = SendMessage(hwndList, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR)
	{
		for(; iCount; --iCount)
		{
			rapidjson::Value record(rapidjson::kObjectType);
			rapidjson::Value fld;

			array<wchar_t, 1024> wsString;
			array<char, 1024> sString;
			ListView_GetItemText(hwndList, iCount - 1, 1, wsString.data(), wsString.size());

			if(!wcscmp(wsString.data(), L"Записи не найдены ...")) continue;

			sprintf_s(sString.data(), sString.size(), "%ls", wsString.data());
			fld.SetString(sString.data(), strlen(sString.data()), doc.GetAllocator());
			record.AddMember(NUM_FLD_NAME, fld, doc.GetAllocator());

			ListView_GetItemText(hwndList, iCount - 1, 2, wsString.data(), wsString.size());
			sprintf_s(sString.data(), sString.size(), "%ls", wsString.data());
			fld.SetString(sString.data(), strlen(sString.data()), doc.GetAllocator());
			record.AddMember(TIM_FLD_NAME, fld, doc.GetAllocator());

			ListView_GetItemText(hwndList, iCount - 1, 3, wsString.data(), wsString.size());
			sprintf_s(sString.data(), sString.size(), "%ls", wsString.data());
			fld.SetString(sString.data(), strlen(sString.data()), doc.GetAllocator());
			record.AddMember(DAT_FLD_NAME, fld, doc.GetAllocator());

			ListView_GetItemText(hwndList, iCount - 1, 4, wsString.data(), wsString.size());
			sprintf_s(sString.data(), sString.size(), "%ls", wsString.data());
			fld.SetString(sString.data(), strlen(sString.data()), doc.GetAllocator());
			record.AddMember(DUR_FLD_NAME, fld, doc.GetAllocator());

			doc[HISTORY_OBJ_NAME][pListName].PushBack(record, doc.GetAllocator());
		}
	}
}

void CHistory::_LoadParameters()
{
	auto hPrevCursor = SetCursor(LoadCursor(hInstance, IDC_WAIT));

	_AddToCallList(L"Идёт загрузка записей ...", nullptr, nullptr, nullptr, enListType::enLTAll);

	int iContentLen = strlen("sip_acc=") + wstrLogin.length() + strlen("&") + strlen("sip_pwd=") + wstrPassword.length();
	array<char, 512> szPost;
	auto iLen = sprintf_s(szPost.data(), szPost.size(),
		"POST /api/sip.history.php HTTP/1.1\r\n"
		"Host: www.%ls\r\n"
		"Origin: http://www.%ls\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Cache-Control: max-age=0\r\n"
		"Accept: text/html,application/json;\r\n"
		"\r\n"
		"sip_acc=%ls&sip_pwd=%ls\r\n",
		wstrWEBServer.c_str(), wstrWEBServer.c_str(), iContentLen, wstrLogin.c_str(), wstrPassword.c_str());

	string strAnswer;
	auto pCloffReq = make_unique<CCloffRequest>();
	bool bDataPresent = pCloffReq->RequestDataFromDataBase(szPost.data(), strAnswer);

	_ListViewClear(true);
	if(bDataPresent) HTTPParser(strAnswer.c_str());

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile)
	{
		fseek(fCfgFile, 0, SEEK_END);
		auto fSize = ftell(fCfgFile);
		fseek(fCfgFile, 0, SEEK_SET);
		vector<char> vConfig;
		vConfig.resize(fSize);

		rapidjson::FileReadStream is(fCfgFile, vConfig.data(), vConfig.size());

		rapidjson::Document cfg;
		cfg.ParseStream(is);
		if(!cfg.HasParseError() && cfg.IsObject())
		{
			if(cfg.HasMember(HISTORY_OBJ_NAME) && cfg[HISTORY_OBJ_NAME].IsObject())
			{
				auto hist = cfg[HISTORY_OBJ_NAME].GetObject();

				array<wchar_t, 256> wszText;
				ListView_GetItemText(m_hInListWnd, 0, 1, wszText.data(), wszText.size());
				if(!wcscmp(wszText.data(), L"Записи не найдены ..."))  ParseJSONArray(enListType::enLTIn, hist);
				ListView_GetItemText(m_hOutListWnd, 0, 1, wszText.data(), wszText.size());
				if(!wcscmp(wszText.data(), L"Записи не найдены ..."))  ParseJSONArray(enListType::enLTOut, hist);
				ListView_GetItemText(m_hMissListWnd, 0, 1, wszText.data(), wszText.size());
				if(!wcscmp(wszText.data(), L"Записи не найдены ..."))  ParseJSONArray(enListType::enLTMiss, hist);
			}
		}
		::fclose(fCfgFile);
	}
	else m_Log._LogWrite(L"   History: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);

	SetCursor(hPrevCursor);
}

void CHistory::ParseJSONArray(enListType enT, rapidjson::Value::Object& list_obj)
{
	const char* pListName{ nullptr };
	switch(enT)
	{
		case enListType::enLTIn:   pListName = INC_ARRAY_NAME; break;
		case enListType::enLTOut:  pListName = OUT_ARRAY_NAME; break;
		case enListType::enLTMiss: pListName = MIS_ARRAY_NAME; break;
	}
	if(list_obj.HasMember(pListName) && list_obj[pListName].IsArray())
	{
		auto arr = list_obj[pListName].GetArray();
		for(auto& rec : arr)
		{
			string strBuffer;
			if(rec.HasMember(NUM_FLD_NAME) && rec[NUM_FLD_NAME].IsString()) strBuffer = rec[NUM_FLD_NAME].GetString();
			wstring wstrNumber;
			wstrNumber.resize(strBuffer.length() + 1);
			swprintf_s(wstrNumber.data(), wstrNumber.size(), L"%S", strBuffer.c_str());

			if(rec.HasMember(TIM_FLD_NAME) && rec[TIM_FLD_NAME].IsString()) strBuffer = rec[TIM_FLD_NAME].GetString();
			wstring wstrTime;
			wstrTime.resize(strBuffer.length() + 1);
			swprintf_s(wstrTime.data(), wstrTime.size(), L"%S", strBuffer.c_str());

			if(rec.HasMember(DAT_FLD_NAME) && rec[DAT_FLD_NAME].IsString()) strBuffer = rec[DAT_FLD_NAME].GetString();
			wstring wstrDate;
			wstrDate.resize(strBuffer.length() + 1);
			swprintf_s(wstrDate.data(), wstrDate.size(), L"%S", strBuffer.c_str());

			if(rec.HasMember(DUR_FLD_NAME) && rec[DUR_FLD_NAME].IsString()) strBuffer = rec[DUR_FLD_NAME].GetString();
			wstring wstrDuration;
			wstrDuration.resize(strBuffer.length() + 1);
			swprintf_s(wstrDuration.data(), wstrDuration.size(), L"%S", strBuffer.c_str());

			_AddToCallList(wstrNumber.c_str(), wstrTime.c_str(), wstrDate.c_str(), wstrDuration.c_str(), enT);
		}
	}
}

void CHistory::HTTPParser(const char* pszText)
{
	rapidjson::Document document;
	document.Parse(pszText);
	if(!document.HasParseError() && document.IsObject())
	{
		if(document.HasMember("errorcode") && document["errorcode"].IsInt())
		{
			if(document["errorcode"].GetInt() == 0)
			{
				if(document.HasMember("data") && document["data"].IsObject())
				{
					auto dataObj = document["data"].GetObjectW();
					if(dataObj.HasMember("arr") && dataObj["arr"].IsArray())
					{
						auto arr = dataObj["arr"].GetArray();
						for(decltype(arr.Size()) stRec = arr.Size(); stRec > 0; --stRec)
						{
							int iDirect{ -1 };
							if(arr[stRec - 1].HasMember("direct") && arr[stRec - 1]["direct"].IsInt()) iDirect = arr[stRec - 1]["direct"].GetInt();
							string strDT;
							if(arr[stRec - 1].HasMember("dt") && arr[stRec - 1]["dt"].IsString()) strDT = arr[stRec - 1]["dt"].GetString();

							wstring wstrDate, wstrTime;
							regex reData("(\\S+)-(\\S+)-(\\S+)\\s(\\S+)", std::regex_constants::icase);
							cmatch reMatch;
							if(regex_search(strDT.c_str(), reMatch, reData))
							{
								wstrDate.resize(wcslen(L"DD.MM.YYYY") + 1);
								swprintf_s(wstrDate.data(), wstrDate.size(), L"%S.%S.%S", reMatch[3].str().c_str(), reMatch[2].str().c_str(), reMatch[1].str().c_str());
								wstrTime.resize(wcslen(L"hh:mm:ss") + 1);
								swprintf_s(wstrTime.data(), wstrTime.size(), L"%S", reMatch[4].str().c_str());
							}
							else
							{
								wstrTime.resize(strDT.length() + 1);
								swprintf_s(wstrTime.data(), wstrTime.size(), L"%S", strDT.c_str());
							}
							int iCallDur{ 0 };
							if(arr[stRec - 1].HasMember("dur") && arr[stRec - 1]["dur"].IsInt()) iCallDur = arr[stRec - 1]["dur"].GetInt();
							int iAnswerDur{ 0 };
							if(arr[stRec - 1].HasMember("dur_ans") && arr[stRec - 1]["dur_ans"].IsInt()) iAnswerDur = arr[stRec - 1]["dur_ans"].GetInt();
							array<wchar_t, 256> wszDuration{ 0 };
							swprintf_s(wszDuration.data(), wszDuration.size(), L"%dмин:%02dсек / %dмин:%02dсек", iCallDur / 60, iCallDur % 60, iAnswerDur / 60, iAnswerDur % 60);
							string strNumber;
							if(arr[stRec - 1].HasMember("name") && arr[stRec - 1]["name"].IsString()) strNumber = arr[stRec - 1]["name"].GetString();
							vector<wchar_t> vData;
							switch(iDirect)
							{
								case 1:
								{
									if(!strNumber.length() && arr[stRec - 1].HasMember("to") && arr[stRec - 1]["to"].IsString()) strNumber = arr[stRec - 1]["to"].GetString();
									vData.resize(strNumber.length() + 1);
									swprintf_s(vData.data(), vData.size(), L"%S", strNumber.c_str());
									_AddToCallList(vData.data(), wstrTime.c_str(), wstrDate.c_str(), wszDuration.data(), enListType::enLTOut);
									break;//Out
								}
								case 2:
								{
									if(!strNumber.length() && arr[stRec - 1].HasMember("from") && arr[stRec - 1]["from"].IsString()) strNumber = arr[stRec - 1]["from"].GetString();
									vData.resize(strNumber.length() + 1);
									swprintf_s(vData.data(), vData.size(), L"%S", strNumber.c_str());
									_AddToCallList(vData.data(), wstrTime.c_str(), wstrDate.c_str(), wszDuration.data(), enListType::enLTIn);
									break;//In
								}
							}
						}
					}
				}
			}
		}
	}
}

void CHistory::_SetButtonStyle(LPDRAWITEMSTRUCT& pDIS)
{
	switch(pDIS->itemAction)
	{
		case ODA_SELECT:
		{
			if(pDIS->hwndItem == m_hwndButtonAll) { DrawButton(m_hwndButtonAll, pDIS->itemState & ODS_SELECTED ? m_hbmALL_GR : m_hbmALL_Act);  m_ActiveList = enListType::enLTAll; }
			else if(pDIS->hwndItem == m_hwndButtonIn) { DrawButton(m_hwndButtonIn, pDIS->itemState & ODS_SELECTED ? m_hbmIN_GR : m_hbmIN_Act);   m_ActiveList = enListType::enLTIn; }
			else if(pDIS->hwndItem == m_hwndButtonOut) { DrawButton(m_hwndButtonOut, pDIS->itemState & ODS_SELECTED ? m_hbmOUT_GR : m_hbmOUT_Act);   m_ActiveList = enListType::enLTOut; }
			else if(pDIS->hwndItem == m_hwndButtonMiss) { DrawButton(m_hwndButtonMiss, pDIS->itemState & ODS_SELECTED ? m_hbmMISS_GR : m_hbmMISS_Act); m_ActiveList = enListType::enLTMiss; }
			break;
		}
		case ODA_DRAWENTIRE:
		{
			if(pDIS->hwndItem == m_hwndButtonAll)		DrawButton(m_hwndButtonAll, m_ActiveList == enListType::enLTAll ? m_hbmALL_Act : m_hbmALL_HVR);
			else if(pDIS->hwndItem == m_hwndButtonIn)	DrawButton(m_hwndButtonIn, m_ActiveList == enListType::enLTIn ? m_hbmIN_Act : m_hbmIN_HVR);
			else if(pDIS->hwndItem == m_hwndButtonOut)	DrawButton(m_hwndButtonOut, m_ActiveList == enListType::enLTOut ? m_hbmOUT_Act : m_hbmOUT_HVR);
			else if(pDIS->hwndItem == m_hwndButtonMiss)	DrawButton(m_hwndButtonMiss, m_ActiveList == enListType::enLTMiss ? m_hbmMISS_Act : m_hbmMISS_HVR);
			break;
		}
		case ODA_FOCUS:
		{
			break;
		}
	}

	if(pDIS->hwndItem != m_hwndButtonAll && pDIS->itemAction == ODA_SELECT)	DrawButton(m_hwndButtonAll, m_hbmALL_HVR);
	else if(pDIS->hwndItem != m_hwndButtonIn && pDIS->itemAction == ODA_SELECT)	DrawButton(m_hwndButtonIn, m_hbmIN_HVR);
	else if(pDIS->hwndItem != m_hwndButtonOut && pDIS->itemAction == ODA_SELECT) DrawButton(m_hwndButtonOut, m_hbmOUT_HVR);
	else if(pDIS->hwndItem != m_hwndButtonMiss && pDIS->itemAction == ODA_SELECT) DrawButton(m_hwndButtonMiss, m_hbmMISS_HVR);
}

void CHistory::DrawButton(HWND hCurrentButton, HBITMAP hBitmap)
{
	HDC hDC = GetDC(hCurrentButton);

	// Draw the bitmap on button
	RECT rcImage;
	BITMAP bm;
	LONG cxBitmap{ 0 }, cyBitmap{ 0 };
	if(GetObject(hBitmap, sizeof(bm), &bm))
	{
		cxBitmap = bm.bmWidth;
		cyBitmap = bm.bmHeight;
	}

	// Center image horizontally 
	RECT rectItem;
	GetClientRect(hCurrentButton, &rectItem);
	CopyRect(&rcImage, &rectItem);
	LONG image_width = rcImage.right - rcImage.left;
	LONG image_height = rcImage.bottom - rcImage.top;
	rcImage.left = (image_width - cxBitmap) / 2;
	rcImage.top = (image_height - cyBitmap) / 2;
	DrawState(hDC, NULL, NULL, (LPARAM)hBitmap, 0, rcImage.left, rcImage.top, rcImage.right - rcImage.left, rcImage.bottom - rcImage.top, DSS_NORMAL | DST_BITMAP);
}

void CHistory::_Resize()
{
	RECT rcClient;                       // The parent window's client area.
	GetClientRect(m_hHistoryWnd, &rcClient);
	SetWindowPos(m_hAllListWnd, HWND_TOP, rcClient.left + 2, rcClient.top + 46, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 42, IsWindowVisible(m_hAllListWnd) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	SetWindowPos(m_hInListWnd, HWND_TOP, rcClient.left + 2, rcClient.top + 46, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 42, IsWindowVisible(m_hInListWnd) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	SetWindowPos(m_hOutListWnd, HWND_TOP, rcClient.left + 2, rcClient.top + 46, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 42, IsWindowVisible(m_hOutListWnd) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	SetWindowPos(m_hMissListWnd, HWND_TOP, rcClient.left + 2, rcClient.top + 46, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 42, IsWindowVisible(m_hMissListWnd) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
}

void CHistory::_ColumnResize()
{
	auto iNumColumnWidth{ ci_HistoryNumHeaderWidth };
	auto iTimeColumnWidth{ ci_HistoryTimeHeaderWidth };
	auto iDateColumnWidth{ ci_HistoryDateHeaderWidth };
	auto iDurColumnWidth{ ci_HistoryDurationHeaderWidth };

	auto hList = m_hAllListWnd;
	if(IsWindowVisible(m_hInListWnd)) hList = m_hInListWnd;
	else if(IsWindowVisible(m_hOutListWnd)) hList = m_hOutListWnd;
	else if(IsWindowVisible(m_hMissListWnd)) hList = m_hMissListWnd;

	iNumColumnWidth = ListView_GetColumnWidth(hList, 1);
	iTimeColumnWidth = ListView_GetColumnWidth(hList, 2);
	iDateColumnWidth = ListView_GetColumnWidth(hList, 3);
	iDurColumnWidth = ListView_GetColumnWidth(hList, 4);

	ListView_SetColumnWidth(m_hAllListWnd, 1, iNumColumnWidth);
	ListView_SetColumnWidth(m_hAllListWnd, 2, iTimeColumnWidth);
	ListView_SetColumnWidth(m_hAllListWnd, 3, iDateColumnWidth);
	ListView_SetColumnWidth(m_hAllListWnd, 4, iDurColumnWidth);

	ListView_SetColumnWidth(m_hInListWnd, 1, iNumColumnWidth);
	ListView_SetColumnWidth(m_hInListWnd, 2, iTimeColumnWidth);
	ListView_SetColumnWidth(m_hInListWnd, 3, iDateColumnWidth);
	ListView_SetColumnWidth(m_hInListWnd, 4, iDurColumnWidth);

	ListView_SetColumnWidth(m_hOutListWnd, 1, iNumColumnWidth);
	ListView_SetColumnWidth(m_hOutListWnd, 2, iTimeColumnWidth);
	ListView_SetColumnWidth(m_hOutListWnd, 3, iDateColumnWidth);
	ListView_SetColumnWidth(m_hOutListWnd, 4, iDurColumnWidth);

	ListView_SetColumnWidth(m_hMissListWnd, 1, iNumColumnWidth);
	ListView_SetColumnWidth(m_hMissListWnd, 2, iTimeColumnWidth);
	ListView_SetColumnWidth(m_hMissListWnd, 3, iDateColumnWidth);
	ListView_SetColumnWidth(m_hMissListWnd, 4, iDurColumnWidth);
}
bool CHistory::_IsEmptyList(HWND hListWnd)
{
	bool bRet{false};


	return bRet;
}
