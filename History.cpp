#include "History.h"
#include "resource.h"
#include <commctrl.h>
#include <time.h>
#include <sys/timeb.h>

CHistory::CHistory()
{
	CreateHistory();
	m_Log._LogWrite(L"   History: Constructor");
}
CHistory::~CHistory()
{
	ImageList_Destroy(m_hImageList);
	DestroyWindow(m_hHistoryWnd);
	m_hHistoryWnd = NULL;
	m_Log._LogWrite(L"   History: Destructor");
}

void CHistory::_AddToCallList(const wchar_t* wstrCallNum, const time_t tCallTime, enListType enLT)
{
	auto tCallBegin = *localtime(&tCallTime);
	array<wchar_t, 32> wszTime;
	swprintf_s(wszTime.data(), wszTime.size(), L"%02d:%02d:%02d %02d.%02d", tCallBegin.tm_hour, tCallBegin.tm_min, tCallBegin.tm_sec, tCallBegin.tm_mday, tCallBegin.tm_mon + 1/*, tCallBegin.tm_year + 1900*/);

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
		case enListType::enLTOut:	lvI.iImage = 0;	phwndList = &m_hOutListWnd;	break;
		case enListType::enLTIn:	lvI.iImage = 1;	phwndList = &m_hInListWnd;	break;
		case enListType::enLTMiss:	lvI.iImage = 2;	phwndList = &m_hMissListWnd;	break;
	}
	auto iCount = SendMessage(*phwndList, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR && iCount >= ci_HistoryMAX) ListView_DeleteItem(*phwndList, iCount - 1);

	iCount = SendMessage(m_hAllListWnd, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR && iCount >= ci_HistoryMAX * 2) ListView_DeleteItem(m_hAllListWnd, iCount - 1);

	ListView_InsertItem(*phwndList, &lvI);
	ListView_InsertItem(m_hAllListWnd, &lvI);

	ListView_SetItemText(*phwndList, 0, 1, (LPWSTR)wstrCallNum);
	ListView_SetItemText(m_hAllListWnd, 0, 1, (LPWSTR)wstrCallNum);
	ListView_SetItemText(*phwndList, 0, 2, (LPWSTR)wszTime.data());
	ListView_SetItemText(m_hAllListWnd, 0, 2, (LPWSTR)wszTime.data());
}

void CHistory::_AddToCallList(const wchar_t* wstrCallNum, enListType enLT)
{
	auto wsTime = wcschr(wstrCallNum, L' ');
	if(wsTime)
	{
		wstring wstrNum(wstrCallNum, wsTime - wstrCallNum);

		LVITEM lvI;
		lvI.mask = LVIF_IMAGE | LVIF_TEXT;
		lvI.stateMask = 0;
		lvI.state = 0;
		lvI.iItem = 0;

		lvI.iSubItem = 0;
		lvI.pszText = nullptr;
		// Insert items into the list.
		HWND* phwndList = &m_hOutListWnd;
		switch(enLT)
		{
			case enListType::enLTOut:	lvI.iImage = 0;	phwndList = &m_hOutListWnd;	break;
			case enListType::enLTIn:	lvI.iImage = 1;	phwndList = &m_hInListWnd;	break;
			case enListType::enLTMiss:	lvI.iImage = 2;	phwndList = &m_hMissListWnd;	break;
		}
		auto iCount = SendMessage(*phwndList, LVM_GETITEMCOUNT, 0, 0);
		if(iCount != LB_ERR && iCount >= ci_HistoryMAX) ListView_DeleteItem(*phwndList, iCount - 1);

		iCount = SendMessage(m_hAllListWnd, LVM_GETITEMCOUNT, 0, 0);
		if(iCount != LB_ERR && iCount >= ci_HistoryMAX * 2) ListView_DeleteItem(m_hAllListWnd, iCount - 1);

		ListView_InsertItem(*phwndList, &lvI);
		ListView_InsertItem(m_hAllListWnd, &lvI);
		ListView_SetItemText(*phwndList, 0, 1, (LPWSTR)wstrNum.c_str());
		ListView_SetItemText(m_hAllListWnd, 0, 1, (LPWSTR)wstrNum.c_str());
		ListView_SetItemText(*phwndList, 0, 2, (LPWSTR)++wsTime);
		ListView_SetItemText(m_hAllListWnd, 0, 2, (LPWSTR)wsTime);
	}
}

size_t CHistory::_GetCallList(vector<wstring>& vList, enListType enLT)
{
	HWND* phwndList = &m_hOutListWnd;
	switch(enLT)
	{
		case enListType::enLTIn:	phwndList = &m_hInListWnd;	break;
		case enListType::enLTOut:	phwndList = &m_hOutListWnd;	break;
		case enListType::enLTMiss:	phwndList = &m_hMissListWnd;	break;
	}

	auto iCount = SendMessage(*phwndList, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR)
	{
		for(; iCount; --iCount)
		{
			array<wchar_t, 1024> wsString;
			ListView_GetItemText(*phwndList, iCount - 1, 1, wsString.data(), wsString.size());
			auto stLen = wcslen(wsString.data());
			wsString[stLen++] = L' ';
			ListView_GetItemText(*phwndList, iCount - 1, 2, &wsString[stLen], wsString.size() - stLen);
			vList.push_back(wstring(wsString.data()));
		}
	}
	return vList.size();
}

void CHistory::_ListViewSelect(const int iListID)
{
	ShowWindow(m_hAllListWnd, iListID == ID_HISTBUTTON_ALL ? SW_SHOW : SW_HIDE);
	SendMessage(m_hwndButtonAll, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, iListID == ID_HISTBUTTON_ALL ? (LPARAM)m_hbmALL_Act : (LPARAM)m_hbmALL_Back);
	ShowWindow(m_hOutListWnd, iListID == ID_HISTBUTTON_OUT ? SW_SHOW : SW_HIDE);
	SendMessage(m_hwndButtonOut, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, iListID == ID_HISTBUTTON_OUT ? (LPARAM)m_hbmOUT_Act : (LPARAM)m_hbmOUT_Back);
	ShowWindow(m_hInListWnd, iListID == ID_HISTBUTTON_IN ? SW_SHOW : SW_HIDE);
	SendMessage(m_hwndButtonIn, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, iListID == ID_HISTBUTTON_IN ? (LPARAM)m_hbmIN_Act : (LPARAM)m_hbmIN_Back);
	ShowWindow(m_hMissListWnd, iListID == ID_HISTBUTTON_MISS ? SW_SHOW : SW_HIDE);
	SendMessage(m_hwndButtonMiss, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, iListID == ID_HISTBUTTON_MISS ? (LPARAM)m_hbmMISS_Act : (LPARAM)m_hbmMISS_Back);
}

void CHistory::_ListViewClear(bool bClearAll)
{
	if(IsWindowVisible(m_hAllListWnd) || bClearAll) ListView_DeleteAllItems(m_hAllListWnd);
	if(IsWindowVisible(m_hOutListWnd) || bClearAll) ListView_DeleteAllItems(m_hOutListWnd);
	if(IsWindowVisible(m_hInListWnd) || bClearAll) ListView_DeleteAllItems(m_hInListWnd);
	if(IsWindowVisible(m_hMissListWnd) || bClearAll) ListView_DeleteAllItems(m_hMissListWnd);
}

void CHistory::_ListViewCopy2Clipboard()
{
	HWND hwndCurrentList = nullptr;
	if(IsWindowVisible(m_hAllListWnd))  hwndCurrentList = m_hAllListWnd;
	if(IsWindowVisible(m_hOutListWnd))  hwndCurrentList = m_hOutListWnd;
	if(IsWindowVisible(m_hInListWnd) )  hwndCurrentList = m_hInListWnd;
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
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE,(wcslen(wsString.data()) + 1) * sizeof(wchar_t));
		if(hglbCopy == NULL)
		{
			CloseClipboard();
			return;
		}
		// Lock the handle and copy the text to the buffer. 
		lptstrCopy = static_cast<LPTSTR>(GlobalLock(hglbCopy));
		if(lptstrCopy) memcpy(lptstrCopy, wsString.data(), (wcslen(wsString.data())+ 1) * sizeof(wchar_t));
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
HWND CHistory::CreateListView(HWND hwndParent, const int iLVCodeID)
{
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.
	GetClientRect(hwndParent, &rcClient);
	// Create the list-view window in report view with label editing enabled.
	HWND hWndListView = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS, 2, 42, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 46, hwndParent, (HMENU)iLVCodeID, hInstance, NULL);

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
	lvc.cx = ci_HistoryNumHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 1, &lvc);
	// Add the columns.
	lvc.iSubItem = 2;
	lvc.pszText = (LPWSTR)L"Время вызова";
	lvc.cx = iListWiewWidth - ci_HistoryNumHeaderWidth - ci_HistoryLTHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(hWndListView, 2, &lvc);

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
				case ID__MHL_COPY:		if(pThis) pThis->_ListViewCopy2Clipboard(); break;
				case ID__MHL_CLEAR:		if(pThis) pThis->_ListViewClear();			break;
				case ID__MHL_CLEAR_ALL:	if(pThis) pThis->_ListViewClear(true);		break;
				default:
				{
					switch(HIWORD(wParam))
					{
						case BN_PUSHED:
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
		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->code)
			{
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
						SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
					}
					break;
				}
				case NM_RCLICK:
				{
					POINT pt;
					GetCursorPos(&pt);
					auto hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_HLIST));
					hMenu = GetSubMenu(hMenu, 0);
					SetForegroundWindow(hDlgPhoneWnd);
					auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
					break;
				}
				default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_CLOSE:
		{
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		case WM_DESTROY:
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

//LRESULT CALLBACK CHistory::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	BOOL bRet = TRUE;
//
//	switch(uMsg)
//	{
//		case WM_CREATE:	{	break;	}
//		//case WM_PAINT:
//		//{
//		//	PAINTSTRUCT ps;
//		//	auto hDC = BeginPaint(hWnd, &ps);
//
//		//	EndPaint(hWnd, &ps);
//		//	bRet = FALSE;
//		//	break;
//		//}
//		case WM_COMMAND:
//		{
//			switch(HIWORD(wParam))
//			{
//				case BN_PUSHED:
//				case BN_CLICKED:
//				{
//					switch(LOWORD(wParam))
//					{
//						case ID_HISTBUTTON_ALL:	 ListViewSelect(m_hAllListWnd);		break;
//						case ID_HISTBUTTON_OUT:	 ListViewSelect(m_hOutListWnd);		break;
//						case ID_HISTBUTTON_IN:	 ListViewSelect(m_hInListWnd);		break;
//						case ID_HISTBUTTON_MISS: ListViewSelect(m_hMissListWnd);	break;
//						default: break;
//					}
//					bRet = FALSE;
//					break;
//				}
//			}
//			break;
//		}
//		case WM_NOTIFY:
//		{
//			switch(((LPNMHDR)lParam)->code)
//			{
//				case NM_DBLCLK:
//				{
//					auto lpnmitem = (LPNMITEMACTIVATE)lParam;
//					LVHITTESTINFO lvh;
//					lvh.pt.x = lpnmitem->ptAction.x;
//					lvh.pt.y = lpnmitem->ptAction.y;
//					ListView_SubItemHitTest(((LPNMHDR)lParam)->hwndFrom, &lvh);
//					if(lvh.iItem != -1)
//					{
//						array<wchar_t, 1024> wsString;
//						ListView_GetItemText(((LPNMHDR)lParam)->hwndFrom, lvh.iItem, 1, wsString.data(), wsString.size());
//						SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
//					}
//					bRet = FALSE;
//					break;
//				}
//				default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
//			}
//		}
//		case WM_CLOSE:
//		{
//			ShowWindow(hWnd, SW_HIDE);
//			bRet = FALSE;
//			break;
//		}
//		case WM_DESTROY:
//		{
//			return DefWindowProc(hWnd, uMsg, wParam, lParam);
//		}
//		default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
//	}
//	return 0;
//}

bool CHistory::CreateHistory()
{
// Register the window class.
	const wchar_t CLASS_NAME[] = L"History Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = HistoryWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	RegisterClass(&wc);

	// Create the window.

	m_hHistoryWnd = CreateWindowEx(0, CLASS_NAME, L"Списки вызовов", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, ci_HistoryWidth, ci_HistoryHeight, 0, NULL, hInstance, this);

	m_hwndButtonAll = CreateWindow(L"BUTTON", L"Все", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_BITMAP | BS_FLAT | BS_PUSHLIKE, 2, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_ALL, hInstance, NULL);
	m_hbmALL_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ALL_GR));
	m_hbmALL_Back = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ALL));
	SendMessage(m_hwndButtonAll, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hbmALL_Act);
	SendMessage(m_hwndButtonAll, BM_SETCHECK, 1, 0);
	SendMessage(m_hwndButtonAll, BM_SETSTATE, 1, 0);

	m_hwndButtonOut = CreateWindow(L"BUTTON", L"Out", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_BITMAP | BS_FLAT | BS_PUSHLIKE, 84, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_OUT, hInstance, NULL);
	m_hbmOUT_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUT_GR));
	m_hbmOUT_Back = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUTCALL));
	SendMessage(m_hwndButtonOut, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hbmOUT_Back);

	m_hwndButtonIn = CreateWindow(L"BUTTON", L"In", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_BITMAP | BS_FLAT | BS_PUSHLIKE, 166, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_IN, hInstance, NULL);
	m_hbmIN_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_IN_GR));
	m_hbmIN_Back = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_INCALL));
	SendMessage(m_hwndButtonIn, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hbmIN_Back);

	m_hwndButtonMiss = CreateWindow(L"BUTTON", L"Miss", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | BS_BITMAP | BS_FLAT | BS_PUSHLIKE, 248, 2, 80, 40, m_hHistoryWnd, (HMENU)ID_HISTBUTTON_MISS, hInstance, NULL);
	m_hbmMISS_Act = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISS_GR));
	m_hbmMISS_Back = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISSCALL));
	SendMessage(m_hwndButtonMiss, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hbmIN_Back);

	m_hImageList = ImageList_Create(24, 24, ILC_COLOR24, 3, 1);
	auto hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_OUT_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);
	hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_IN_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);
	hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MISS_SMALL));
	ImageList_Add(m_hImageList, hBM, 0);

	m_hAllListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_ALL);
	ShowWindow(m_hAllListWnd, SW_SHOW);
	m_hOutListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_OUT);
	ShowWindow(m_hOutListWnd, SW_HIDE);
	m_hInListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_IN);
	ShowWindow(m_hInListWnd, SW_HIDE);
	m_hMissListWnd = CreateListView(m_hHistoryWnd, ID_HISTLISTV_MISS);
	ShowWindow(m_hMissListWnd, SW_HIDE);

	ShowWindow(m_hHistoryWnd, SW_HIDE);

	return m_hHistoryWnd ? true : false;
}


void CHistory::_SaveParameters(unique_ptr<CConfigFile>& pCfg)
{
	RECT sCoord{ 0 };
	GetWindowRect(m_hHistoryWnd, &sCoord);
	pCfg->PutIntParameter("X", sCoord.left, "HISTORY");
	pCfg->PutIntParameter("Y", sCoord.top);
	pCfg->PutIntParameter("W", sCoord.right - sCoord.left);
	pCfg->PutIntParameter("H", sCoord.bottom - sCoord.top);
	ListView_GetSubItemRect(m_hOutListWnd, 0, 1, LVIR_LABEL, &sCoord);
	pCfg->PutIntParameter("HeaderW", sCoord.right - sCoord.left);

	wstring wstrList;
	List2Wstring(m_hOutListWnd, wstrList);
	pCfg->PutStringParameter("OutList", wstrList.c_str());
	List2Wstring(m_hInListWnd, wstrList);
	pCfg->PutStringParameter("IncList", wstrList.c_str());
	List2Wstring(m_hMissListWnd, wstrList);
	pCfg->PutStringParameter("MissedList", wstrList.c_str());
}

size_t CHistory::List2Wstring(HWND& hwndList, wstring& wstrRes)
{
	wstrRes.clear();
	auto iCount = SendMessage(hwndList, LVM_GETITEMCOUNT, 0, 0);
	if(iCount != LB_ERR)
	{
		for(; iCount; --iCount)
		{
			array<wchar_t, 1024> wsString;
			ListView_GetItemText(hwndList, iCount - 1, 1, wsString.data(), wsString.size());
			wstrRes.append(wsString.data());
			wstrRes.append(L" ");
			ListView_GetItemText(hwndList, iCount - 1, 2, wsString.data(), wsString.size());
			wstrRes.append(wsString.data());
			wstrRes.append(L",");
		}
	}
	return wstrRes.length();
}

void CHistory::_LoadParameters(unique_ptr<CConfigFile>& pCfg)
{
	int x{ 0 }, y{ 0 }, w{ ci_HistoryWidth }, h{ ci_HistoryHeight };
	x = pCfg->GetIntParameter("HISTORY", "X", x);
	y = pCfg->GetIntParameter("HISTORY", "Y", y);
	SetWindowPos(m_hHistoryWnd, HWND_TOP, x, y, ci_HistoryWidth, ci_HistoryHeight, SWP_HIDEWINDOW);

	wstring wstrList;
	pCfg->GetStringParameter("HISTORY", "OutList", wstrList);
	GetCallInfoList(m_hOutListWnd, wstrList.c_str(), enListType::enLTOut);
	pCfg->GetStringParameter("HISTORY", "IncList", wstrList);
	GetCallInfoList(m_hInListWnd, wstrList.c_str(), enListType::enLTIn);
	pCfg->GetStringParameter("HISTORY", "MissedList", wstrList);
	GetCallInfoList(m_hMissListWnd, wstrList.c_str(), enListType::enLTMiss);
}
void CHistory::GetCallInfoList(HWND& hwndList, const wchar_t* wszSource, enListType enLT)
{
	if(!wcslen(wszSource)) return;

	auto pStart = wszSource;
	auto pEnd = wszSource + wcslen(wszSource);
	wstring wstrNumber;
	while(pStart < pEnd)
	{
		auto pNumEnd = wcschr(pStart, L',');
		if(pNumEnd)
		{
			wstrNumber.clear();
			wstrNumber.append(pStart, pNumEnd - pStart);
			if(m_History) m_History->_AddToCallList(wstrNumber.c_str(), enLT);
			if(*(++pNumEnd) == L'\r') break;
		}
		else break;
		pStart = pNumEnd;
	}
}

/*
	m_hInListWnd = CreateWindow(WC_LISTBOX, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_MULTICOLUMN | WS_VSCROLL | WS_HSCROLL, 5, 50, rcClient.right - 10, rcClient.bottom - 55, hwndParent, NULL, hInstance, NULL);
*/