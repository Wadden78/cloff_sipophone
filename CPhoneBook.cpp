#include "CPhoneBook.h"
#include "resource.h"
#include <commctrl.h>
#include <time.h>
#include <sys/timeb.h>
#include <ws2tcpip.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include "CCloffRequest.h"

CPhoneBook::CPhoneBook()
{
	int x{ 0 }, y{ 0 };
	int w{ ci_PhBWidth }, h{ ci_PhBHeight };
	int iNameHeaderWidth	{ ci_PhBNumHeaderWidth };		/**< ширина поля контакта*/
	int iCompanyHeaderWidth	{ ci_PhBNumHeaderWidth };		/**< ширина поля контакта*/
	int iPositionHeaderWidth{ ci_PhBNumHeaderWidth };		/**< ширина поля контакта*/
	int iNumberHeaderWidth	{ ci_PhBNumHeaderWidth };		/**< ширина поля контакта*/

	RECT rMainWindowRect{ 0 };
	GetWindowRect(hDlgPhoneWnd, &rMainWindowRect);

	x = rMainWindowRect.left;
	y = rMainWindowRect.top;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL) m_Log._LogWrite(L"   PhoneBook: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
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
			if(cfg.HasMember(PHBOOK_OBJ_NAME) && cfg[PHBOOK_OBJ_NAME].IsObject())
			{
				auto hist = cfg[PHBOOK_OBJ_NAME].GetObject();
				if(hist.HasMember(X_COORD_NAME) && hist[X_COORD_NAME].IsInt()) x = hist[X_COORD_NAME].GetInt();
				if(hist.HasMember(Y_COORD_NAME) && hist[Y_COORD_NAME].IsInt()) y = hist[Y_COORD_NAME].GetInt();
				if(hist.HasMember(W_WIDTH_NAME) && hist[W_WIDTH_NAME].IsInt()) w = hist[W_WIDTH_NAME].GetInt();
				if(hist.HasMember(H_HIGH_NAME)  && hist[H_HIGH_NAME].IsInt())  h = hist[H_HIGH_NAME].GetInt();

				if(hist.HasMember(PHB_NAM_HD_WIDTH_NAME) && hist[PHB_NAM_HD_WIDTH_NAME].IsInt())  iNameHeaderWidth     = hist[PHB_NAM_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(PHB_ORG_HD_WIDTH_NAME) && hist[PHB_ORG_HD_WIDTH_NAME].IsInt())  iCompanyHeaderWidth  = hist[PHB_ORG_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(PHB_POS_HD_WIDTH_NAME) && hist[PHB_POS_HD_WIDTH_NAME].IsInt())  iPositionHeaderWidth = hist[PHB_POS_HD_WIDTH_NAME].GetInt();
				if(hist.HasMember(PHB_NUM_HD_WIDTH_NAME) && hist[PHB_NUM_HD_WIDTH_NAME].IsInt())  iNumberHeaderWidth   = hist[PHB_NUM_HD_WIDTH_NAME].GetInt();
			}
		}
		else m_Log._LogWrite(L"   PhoneBook: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
		::fclose(fCfgFile);
	}

	// Register the window class.
	const wchar_t CLASS_NAME[] = L"PhoneBook Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = PhBWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(216, 216, 216)); //(HBRUSH)(COLOR_WINDOW + 1);

	RegisterClass(&wc);

	// Create the window.

	m_hPhBWnd = CreateWindowEx(0, CLASS_NAME, L"Контакты", WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU, x, y, w, h, 0, NULL, hInstance, this);

	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	RECT rcClient;                       // The parent window's client area.
	GetClientRect(m_hPhBWnd, &rcClient);
	// Create the list-view window in report view with label editing enabled.
	m_hListWnd = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE, 2, 2, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 4, m_hPhBWnd, (HMENU)ID_PHONEBOOKLIST_ALL, hInstance, NULL);

	auto hFnt = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	SendMessage(m_hListWnd, WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

	m_hImageList = ImageList_Create(24, 24, ILC_COLOR24, 1, 1);
	auto hBM = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONTACT));
	ImageList_Add(m_hImageList, hBM, 0);

	ListView_SetImageList(m_hListWnd, m_hImageList, LVSIL_SMALL);

	auto iListWiewWidth = rcClient.right - rcClient.left - 4;
	LVCOLUMN lvc;
	// Initialize the LVCOLUMN structure.
	// The mask specifies that the format, width, text,
	// and subitem members of the structure are valid.
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_IMAGE | LVCF_SUBITEM;

	// Add the columns.
	lvc.iSubItem = 0;
	lvc.pszText = (LPWSTR)L"";
	lvc.cx = 35; // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(m_hListWnd, 0, &lvc);

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	// Add the columns.
	lvc.iSubItem = 1;
	lvc.pszText = (LPWSTR)L"Имя";
	lvc.cx = iNameHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(m_hListWnd, 1, &lvc);

	// Add the columns.
	lvc.iSubItem = 2;
	lvc.pszText = (LPWSTR)L"Номер по умолчанию";
	lvc.cx = iNumberHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(m_hListWnd, 2, &lvc);

	// Add the columns.
	lvc.iSubItem = 3;
	lvc.pszText = (LPWSTR)L"Должность";
	lvc.cx = iPositionHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(m_hListWnd, 3, &lvc);

	// Add the columns.
	lvc.iSubItem = 4;
	lvc.pszText = (LPWSTR)L"Организация";
	lvc.cx = iCompanyHeaderWidth;               // Width of column in pixels.
	lvc.fmt = LVCFMT_LEFT;  // Left-aligned column.
	// Insert the columns into the list view.
	ListView_InsertColumn(m_hListWnd, 4, &lvc);

	ListView_SetExtendedListViewStyle(m_hListWnd, LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);

	ShowWindow(m_hListWnd, SW_SHOW);

	_LoadData();
	ShowWindow(m_hPhBWnd, SW_HIDE);

	m_Log._LogWrite(L"   PhoneBook: Constructor");
}
CPhoneBook::~CPhoneBook()
{
	DestroyWindow(m_hPhBWnd);
	m_hPhBWnd = NULL;
	m_Log._LogWrite(L"   PhoneBook: Destructor");
}

void CPhoneBook::_AddToList(const wchar_t* wstrName, const wchar_t* wstrCompany, const wchar_t* wstrPosition, const wchar_t* wstrNum)
{
	LVITEM lvI;
	lvI.mask = LVIF_IMAGE | LVIF_TEXT;
	lvI.stateMask = 0;
	lvI.state = 0;
	lvI.iItem = 0;
	lvI.iImage = 0;

	lvI.iSubItem = 0;
	lvI.pszText = nullptr;
	ListView_InsertItem(m_hListWnd, &lvI);

	ListView_SetItemText(m_hListWnd, 0, 1, (LPWSTR)wstrName);
	ListView_SetItemText(m_hListWnd, 0, 2, (LPWSTR)wstrNum);
	ListView_SetItemText(m_hListWnd, 0, 3, (LPWSTR)wstrPosition);
	ListView_SetItemText(m_hListWnd, 0, 4, (LPWSTR)wstrCompany);

}

void CPhoneBook::_ListViewClear(bool bClearAll)
{
	if(bClearAll)
	{
		m_vContactList.clear();
		ListView_DeleteAllItems(m_hListWnd);
	}
}

/*******************************************************/

LRESULT CALLBACK CPhoneBook::PhBWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CPhoneBook* pThis;

	if(uMsg == WM_NCCREATE)
	{
		pThis = static_cast<CPhoneBook*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

		SetLastError(0);
		if(!SetWindowLongPtr(hWnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(pThis)) && GetLastError()) return -1;
	}
	else pThis = reinterpret_cast<CPhoneBook*>(GetWindowLongPtr(hWnd, GWL_USERDATA));

	switch(uMsg)
	{
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				default:
				{
					if((LOWORD(wParam) >= ID_PHONEBOOKLIST_LISTBEG) && (LOWORD(wParam) <= ID_PHONEBOOKLIST_LISTEND))
					{
						if(pThis) pThis->_CallContactFromList(LOWORD(wParam));
					}
					else
					{
						switch(HIWORD(wParam))
						{
							case BN_PUSHED:
							case BN_CLICKED:
							{
								break;
							}
							default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
						}
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
						pThis->_LoadData();
						return true;
					}
					break;
				}
			}
		}
		case WM_SIZE:
		{
			if(pThis) pThis->_Paint();
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
							if(pThis)
							{
								pThis->_LoadData();
								return true;
							}
							break;
						}
						case VK_SPACE:
						{
							if(pThis)
							{
								POINT pt;
								GetCursorPos(&pt);
								auto hMenu = pThis->_CreatePopUpMenu(lParam);
								auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
								TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
							}
							break;
						}
						case VK_RETURN:
						{
							if(pThis) 
							{
								pThis->_CallContactByDefault(lParam);
								return true;
							}
							break;
						}
					}
					break;
				}
				case NM_DBLCLK:
				{
					if(pThis) pThis->_CallContactByDefault(lParam);
					return true;
					break;
				}
				case NM_RCLICK:
				{
					if(pThis)
					{
						POINT pt;
						GetCursorPos(&pt);
						auto hMenu = pThis->_CreatePopUpMenu(lParam);
						auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
						TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
					}
					break;
				}
				case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					switch(lplvcd->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:	return CDRF_NOTIFYITEMDRAW;
							break;
						case CDDS_ITEMPREPAINT:
							if(((int)lplvcd->nmcd.dwItemSpec % 2) == 0)
							{
								lplvcd->clrText = RGB(0, 0, 0);
								lplvcd->clrTextBk = RGB(216, 216, 216);
							}
							else
							{
								lplvcd->clrText = RGB(0, 0, 0);
								lplvcd->clrTextBk = RGB(255, 255, 255);
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

void CPhoneBook::_SaveParameters()
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
		::fclose(fCfgFile);
	}
	auto& allocator = cfg.GetAllocator();

	if(!cfg.IsObject()) cfg.SetObject();
	if(!cfg.HasMember(PHBOOK_OBJ_NAME))
	{
		rapidjson::Value hist_obj(rapidjson::kObjectType);
		cfg.AddMember(PHBOOK_OBJ_NAME, hist_obj, allocator);
	}

	RECT sCoord{ 0 };
	GetWindowRect(m_hPhBWnd, &sCoord);
	rapidjson::Value vVal;

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(X_COORD_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(X_COORD_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][X_COORD_NAME].SetInt(sCoord.left);

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(Y_COORD_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(Y_COORD_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][Y_COORD_NAME].SetInt(sCoord.top);

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(W_WIDTH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(W_WIDTH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][W_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(H_HIGH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(H_HIGH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][H_HIGH_NAME].SetInt(sCoord.bottom - sCoord.top);

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(PHB_NAM_HD_WIDTH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(PHB_NAM_HD_WIDTH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][PHB_NAM_HD_WIDTH_NAME].SetInt(ListView_GetColumnWidth(m_hListWnd, 1));
	if(!cfg[PHBOOK_OBJ_NAME].HasMember(PHB_NUM_HD_WIDTH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(PHB_NUM_HD_WIDTH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][PHB_NUM_HD_WIDTH_NAME].SetInt(ListView_GetColumnWidth(m_hListWnd, 2));
	if(!cfg[PHBOOK_OBJ_NAME].HasMember(PHB_POS_HD_WIDTH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(PHB_POS_HD_WIDTH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][PHB_POS_HD_WIDTH_NAME].SetInt(ListView_GetColumnWidth(m_hListWnd, 3));
	if(!cfg[PHBOOK_OBJ_NAME].HasMember(PHB_ORG_HD_WIDTH_NAME)) cfg[PHBOOK_OBJ_NAME].AddMember(PHB_ORG_HD_WIDTH_NAME, vVal, allocator);
	cfg[PHBOOK_OBJ_NAME][PHB_ORG_HD_WIDTH_NAME].SetInt(ListView_GetColumnWidth(m_hListWnd, 4));

	if(!cfg[PHBOOK_OBJ_NAME].HasMember(PHB_CONTACT_ARRAY_NAME)) 
	{
		rapidjson::Value arr(rapidjson::kArrayType);
		cfg[PHBOOK_OBJ_NAME].AddMember(PHB_CONTACT_ARRAY_NAME, arr, allocator);
	}
	auto arr = cfg[PHBOOK_OBJ_NAME][PHB_CONTACT_ARRAY_NAME].GetArray();
	arr.Clear();
	for(auto itContact : m_vContactList)
	{
		rapidjson::Value record(rapidjson::kObjectType);
		rapidjson::Value fld;

		array<char, 1024> sString;

		fld.SetInt(itContact->iId);
		record.AddMember(PHB_ID_FLD_NAME, fld, allocator);

		sprintf_s(sString.data(), sString.size(), "%ls", itContact->wstrName.c_str());
		fld.SetString(sString.data(), strlen(sString.data()), allocator);
		record.AddMember(PHB_NAME_FLD_NAME, fld, allocator);

		sprintf_s(sString.data(), sString.size(), "%ls", itContact->wstrLocalNum.c_str());
		fld.SetString(sString.data(), strlen(sString.data()), allocator);
		record.AddMember(PHB_LOCNUM_FLD_NAME, fld, allocator);

		sprintf_s(sString.data(), sString.size(), "%ls", itContact->wstrCompany.c_str());
		fld.SetString(sString.data(), strlen(sString.data()), allocator);
		record.AddMember(PHB_ORG_FLD_NAME, fld, allocator);

		sprintf_s(sString.data(), sString.size(), "%ls", itContact->wstrPosition.c_str());
		fld.SetString(sString.data(), strlen(sString.data()), allocator);
		record.AddMember(PHB_POS_FLD_NAME, fld, allocator);

		rapidjson::Value arrValue(rapidjson::kArrayType);
		record.AddMember(PHB_NUM_ARRAY_NAME, arrValue, allocator);

		auto arr_phone = record[PHB_NUM_ARRAY_NAME].GetArray();
		for(auto itPhone : itContact->vwstrNumber)
		{
			sprintf_s(sString.data(), sString.size(), "%ls", itPhone.c_str());
			fld.SetString(sString.data(), strlen(sString.data()), allocator);
			arr_phone.PushBack(fld, allocator);
		}

		arr.PushBack(record, allocator);
	}

	auto fFile = fopen("cloff_sip_phone.cfg", "w");
	if(fFile)
	{
		std::vector<char> writeBuffer;
		writeBuffer.resize(1048000);
		rapidjson::FileWriteStream os(fFile, writeBuffer.data(), writeBuffer.size());

		rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);

		cfg.Accept(writer);

		::fclose(fFile);
	}
	else m_Log._LogWrite(L"   History: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);

	SetCursor(hPrevCursor);
}

void CPhoneBook::_LoadData()
{
	auto hPrevCursor = SetCursor(LoadCursor(hInstance, IDC_WAIT));

	m_vContactList.clear();
	ListView_DeleteAllItems(m_hListWnd);

	int iContentLen = strlen("sip_acc=") + wstrLogin.length() + strlen("&") + strlen("sip_pwd=") + wstrPassword.length();
	vector<char> szPost; szPost.resize(1024);
	auto iLen = sprintf_s(szPost.data(), szPost.size(),
		"POST /api/sip.ab.php HTTP/1.1\r\n"
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
	if(!pCloffReq->RequestDataFromDataBase(szPost.data(), strAnswer))
	{
		m_Log._LogWrite(L"   PhoneBook: Load from file");
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
				if(cfg.HasMember(PHBOOK_OBJ_NAME) && cfg[PHBOOK_OBJ_NAME].IsObject())
				{
					auto phb = cfg[PHBOOK_OBJ_NAME].GetObject();
					JSONParser(phb);
				}
			}
			::fclose(fCfgFile);
		}
		else m_Log._LogWrite(L"   History: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
	}
	else HTTPParser(strAnswer.c_str());

	for(auto itRec = m_vContactList.rbegin(); itRec != m_vContactList.rend(); ++itRec)
		_AddToList(itRec->get()->wstrName.c_str(),
			itRec->get()->wstrCompany.c_str(),
			itRec->get()->wstrPosition.c_str(),
			itRec->get()->wstrLocalNum.length() ? itRec->get()->wstrLocalNum.c_str() : itRec->get()->vwstrNumber[0].c_str());

	SetCursor(hPrevCursor);
}

void CPhoneBook::_CreateNewRecord()
{

	//_AddToList(L"Имя", L"Номер по умолчанию");

	//m_vContactList.push_back(make_shared<SContact>());
	//m_vContactList.rbegin()->get()->pData = make_shared<nsContact::SContactData>();
	//m_vContactList.rbegin()->get()->pContactCard = make_shared<CContactCard>((int)m_vContactList.size() - 1, m_vContactList.rbegin()->get()->pData);
	//m_vContactList.rbegin()->get()->pContactCard->_Show();
}

void CPhoneBook::_Paint()
{
	RECT rcClient;                       // The parent window's client area.
	GetClientRect(m_hPhBWnd, &rcClient);
	SetWindowPos(m_hListWnd, HWND_TOP, rcClient.left + 2, rcClient.top + 2, rcClient.right - rcClient.left - 4, rcClient.bottom - rcClient.top - 4, SWP_SHOWWINDOW);
}

void CPhoneBook::_EditRecord(const int iRecNum)
{
	if(iRecNum < (decltype(iRecNum))m_vContactList.size())
	{
		ListView_SetItemText(m_hListWnd, m_vContactList[iRecNum]->iId, 1, (LPWSTR)m_vContactList[iRecNum]->wstrName.c_str());
		if(m_vContactList[iRecNum]->wstrLocalNum.length()) { ListView_SetItemText(m_hListWnd, m_vContactList[iRecNum]->iId, 2, (LPWSTR)m_vContactList[iRecNum]->wstrLocalNum.c_str()); }
		else { ListView_SetItemText(m_hListWnd, m_vContactList[iRecNum]->iId, 2, (LPWSTR)m_vContactList[iRecNum]->vwstrNumber[0].c_str()); }
	}
}

int CPhoneBook::HTTPParser(const char* pszText)
{
	int iRet = -1;

	rapidjson::Document document;
	document.Parse(pszText);
	if(!document.HasParseError() && document.IsObject())
	{
		if(document.HasMember("errorcode") && document["errorcode"].IsInt())
		{
			if(document["errorcode"].GetInt() == 0)
			{
				if(document.HasMember(PHB_DB_OBJ_NAME) && document.IsObject())
				{
					auto dataObj = document[PHB_DB_OBJ_NAME].GetObject();
					JSONParser(dataObj,true);
				}
			}
		}
	}
	else
	{
		m_Log._LogWrite(L"   PhoneBook: JSON parser ERROR. Offset=%d code=%d", document.GetErrorOffset(), document.GetParseError());
	}
	return iRet;
}

void CPhoneBook::JSONParser(rapidjson::GenericObject<false, rapidjson::Value>& phb_obj, bool bUTF8)
{
	if(phb_obj.HasMember(PHB_CONTACT_ARRAY_NAME) && phb_obj[PHB_CONTACT_ARRAY_NAME].IsArray())
	{
		auto arr = phb_obj[PHB_CONTACT_ARRAY_NAME].GetArray();	//массив абонентов
		for(auto& stRec : arr)
		{
			m_vContactList.push_back(make_shared<SContact>());

			if(stRec.HasMember(PHB_ID_FLD_NAME) && stRec[PHB_ID_FLD_NAME].IsInt()) m_vContactList.rbegin()->get()->iId = stRec[PHB_ID_FLD_NAME].GetInt();
			string strBuf;
			if(stRec.HasMember(PHB_NAME_FLD_NAME) && stRec[PHB_NAME_FLD_NAME].IsString()) strBuf = stRec[PHB_NAME_FLD_NAME].GetString();
			if(strBuf.length())
			{
				m_vContactList.rbegin()->get()->wstrName.resize(strBuf.length() + 1);
				if(bUTF8) MultiByteToWideChar(CP_UTF8, 0, strBuf.c_str(), -1, m_vContactList.rbegin()->get()->wstrName.data(), m_vContactList.rbegin()->get()->wstrName.length());
				else swprintf_s(m_vContactList.rbegin()->get()->wstrName.data(), m_vContactList.rbegin()->get()->wstrName.length(),L"%S", strBuf.c_str());
			}
			strBuf.clear();
			if(stRec.HasMember(PHB_ORG_FLD_NAME) && stRec[PHB_ORG_FLD_NAME].IsString()) strBuf = stRec[PHB_ORG_FLD_NAME].GetString();
			if(strBuf.length())
			{
				m_vContactList.rbegin()->get()->wstrCompany.resize(strBuf.length() + 1);
				if(bUTF8) MultiByteToWideChar(CP_UTF8, 0, strBuf.c_str(), -1, m_vContactList.rbegin()->get()->wstrCompany.data(), m_vContactList.rbegin()->get()->wstrCompany.length());
				else swprintf_s(m_vContactList.rbegin()->get()->wstrCompany.data(), m_vContactList.rbegin()->get()->wstrCompany.length(), L"%S", strBuf.c_str());
			}
			strBuf.clear();
			if(stRec.HasMember(PHB_POS_FLD_NAME) && stRec[PHB_POS_FLD_NAME].IsString()) strBuf = stRec[PHB_POS_FLD_NAME].GetString();
			if(strBuf.length())
			{
				m_vContactList.rbegin()->get()->wstrPosition.resize(strBuf.length() + 1);
				if(bUTF8) MultiByteToWideChar(CP_UTF8, 0, strBuf.c_str(), -1, m_vContactList.rbegin()->get()->wstrPosition.data(), m_vContactList.rbegin()->get()->wstrPosition.length());
				else swprintf_s(m_vContactList.rbegin()->get()->wstrPosition.data(), m_vContactList.rbegin()->get()->wstrPosition.length(), L"%S", strBuf.c_str());
			}
			strBuf.clear();
			if(stRec.HasMember(PHB_LOCNUM_FLD_NAME) && stRec[PHB_LOCNUM_FLD_NAME].IsString()) strBuf = stRec[PHB_LOCNUM_FLD_NAME].GetString();
			if(strBuf.length())
			{
				m_vContactList.rbegin()->get()->wstrLocalNum.resize(strBuf.length() + 1);
				if(bUTF8) MultiByteToWideChar(CP_UTF8, 0, strBuf.c_str(), -1, m_vContactList.rbegin()->get()->wstrLocalNum.data(), m_vContactList.rbegin()->get()->wstrLocalNum.length());
				else swprintf_s(m_vContactList.rbegin()->get()->wstrLocalNum.data(), m_vContactList.rbegin()->get()->wstrLocalNum.length(), L"%S", strBuf.c_str());
			}
			if(stRec.HasMember(PHB_NUM_ARRAY_NAME) && stRec[PHB_NUM_ARRAY_NAME].IsArray())
			{
				auto arr_phone = stRec[PHB_NUM_ARRAY_NAME].GetArray();	//массив номеров
				for(auto& stPhone : arr_phone)
				{
					wstring wstrRes;
					strBuf.clear();
					if(stPhone.IsString()) strBuf = stPhone.GetString();
					wstrRes.resize(strBuf.length() + 1);
					if(bUTF8) MultiByteToWideChar(CP_UTF8, 0, strBuf.c_str(), -1, wstrRes.data(), wstrRes.length());
					else swprintf_s(wstrRes.data(), wstrRes.length(), L"%S", strBuf.c_str());
					m_vContactList.rbegin()->get()->vwstrNumber.emplace_back(wstrRes);
				}
			}
		}
	}
}

void CPhoneBook::_CallContactFromList(int iStringID)
{
	auto wstrNumber = m_mInMenuNumberList[iStringID];
	auto iDog = wstrNumber.find_first_of(L'@');
	if(iDog != wstring::npos) wstrNumber.erase(iDog);

	SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wstrNumber.c_str());
	SetForegroundWindow(hDlgPhoneWnd);
	BringWindowToTop(hDlgPhoneWnd);
	ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
	SendMessage(hBtDial, BM_CLICK, 0, 0);
}

void CPhoneBook::_CallContactByDefault(LPARAM lParam)
{

	auto lpnmitem = (LPNMITEMACTIVATE)lParam;
	LVHITTESTINFO lvh;
	lvh.pt.x = lpnmitem->ptAction.x;
	lvh.pt.y = lpnmitem->ptAction.y;
	ListView_SubItemHitTest(((LPNMHDR)lParam)->hwndFrom, &lvh);
	if(lvh.iItem != -1)
	{
		array<wchar_t, 1024> wsString;
		ListView_GetItemText(((LPNMHDR)lParam)->hwndFrom, lvh.iItem, 2, wsString.data(), wsString.size());

		auto pDog = wcschr(wsString.data(),L'@');
		if(pDog) *pDog = 0;

		SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, wsString.data());
		SetForegroundWindow(hDlgPhoneWnd);
		BringWindowToTop(hDlgPhoneWnd);
		ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
		SendMessage(hBtDial, BM_CLICK, 0, 0);
	}
}

HMENU CPhoneBook::_CreatePopUpMenu(LPARAM lParam)
{
	HMENU hMenuRet{ nullptr };

	m_mInMenuNumberList.clear();

	auto lpnmitem = (LPNMITEMACTIVATE)lParam;
	LVHITTESTINFO lvh;
	lvh.pt.x = lpnmitem->ptAction.x;
	lvh.pt.y = lpnmitem->ptAction.y;
	ListView_SubItemHitTest(((LPNMHDR)lParam)->hwndFrom, &lvh);
	if(lvh.iItem != -1)
	{
		if(lvh.iItem < (decltype(lvh.iItem))m_vContactList.size())
		{
			hMenuRet = CreatePopupMenu();
			int iID{ ID_PHONEBOOKLIST_LISTBEG };
			for(auto& num : m_vContactList[lvh.iItem]->vwstrNumber) 
			{
				m_mInMenuNumberList.insert(pair<int, wstring&>(iID, num));
				AppendMenu(hMenuRet, MF_STRING, iID++, num.c_str());
			}
		}
	}

	return hMenuRet;
}

