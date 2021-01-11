#include <winsock2.h>
#include <Windows.h>
#include <Windowsx.h>
#include <conio.h>
#include <string>
#include <vector>
#include <codecvt>
#include "resource.h"
#include "Main.h"
#include <wincrypt.h>
#include <shellapi.h>
#include "CWebSocket.h"
#include "History.h"
#include "Config.h"
#include "CPhoneBook.h"
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include "rapidjson/filereadstream.h"
#include <VersionHelpers.h>

std::unique_ptr<CSIPWEB> m_SIPWEB;
std::unique_ptr<CSIPProcess> m_SIPProcess;
std::unique_ptr<CWebSocket> m_WebSocketServer;
std::unique_ptr<CHistory> m_History;
std::unique_ptr<CPhoneBook> m_PhoneBook;
in_port_t ipWSPort = 20005;
string strProductName;
string strProductVersion;
array<wchar_t, 1024> csEXEPath;
std::wstring wstrWEBServer{ L"cloff.ru" };

bool bMinimizeOnStart{ false };	/** Минимизация после старта*/
bool bAutoStart{ false };		/** Автоматический запуск после старта ОС*/

CLogFile m_Log;

std::wstring	wstrLogin{ L"" };
std::wstring	wstrPassword{ L"" };
std::wstring	wstrServer{ L"sip.cloff.ru:5060" };
int iTransportType{ IPPROTO_TCP };
uint16_t uiRingToneNumber{ 0 };
bool bDTMF_2833{ true };
bool bDebugMode{ false };
bool bConfigMode{ false };

bool bMicrofonOn{ true };
HBITMAP hBtMpMicON{ 0 };
HBITMAP hBtMpMicOFF{ 0 };
DWORD dwMicLevel{ ci_LevelMAX };

bool bSndOn{ true };
HBITMAP hBtMpSndON{ 0 };
HBITMAP hBtMpSndOFF{ 0 };
DWORD dwSndLevel{ ci_LevelMAX };

HWND hSliderMic{ 0 };
HWND hSliderSnd{ 0 };

HWND hDlgPhoneWnd{ 0 };

HWND hComboBox{ 0 };
HWND hBtDial{ 0 };
HWND hBtDisconnect{ 0 };
HWND hBtHistory{ 0 };
HWND hBt1{ 0 };
HWND hBt2{ 0 };
HWND hBt3{ 0 };
HWND hBtBackSpace{ 0 };
HWND hBt4{ 0 };
HWND hBt5{ 0 };
HWND hBt6{ 0 };
HWND hBtConfig{ 0 };
HWND hBt7{ 0 };
HWND hBt8{ 0 };
HWND hBt9{ 0 };
HWND hBtPhoneBook{ 0 };
HWND hBtAsterisk{ 0 };
HWND hBt0{ 0 };
HWND hBtDies{ 0 };
HWND hBtContactState{ 0 };
HWND hBtP{ 0 };
HWND hBtW{ 0 };
HWND hBtMute{ 0 };
HWND hBtSilence{ 0 };
HWND hStatusInfo{ 0 };

HIMAGELIST hDialImageList{ 0 };
HIMAGELIST hDiscImageList{ 0 };
HIMAGELIST hHistImageList{ 0 };
HIMAGELIST h1ImageList{ 0 };
HIMAGELIST h2ImageList{ 0 };
HIMAGELIST h3ImageList{ 0 };
HIMAGELIST h4ImageList{ 0 };
HIMAGELIST h5ImageList{ 0 };
HIMAGELIST h6ImageList{ 0 };
HIMAGELIST h7ImageList{ 0 };
HIMAGELIST h8ImageList{ 0 };
HIMAGELIST h9ImageList{ 0 };
HIMAGELIST h0ImageList{ 0 };
HIMAGELIST hPImageList{ 0 };
HIMAGELIST hWImageList{ 0 };
HIMAGELIST hAstImageList{ 0 };
HIMAGELIST hDiesImageList{ 0 };
HIMAGELIST hCfgImageList{ 0 };
HIMAGELIST hPhBImageList{ 0 };
HIMAGELIST hCInfImageList{ 0 };
HIMAGELIST hMicImageList{ 0 };
HIMAGELIST hSndImageList{ 0 };

HINSTANCE hInstance{ 0 };

list<wstring>	lSubscriberAccounts;

array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName{ 0 };

int iMainX{ CW_USEDEFAULT };
int iMainY{ CW_USEDEFAULT };

bool bShift{ false };

LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitDialog();

int CALLBACK WinMain(HINSTANCE curInst, HINSTANCE hPrev, LPSTR lpstrCmdLine, int nCmdShow)
{
	bool bHelp{false};

	hInstance = curInst;
	setlocale(LC_CTYPE, ".1251");

	GetModuleFileName(NULL, csEXEPath.data(), csEXEPath.size());
	GetProductAndVersion();

	std::array<wchar_t, 1500> buffer{ 0 };

	if(IsWindows7OrGreater() && GetEnvironmentVariable(L"ProgramData", buffer.data(), buffer.size()))
	{
		std::array<wchar_t, MAX_PATH> pf{ 0 };
		if(GetEnvironmentVariable(L"ProgramFiles", pf.data(), pf.size()))
		{
			if(pf[0] && _wcsnicmp(pf.data(), csEXEPath.data(), wcslen(pf.data())) == 0)
			{
				swprintf_s(&buffer[wcslen(buffer.data())], buffer.size() - wcslen(buffer.data()), L"\\%S", strProductName.c_str());
				CreateDirectory(buffer.data(), NULL);
				SetCurrentDirectory(buffer.data());
			}
		}
	}

	if(CheckCopyStart() != 0) return 0;

	m_Log._Start();

	int iNumParams{ 0 };
	auto argv = CommandLineToArgvW(GetCommandLineW(), &iNumParams);
	for(decltype(iNumParams) iCnt = 0; iCnt < iNumParams; ++iCnt)
	{
		wchar_t* pPar = nullptr;
		if(wcsstr(argv[iCnt], L"-debug")) bDebugMode = true;
		if(wcsstr(argv[iCnt], L"-config")) bConfigMode = true;
		if(wcsstr(argv[iCnt], L"-default")) DeleteFile(L"cloff_sip_phone.cfg");
		if(wcsstr(argv[iCnt], L"-autostart")) { CorrectBoolParameter(MAIN_AUTOSTART_NAME, true); SetAutostart(true);  }
		if(wcsstr(argv[iCnt], L"-minimize")) CorrectBoolParameter(MAIN_MINONSTART_NAME, true);
		if(wcsstr(argv[iCnt], L"-help") || wcsstr(argv[iCnt], L"-h") || wcsstr(argv[iCnt], L"\\help") || wcsstr(argv[iCnt], L"\\h")) bHelp = true;
		if((pPar = wcsstr(argv[iCnt], L"-login:")) != nullptr) wstrLogin = pPar + wcslen(L"-login:");
		if((pPar = wcsstr(argv[iCnt], L"-password:")) != nullptr) wstrPassword = pPar + wcslen(L"-password:");
	}

	m_Log._LogWrite(L"---------------------------------------------------------------");
	m_Log._LogWrite(L"Main: *************** New Start  V.%S key=%S ****************", strProductVersion.c_str(), lpstrCmdLine);

	if(bHelp)
	{
		printf("-debug\r\n-config\r\n-default\r\n-web\r\n-login\r\n-password\r\n-help\r\n");
		while(!_kbhit());
	}
	else
	{
		DWORD dwLen = MAX_COMPUTERNAME_LENGTH;
		if(!GetComputerNameA((LPSTR)szCompName.data(), &dwLen)) sprintf_s(szCompName.data(), szCompName.size(), "Undefined");

		unique_ptr<CConfigFile> pCfg;
		pCfg = make_unique<CConfigFile>(false);
		if(pCfg->_CheckConfig())
		{
			LoadMainParameters(pCfg);
			pCfg.reset(nullptr);
			DeleteFile(L"cloff_sip_phone.cfg");
			SaveJSONMainParameters();
		}
		else LoadJSONMainParameters();

		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if(iResult != NO_ERROR) m_Log._LogWrite(L"Main: Error at WSAStartup() !!!");

		InitDialog();
		MSG msg{};

		// Цикл основного сообщения:
		while(GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		WSACleanup();
	}

	m_Log._LogWrite(L"Main: *************** Stop ****************");

	m_Log._Stop(5000);

	return 0;
}

void AddDigitToComboDial(HWND hWnd, wchar_t wcDigit)
{
	std::array<wchar_t, 1024> vText; vText[0] = 0;
	auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data(), vText.size());
	swprintf_s(vText.data(), vText.size() - wcslen(vText.data()), L"%s%c", vText.data(), wcDigit);
	SetWindowText(hComboBox, vText.data());
	if(m_SIPProcess && m_SIPProcess->_DTMF(wcDigit)) Beep(400 + wcDigit, 100);
}

void CheckDigitInComboDial(HWND hWnd)
{
	std::array<wchar_t, 1024> vText; vText[0] = 0;
	auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data(), vText.size());
	if(iLen && (vText[iLen - 1] >= '0'))
	{
		if(m_SIPProcess && m_SIPProcess->_DTMF(vText[iLen - 1])) Beep(400 + vText[iLen - 1], 100);
	}
}

bool bHideIcon{ false };

LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch(uMsg)
	{
		case WM_SYSCOMMAND:
		{
			switch(wParam)
			{
				case SC_MINIMIZE:
				{
					MinimizeToTray();
					ShowNotifyIcon(TRUE);
					// Return TRUE to tell DefDlgProc that we handled the message, and set
					// DWL_MSGRESULT to 0 to say that we handled WM_SYSCOMMAND
					SetWindowLong(hWnd, DWL_MSGRESULT, 0);
					return TRUE;
				}
				case SC_CLOSE:
				{
					if(MessageBox(hWnd, L"Закрыть приложение?", L"SIP Phone", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						DefWindowProc(hWnd, uMsg, wParam, lParam);
					}
					else bRet = TRUE;
					break;
				}
				default:  return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDABORT:
				{
					m_Log._LogWrite(L"IDABORT");
					SaveJSONMainParameters();
					ShowNotifyIcon(FALSE);
					if(m_WebSocketServer) m_WebSocketServer->_Stop(5000);
					EndDialog(hWnd, LOWORD(wParam));
					if(m_SIPProcess) m_SIPProcess->_Stop(15000);
					if(m_History) { m_History->_Stop(1000); m_History.reset(nullptr); }
					if(m_PhoneBook) m_PhoneBook.reset(nullptr);
					bRet = TRUE;
					break;
				}
				case IDOK:
				case IDC_BUTTON_DIAL:
				{
					if(m_SIPProcess)
					{
						std::array<wchar_t, 32> vCommand;
						auto iLen = GetDlgItemText(hWnd, IDC_BUTTON_DIAL, vCommand.data(), vCommand.size());
						if(m_SIPProcess->_GetCallState() == nsMsg::CallState::stNUll)
						{
							std::vector<wchar_t> vCalledPNUM;
							vCalledPNUM.resize(256);
							auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, &vCalledPNUM[0], vCalledPNUM.size());
							if(iLen) m_SIPProcess->_MakeCall(vCalledPNUM.data());
							auto iRes = SendMessage(hComboBox, (UINT)CB_FINDSTRINGEXACT, (WPARAM)1, (LPARAM)vCalledPNUM.data());
							if(iRes == CB_ERR)
							{
								auto iCount = SendMessage(hComboBox, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
								if(iCount != CB_ERR && iCount >= ci_HistoryMAX) SendMessage(hComboBox, (UINT)CB_DELETESTRING, (WPARAM)--iCount, (LPARAM)0);
								SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)0, (LPARAM)vCalledPNUM.data());
							}
						}
						else if(m_SIPProcess->_GetCallState() == nsMsg::CallState::stRinging) m_SIPProcess->_Answer();
					}
					break;
				}
				case IDC_BUTTON_DISCONNECT:	if(m_SIPProcess) m_SIPProcess->_Disconnect(); break;
				case IDC_COMBO_DIAL:
				{
					switch(HIWORD(wParam))
					{
						case CBN_SELCHANGE:
						{
							auto iSelString = SendMessage(hComboBox, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
							if(iSelString != CB_ERR) SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)iSelString, (LPARAM)0);
							break;
						}
						case CBN_EDITCHANGE:	CheckDigitInComboDial(hWnd);								break;
						case CBN_EDITUPDATE:	CheckDigitInComboDial(hWnd); 								break;
						case CBN_SELENDCANCEL:  m_Log._LogWrite(L"CBN_SELENDCANCEL LOWORD(wParam)=%d", LOWORD(wParam)); break;
						default:																			break;
					}
					break;
				}
				case IDC_BUTTON_0:			AddDigitToComboDial(hWnd, '0');	break;
				case IDC_BUTTON_1:			AddDigitToComboDial(hWnd, '1');	break;
				case IDC_BUTTON_2:			AddDigitToComboDial(hWnd, '2');	break;
				case IDC_BUTTON_3:			AddDigitToComboDial(hWnd, '3');	break;
				case IDC_BUTTON_4:			AddDigitToComboDial(hWnd, '4');	break;
				case IDC_BUTTON_5:			AddDigitToComboDial(hWnd, '5');	break;
				case IDC_BUTTON_6:			AddDigitToComboDial(hWnd, '6');	break;
				case IDC_BUTTON_7:			AddDigitToComboDial(hWnd, '7');	break;
				case IDC_BUTTON_8:			AddDigitToComboDial(hWnd, '8');	break;
				case IDC_BUTTON_9:			AddDigitToComboDial(hWnd, '9');	break;
				case IDC_BUTTON_ASTERISK:	AddDigitToComboDial(hWnd, '*');	break;
				case IDC_BUTTON_DIES:		AddDigitToComboDial(hWnd, '#');	break;
				case IDC_BUTTON_WAIT:		AddDigitToComboDial(hWnd, ';');	break;
				case IDC_BUTTON_PAUSE:		AddDigitToComboDial(hWnd, ',');	break;
				case IDC_BUTTON_MUTE:
				{
					bMicrofonOn = !bMicrofonOn;
					SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, bMicrofonOn ? (WPARAM)dwMicLevel : (WPARAM)0);
					if(m_SIPProcess)m_SIPProcess->_Microfon(bMicrofonOn ? dwMicLevel : 0);
//						SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
					DrawButton(hBtMute, bMicrofonOn ? hBtMpMicON : hBtMpMicOFF);
					break;
				}
				case IDC_BUTTON_SILENCE:
				{
					bSndOn = !bSndOn;
					SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, bSndOn ? (WPARAM)dwSndLevel : (WPARAM)0);
					if(m_SIPProcess) m_SIPProcess->_Sound(bSndOn ? dwSndLevel : 0);
//						SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
					DrawButton(hBtSilence, bSndOn ? hBtMpSndON : hBtMpSndOFF);
					break;
				}
				case IDC_BUTTON_BACKSPACE:
				{
					std::array<wchar_t, 1024> vText; vText[0] = 0;
					auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data(), vText.size());
					if(iLen) vText[iLen - 1] = 0;
					SetWindowText(hComboBox, vText.data());
					break;
				}
				case IDC_BUTTON_CONFIG:
				{
					DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PARAMS), hWnd, (ConfigDlgProc), 0);
					SaveJSONMainParameters();
					if(m_History) { m_History->_Stop(1000); m_History.reset(nullptr); }
					if(m_PhoneBook) m_PhoneBook.reset(nullptr);
					break;
				}
				case IDC_BUTTON_HISTORY:
				{
					if(wstrLogin.length())
					{
						if(!m_History)
						{
							m_History = make_unique<CHistory>(); 
							m_History->_Start();
						}
						else m_History->_Show();
					}
					break;
				}
				case IDC_BUTTON_PHONEBOOK:
				{
					if(wstrLogin.length())
					{
						if(!m_PhoneBook) m_PhoneBook = make_unique<CPhoneBook>();
						m_PhoneBook->_Show();
					}
					break;
				}
				case ID_MIN_MENU_RESTORE:
				{
					RestoreFromTray();
					ShowNotifyIcon(FALSE);
					break;
				}
				case ID_MIN_MENU_ABOUT:
				{
					array<wchar_t, 256> wszAbout;
					swprintf_s(wszAbout.data(), wszAbout.size(), L"%S\r\nV.%S\r\nООО \"Омикрон\". Санкт-Петербург.", strProductName.c_str(), strProductVersion.c_str());
					MessageBox(hWnd, wszAbout.data(), L"О программе:", MB_ICONINFORMATION | MB_OK);
					break;
				}
				case ID_MIN_MENU_EXIT:
				{
					if(MessageBox(hWnd, L"Закрыть приложение?", L"SIP Phone", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						SendMessage(hDlgPhoneWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
					}
					bRet = TRUE;
					break;
				}
				case IDC_STATIC_REGSTATUS:
				{
					switch(HIWORD(wParam))
					{
						case STN_DBLCLK:
						{
							if(!wstrLogin.length()) DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PARAMS), hWnd, (ConfigDlgProc), 0);
							else
							{
								wstring wstrStatus;
								m_SIPProcess->_GetStatusString(wstrStatus);
								SetWindowText(hStatusInfo, wstrStatus.c_str());
							}
							break;
						}
					}
					break;
				}
				default:  return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			bRet = TRUE;
			break;
		}
		case WM_KEYDOWN:
		{
//			m_Log._LogWrite(L"WM_KEYDOWN=0x%0x",wParam);
			switch(wParam)
			{
				case VK_RETURN:
				{
					SendMessage(hBtDial, BM_CLICK, 0, 0);
					return true;
					break;
				}
				case '0': case VK_NUMPAD0: AddDigitToComboDial(hWnd, L'0');	break;
				case '1': case VK_NUMPAD1: AddDigitToComboDial(hWnd, L'1');	break;
				case '2': case VK_NUMPAD2: AddDigitToComboDial(hWnd, L'2');	break;
				case '3': case VK_NUMPAD3: AddDigitToComboDial(hWnd, bShift ? L'#' : L'3');	break;
				case '4': case VK_NUMPAD4: AddDigitToComboDial(hWnd, L'4');	break;
				case '5': case VK_NUMPAD5: AddDigitToComboDial(hWnd, L'5');	break;
				case '6': case VK_NUMPAD6: AddDigitToComboDial(hWnd, L'6');	break;
				case '7': case VK_NUMPAD7: AddDigitToComboDial(hWnd, L'7');	break;
				case '8': case VK_NUMPAD8: AddDigitToComboDial(hWnd, bShift ? L'*' : L'8');	break;
				case '9': case VK_NUMPAD9: AddDigitToComboDial(hWnd, L'9');	break;
				case VK_MULTIPLY:		   AddDigitToComboDial(hWnd, L'*');	break;
				case VK_SHIFT: bShift = true;  break;
				case 'p': case 'P': AddDigitToComboDial(hWnd, L',');	break;
				case 'w': case 'W': AddDigitToComboDial(hWnd, L';');	break;
				case VK_BACK:
				{
					std::array<wchar_t, 1024> vText; vText[0] = 0;
					auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data(), vText.size());
					if(iLen) vText[iLen - 1] = 0;
					SetWindowText(hComboBox, vText.data());
					break;
				}
			}
			break;
		}
		case WM_KEYUP:
		{
//			m_Log._LogWrite(L"WM_KEYUP=0x%0x", wParam);
			switch(wParam)
			{
				case VK_SHIFT: bShift = false;  break;
			}
			break;
		}
		case WM_SYSKEYDOWN:
		{
//			m_Log._LogWrite(L"WM_SYSKEYDOWN=0x%0x", wParam);
			switch(wParam)
			{
				case VK_RETURN:
				{
					SendMessage(hBtDial, BM_CLICK, 0, 0);
					return true;
					break;
				}
			}
			break;
		}
//		case WM_SYSKEYUP: m_Log._LogWrite(L"WM_SYSKEYUP"); break;
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
			SetButtonStyle(pDIS);
			return TRUE;
		}
		case WM_HSCROLL:
		{
			switch(LOWORD(wParam))
			{
				case TB_THUMBTRACK:
				case TB_ENDTRACK:
				{
					if((HWND)lParam == hSliderMic)
					{
						dwMicLevel = SendMessage(hSliderMic, TBM_GETPOS, 0, 0);
						if(dwMicLevel == ci_LevelMIN)	bMicrofonOn = false;
						else							bMicrofonOn = true;
						DrawButton(hBtMute, bMicrofonOn ? hBtMpMicON : hBtMpMicOFF);
						if(m_SIPProcess) m_SIPProcess->_Microfon(dwMicLevel);
					}
					else if((HWND)lParam == hSliderSnd)
					{
						dwSndLevel = SendMessage(hSliderSnd, TBM_GETPOS, 0, 0);
						if(dwSndLevel == ci_LevelMIN)	bSndOn = false;
						else							bSndOn = true;
						DrawButton(hBtSilence, bSndOn ? hBtMpSndON : hBtMpSndOFF);
						if(m_SIPProcess) m_SIPProcess->_Sound(dwSndLevel);
					}
					else return DefWindowProc(hWnd, uMsg, wParam, lParam);
					break;
				}
				default:  return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
			break;
		}
		case WM_TRAY_MESSAGE:
		{
			switch(lParam)
			{
				//case WM_LBUTTONDBLCLK:
				//	RestoreFromTray();
				//	// If we remove the icon here, the following WM_LBUTTONUP (the user
				//	// releasing the mouse button after a double click) can be sent to
				//	// the icon that occupies the position we were just using, which can
				//	// then activate, when the user doesn't want it to. So, set a flag
				//	// so that we remove the icon on the next WM_LBUTTONUP
				//	bHideIcon = true;
				//	return TRUE;

				case WM_LBUTTONUP:

					if(wstrLogin.length())
					{
						RestoreFromTray();
				  // The user has previously double-clicked the icon, remove it in
				  // response to this second WM_LBUTTONUP
//					if(bHideIcon)
						{
							ShowNotifyIcon(FALSE);
							bHideIcon = false;
						}
						return TRUE;
					}
					else DefWindowProc(hWnd, uMsg, wParam, lParam);
					break;
				case WM_CONTEXTMENU:
				case WM_RBUTTONDOWN:
				{
					POINT pt;
					GetCursorPos(&pt);
					auto hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU_MIN_N));
					hMenu = GetSubMenu(hMenu, 0);
					SetForegroundWindow(hDlgPhoneWnd);
					auto iRes = GetSystemMetrics(SM_MENUDROPALIGNMENT);
					TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | (iRes ? TPM_RIGHTALIGN : TPM_LEFTALIGN) | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
					break;
				}
				default: break;
			}
			break;
		}
		case WM_CLOSE:
		{
			SaveJSONMainParameters();

			if(m_History) { m_History->_Stop(1000); m_History.reset(nullptr); }
			if(m_PhoneBook) m_PhoneBook.reset(nullptr);

			ShowNotifyIcon(FALSE);
			if(m_WebSocketServer) m_WebSocketServer->_Stop(5000);
			if(m_SIPProcess) m_SIPProcess->_Stop(15000);

			DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;
		}
		case WM_DESTROY: PostQuitMessage(0); break;
		case WM_USER_REGISTER_DS:
		{
			PlaySound(NULL, NULL, 0);
			SetWindowText(hComboBox, L"");
			EnableWindow(hBtDial, true);
			EnableWindow(hBtDisconnect, false);
			bMicrofonOn = true;
			SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpMicON);
			bSndOn = true;
			SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpSndON);

			FLASHWINFO fInfo{ sizeof(FLASHWINFO) };
			fInfo.hwnd = hWnd;
			fInfo.dwFlags = FLASHW_STOP;
			fInfo.dwTimeout = 0;
			fInfo.uCount = 0;
			FlashWindowEx(&fInfo);

			DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);

			break;
		}
		case WM_USER_DIGIT:
		{
			if(m_SIPProcess->_DTMF((wchar_t)lParam)) Beep(450, 100);
			break;
		}
		case WM_USER_REGISTER_OC:
		{
			array<wchar_t, 256> wszCaption;
			swprintf_s(wszCaption.data(), wszCaption.size(), L"%s Зарегистрирован", wstrLogin.c_str());
			SetWindowText(hStatusInfo, wszCaption.data());
			break;
		}
		//case WM_USER_INITCALL:
		//{
		//	SetForegroundWindow(hDlgPhoneWnd);
		//	BringWindowToTop(hDlgPhoneWnd);
		//	ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
		//	break;
		//}
		case WM_USER_CALL_ANM:	m_SIPProcess->_Answer(true);		break;
		case WM_USER_CALL_DSC:	m_SIPProcess->_Disconnect(true);	break;
		case WM_USER_ACC_MOD:	m_SIPProcess->_Modify();			break;
		case WM_USER_MIC_LEVEL:
		{
			SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, dwMicLevel);
			if(dwMicLevel == ci_LevelMIN)	bMicrofonOn = false;
			else							bMicrofonOn = true;
			DrawButton(hBtMute, bMicrofonOn ? hBtMpMicON : hBtMpMicOFF);
			if(m_SIPProcess) m_SIPProcess->_Microfon(dwMicLevel, true);
			break;
		}
		case WM_USER_SND_LEVEL:
		{
			SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, dwSndLevel);
			if(dwSndLevel == ci_LevelMIN)	bSndOn = false;
			else							bSndOn = true;
			DrawButton(hBtSilence, bSndOn ? hBtMpSndON : hBtMpSndOFF);
			if(m_SIPProcess) m_SIPProcess->_Sound(dwSndLevel, true);
			break;
		}
		case WM_INFO_MESSAGE: { break; }
		case WM_USER_SIPWEB_ON:
		{
			return m_SIPProcess->_CreateWebAccount((const char*)wParam, (const char*)lParam);
			break;
		}
		case WM_USER_SIPWEB_OFF:
		{
			m_SIPProcess->_DeleteWebAccount();
			break;
		}
		case WM_USER_SIPWEB_MC:
		{
			if(!wParam)	bRet = m_SIPProcess->_MakeCall((const char*)lParam, true);
			else
			{
				bRet = m_SIPProcess->_MakeCall((const char*)lParam, false);
				wstring wstrNum;
				wstrNum.resize(strlen((const char*)lParam) + 1);
				swprintf_s(wstrNum.data(), wstrNum.size(), L"%S", (const char*)lParam);
				SetWindowText(hComboBox, wstrNum.data());
				auto iCount = SendMessage(hComboBox, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
				if(iCount != CB_ERR && iCount >= ci_HistoryMAX) SendMessage(hComboBox, (UINT)CB_DELETESTRING, (WPARAM)--iCount, (LPARAM)0);
				SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)0, (LPARAM)wstrNum.data());
			}
			break;
		}
		default:
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	return bRet;
}

void InitDialog()
{
	const wchar_t CLASS_NAME[] = L"CLOFF SIP phone";

	WNDCLASS wc = { };

	wc.lpfnWndProc = DlgProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(216, 216, 216)); //(HBRUSH)(COLOR_WINDOW + 1);
	RegisterClass(&wc);

	int iXOffset{ ci_MainXInterval };
	int iYOffset{ ci_MainYInterval };
	// Create the window.
	hDlgPhoneWnd = CreateWindow(CLASS_NAME, L"CLOFF SIP phone", WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX, iMainX, iMainY, ci_MainWidth, ci_MainHeight, 0, NULL, hInstance, NULL);

	auto hFntSmall = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	auto hFntBig = CreateFont(28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);

	//первый ряд 
	RECT rect;
	GetClientRect(hDlgPhoneWnd, &rect);
	hComboBox = CreateWindow(WC_COMBOBOX, L"", WS_TABSTOP | CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | CBS_SORT,
		iXOffset, iYOffset, rect.right - rect.left - ci_MainXInterval * 2, ci_MainComboBox_High, hDlgPhoneWnd, (HMENU)IDC_COMBO_DIAL, hInstance, NULL);
	SendMessage(hComboBox, WM_SETFONT, (WPARAM)hFntBig, (LPARAM)TRUE);
	GetClientRect(hComboBox, &rect);
	MapDialogRect(hComboBox, &rect);
	SetWindowPos(hComboBox, 0, 0, 0, rect.right - 2, (10 + 1) * rect.bottom, SWP_NOMOVE);

	//второй ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainComboBox_High + ci_MainYInterval;
	hBtDial = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainBigButton_Width, ci_MainBigButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_DIAL, hInstance, NULL);
	iXOffset += ci_MainBigButton_Width + ci_MainXInterval * 2;
	hBtDisconnect = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainBigButton_Width, ci_MainBigButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_DISCONNECT, hInstance, NULL);
	iXOffset += ci_MainBigButton_Width + ci_MainXInterval;
	hBtHistory = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_HISTORY, hInstance, NULL);

	//третий ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hBt1 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_1, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt2 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_2, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt3 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_3, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtBackSpace = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_BACKSPACE, hInstance, NULL);

	//четвёртый ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hBt4 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_4, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt5 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_5, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt6 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_6, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtConfig = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_CONFIG, hInstance, NULL);

	//пятый ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hBt7 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_7, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt8 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_8, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt9 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_9, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtPhoneBook = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_PHONEBOOK, hInstance, NULL);

	//пятый ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hBtAsterisk = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_ASTERISK, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBt0 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_0, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtDies = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_DIES, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtContactState = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_CONTACTSTATE, hInstance, NULL);

	//шестой ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hBtP = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_PAUSE, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
//	hBt0 = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_0, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;
	hBtW = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_WAIT, hInstance, NULL);
	iXOffset += ci_MainMiddleButton_Width + ci_MainXInterval;

	//седьмой ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hSliderMic = CreateWindow(TRACKBAR_CLASS, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_TOOLTIPS | TBS_BOTH | TBS_NOTICKS, iXOffset, iYOffset, ci_MainSlider_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_SLIDER_MIC, hInstance, NULL);
	SendMessage(hSliderMic, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
	SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, dwMicLevel);
	SendMessage(hSliderMic, TBM_SETTIPSIDE, TBTS_TOP, 0);
	GetClientRect(hSliderMic, &rect);
	auto hLeftValue = CreateWindow(WC_STATIC, L"0%", WS_VISIBLE | WS_CHILD | SS_LEFT, rect.left + 2, rect.bottom - 15, 20, 15, hSliderMic, (HMENU)IDC_STATIC_MIC_MIN, hInstance, NULL);
	auto hRightValue = CreateWindow(WC_STATIC, L"100%", WS_VISIBLE | WS_CHILD | SS_LEFT, rect.right - 35, rect.bottom - 15, 35, 15, hSliderMic, (HMENU)IDC_STATIC_MIC_MIN, hInstance, NULL);
	SendMessage(hLeftValue, WM_SETFONT, (WPARAM)hFntSmall, (LPARAM)TRUE);
	SendMessage(hRightValue, WM_SETFONT, (WPARAM)hFntSmall, (LPARAM)TRUE);

	iXOffset += ci_MainSlider_Width + ci_MainXInterval;
	hBtMute = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_MUTE, hInstance, NULL);

	//восьмой ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	hSliderSnd = CreateWindow(TRACKBAR_CLASS, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_NOTICKS | TBS_TRANSPARENTBKGND | TBS_TOOLTIPS | TBS_BOTH, iXOffset, iYOffset, ci_MainSlider_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_SLIDER_SND, hInstance, NULL);
	SendMessage(hSliderSnd, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
	SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, dwSndLevel);
	SendMessage(hSliderSnd, TBM_SETTIPSIDE, TBTS_TOP, 0);
	GetClientRect(hSliderSnd, &rect);
	hLeftValue = CreateWindow(WC_STATIC, L"0%", WS_VISIBLE | WS_CHILD | SS_LEFT, rect.left + 2, rect.bottom - 15, 20, 15, hSliderSnd, (HMENU)IDC_STATIC_MIC_MIN, hInstance, NULL);
	hRightValue = CreateWindow(WC_STATIC, L"100%", WS_VISIBLE | WS_CHILD | SS_LEFT, rect.right - 35, rect.bottom - 15, 35, 15, hSliderSnd, (HMENU)IDC_STATIC_MIC_MIN, hInstance, NULL);
	SendMessage(hLeftValue, WM_SETFONT, (WPARAM)hFntSmall, (LPARAM)TRUE);
	SendMessage(hRightValue, WM_SETFONT, (WPARAM)hFntSmall, (LPARAM)TRUE);

	iXOffset += ci_MainSlider_Width + ci_MainXInterval;
	hBtSilence = CreateWindow(WC_BUTTON, L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, iXOffset, iYOffset, ci_MainMiddleButton_Width, ci_MainMiddleButton_High, hDlgPhoneWnd, (HMENU)IDC_BUTTON_SILENCE, hInstance, NULL);

	//девятый ряд
	iXOffset = ci_MainXInterval;
	iYOffset += ci_MainBigButton_High + ci_MainYInterval;
	GetClientRect(hDlgPhoneWnd, &rect);
	hStatusInfo = CreateWindow(WC_STATIC, L"", WS_VISIBLE | WS_CHILD | SS_CENTER, iXOffset, iYOffset, rect.right - rect.left - ci_MainXInterval * 2, ci_MainComboBox_High, hDlgPhoneWnd, (HMENU)IDC_STATIC_REGSTATUS, hInstance, NULL);
	SendMessage(hStatusInfo, WM_SETFONT, (WPARAM)hFntSmall, (LPARAM)TRUE);

	hBtMpMicON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICON));
	hBtMpMicOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICOFF));

	hBtMpSndON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDON));
	hBtMpSndOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDOFF));

	EnableWindow(hBtContactState, FALSE);
	auto mtt = CreateToolTip(hBtConfig, hDlgPhoneWnd, (PTSTR)TEXT("Настройки"));

	if(!wstrLogin.length()) SetWindowText(hStatusInfo, L"Настройте аккаунт или воспользуйтесь web кабинетом CLOFF!");

	m_SIPProcess = std::make_unique<CSIPProcess>(hDlgPhoneWnd);
	m_SIPProcess->_Start();

	m_WebSocketServer = make_unique<CWebSocket>();
	m_WebSocketServer->_Start();

	if(bMinimizeOnStart)
	{
		MinimizeToTray();
		ShowNotifyIcon(TRUE);
	}
	else
	{
		SetForegroundWindow(hDlgPhoneWnd);
		SetActiveWindow(hDlgPhoneWnd);
		ShowWindow(hDlgPhoneWnd, SW_SHOWNORMAL);
	}

	m_Log._LogWrite(L"Init End");
}

void DrawButton(HWND hCurrentButton, HBITMAP hBitmap)
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

void SetButtonStyle(LPDRAWITEMSTRUCT& pDIS)
{
	HBITMAP hBitmap{ nullptr };

	switch(pDIS->itemAction)
	{
		case ODA_SELECT:
		{
			if(pDIS->hwndItem == hBtDial)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ACCEPT_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ACCEPT_N)));
			else if(pDIS->hwndItem == hBtDisconnect)	DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CANCEL_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CANCEL_N)));
			else if(pDIS->hwndItem == hBtHistory)		DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_HISTORY_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_HISTORY)));
			else if(pDIS->hwndItem == hBt1)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_1_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_1)));
			else if(pDIS->hwndItem == hBt2)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_2_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_2)));
			else if(pDIS->hwndItem == hBt3)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_3_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_3)));
			else if(pDIS->hwndItem == hBtBackSpace)		DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_BS_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_BS)));
			else if(pDIS->hwndItem == hBt4)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_4_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_4)));
			else if(pDIS->hwndItem == hBt5)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_5_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_5)));
			else if(pDIS->hwndItem == hBt6)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_6_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_6)));
			else if(pDIS->hwndItem == hBtConfig)		DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONFIG_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONFIG)));
			else if(pDIS->hwndItem == hBt7)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_7_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_7)));
			else if(pDIS->hwndItem == hBt8)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_8_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_8)));
			else if(pDIS->hwndItem == hBt9)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_9_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_9)));
			else if(pDIS->hwndItem == hBtPhoneBook)		DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_PHONEBOOK_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_PHONEBOOK)));
			else if(pDIS->hwndItem == hBtAsterisk)		DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ASTERISK_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ASTERISK)));
			else if(pDIS->hwndItem == hBt0)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_0_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_0)));
			else if(pDIS->hwndItem == hBtDies)			DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_DIES_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_DIES)));
			else if(pDIS->hwndItem == hBtContactState)	DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONTACTSTATE)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONTACTSTATE)));
			else if(pDIS->hwndItem == hBtP)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_P_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_P)));
			else if(pDIS->hwndItem == hBtW)				DrawButton(pDIS->hwndItem, pDIS->itemState & ODS_SELECTED ? LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_W_GR)) : LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_W)));
			else if(pDIS->hwndItem == hBtMute)
			{
				if(pDIS->itemState & ODS_SELECTED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICON_GR)));
			}
			else if(pDIS->hwndItem == hBtSilence)
			{
				if(pDIS->itemState & ODS_SELECTED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDON_GR)));
			}
			break;
		}
		case ODA_DRAWENTIRE:
		{
			if(pDIS->hwndItem == hBtDial)
			{
				if(pDIS->itemState & ODS_DISABLED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ACCEPT_BL)));
				else DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ACCEPT_N)));
			}
			else if(pDIS->hwndItem == hBtDisconnect)
			{
				if(pDIS->itemState & ODS_DISABLED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CANCEL_DIS)));
				else DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CANCEL_N)));
			}
			else if(pDIS->hwndItem == hBtHistory)
			{
				if(pDIS->itemState & ODS_DISABLED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_HISTORY_DIS)));
				else DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_HISTORY)));
			}
			else if(pDIS->hwndItem == hBt1)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_1)));
			else if(pDIS->hwndItem == hBt2)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_2)));
			else if(pDIS->hwndItem == hBt3)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_3)));
			else if(pDIS->hwndItem == hBtBackSpace)		DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_BS)));
			else if(pDIS->hwndItem == hBt4)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_4)));
			else if(pDIS->hwndItem == hBt5)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_5)));
			else if(pDIS->hwndItem == hBt6)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_6)));
			else if(pDIS->hwndItem == hBtConfig)		DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONFIG)));
			else if(pDIS->hwndItem == hBt7)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_7)));
			else if(pDIS->hwndItem == hBt8)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_8)));
			else if(pDIS->hwndItem == hBt9)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_9)));
			else if(pDIS->hwndItem == hBtPhoneBook)
			{
				if(pDIS->itemState & ODS_DISABLED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_PHONEBOOK_DIS)));
				else DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_PHONEBOOK)));
			}
			else if(pDIS->hwndItem == hBtAsterisk)		DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ASTERISK)));
			else if(pDIS->hwndItem == hBt0)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_0)));
			else if(pDIS->hwndItem == hBtDies)			DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_DIES)));
			else if(pDIS->hwndItem == hBtContactState)
			{
				if(pDIS->itemState & ODS_DISABLED) DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONTACTSTATE_DIS)));
				else DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONTACTSTATE)));
			}
			else if(pDIS->hwndItem == hBtP)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_P)));
			else if(pDIS->hwndItem == hBtW)				DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_W)));
			else if(pDIS->hwndItem == hBtMute)			DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, bMicrofonOn ? MAKEINTRESOURCE(IDB_BITMAP_MICON) : MAKEINTRESOURCE(IDB_BITMAP_MICOFF)));
			else if(pDIS->hwndItem == hBtSilence)		DrawButton(pDIS->hwndItem, LoadBitmap(hInstance, bSndOn ? MAKEINTRESOURCE(IDB_BITMAP_SNDON) : MAKEINTRESOURCE(IDB_BITMAP_SNDOFF)));
			break;
		}
		case ODA_FOCUS:
		{
			break;
		}
	}
}

bool GetProductAndVersion()
{
	// get the filename of the executable containing the version resource
	TCHAR moduleName[MAX_PATH + 1];
	GetModuleFileName(NULL, moduleName, MAX_PATH);
	DWORD dummyZero;
	DWORD versionSize = GetFileVersionInfoSize(moduleName, &dummyZero);
	if(versionSize == 0) return false;

	std::vector<BYTE> data; data.resize(versionSize);
	if(GetFileVersionInfo(moduleName, NULL, versionSize, data.data()))
	{
		UINT length;

		struct LANGANDCODEPAGE
		{
			WORD wLanguage;
			WORD wCodePage;
		} *lpTranslate;

		// Read the list of languages and code pages.

		VerQueryValue(data.data(), L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &length);

// Read the file description for each language and code page.

		std::array<wchar_t, 256> SubBlock;
		LPVOID lpBuffer;
		swprintf_s(SubBlock.data(), SubBlock.size(), L"\\StringFileInfo\\%04x%04x\\ProductName", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
		UINT pnlength;
		if(VerQueryValue(data.data(), SubBlock.data(), (LPVOID*)&lpBuffer, &pnlength))
		{
			std::array<char, 256> szName;
			sprintf_s(szName.data(), szName.size(), "%ls", static_cast<wchar_t*>(lpBuffer));
			strProductName = szName.data();
		}
		else strProductName = "CLOFF SIP Phone by Omicron";

		VS_FIXEDFILEINFO* pFixInfo;
		VerQueryValue(data.data(), TEXT("\\"), (LPVOID*)&pFixInfo, &length);
		strProductVersion = std::to_string(HIWORD(pFixInfo->dwProductVersionMS)) + "." +
			std::to_string(LOWORD(pFixInfo->dwProductVersionMS)) + "." +
			std::to_string(HIWORD(pFixInfo->dwProductVersionLS)) + "." +
			std::to_string(LOWORD(pFixInfo->dwProductVersionLS));
	}

	return true;
}

static VOID ShowNotifyIcon(BOOL bAdd)
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hDlgPhoneWnd;
	nid.uID = 0;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAY_MESSAGE;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	array<wchar_t, 256> wsMsg;
	if(wstrLogin.length())	swprintf_s(wsMsg.data(), wsMsg.size(), L"CloffSIP %s\r\nclick to open", wstrLogin.c_str());
	else swprintf_s(wsMsg.data(), wsMsg.size(), L"CloffSIP \r\nWeb mode!");
	lstrcpy(nid.szTip, wsMsg.data());

	if(bAdd) Shell_NotifyIcon(NIM_ADD, &nid);
	else	 Shell_NotifyIcon(NIM_DELETE, &nid);
}

void MinimizeToTray()
{
	if(m_History) m_History->_Hide();
	if(m_PhoneBook) m_PhoneBook->_Hide();
	ShowWindow(hDlgPhoneWnd, SW_HIDE);
}

void RestoreFromTray()
{
// Show the window, and make sure we're the foreground window
	ShowWindow(hDlgPhoneWnd, SW_SHOW);
	SetActiveWindow(hDlgPhoneWnd);
	SetForegroundWindow(hDlgPhoneWnd);
//m_Log._LogWrite(L"RestoreFromTray");
	BringWindowToTop(hDlgPhoneWnd);
}

// Description:
//   Creates a tooltip for an item in a dialog box. 
// Parameters:
//   idTool - identifier of an dialog box item.
//   nDlg - window handle of the dialog box.
//   pszText - string to use as the tooltip text.
// Returns:
//   The handle to the tooltip.
//
HWND CreateToolTip(HWND hwndTool, HWND hDlg, PTSTR pszText)
{
	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindow(TOOLTIPS_CLASS, (LPCWSTR)NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, (HMENU)NULL, hInstance, NULL);

	if(!hwndTip) return (HWND)NULL;

	// Associate the tooltip with the tool.
	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = (LPWSTR)L"Настройки";//pszText;
	if(!SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo))
	{
		DestroyWindow(hwndTip);
		return NULL;
	}
	SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	return hwndTip;
}

void LoadMainParameters(unique_ptr<CConfigFile>& pCfg)
{
	wstring wstrBuf;

	iMainX = pCfg->GetIntParameter("MAIN", "X", 0);
	iMainY = pCfg->GetIntParameter("MAIN", "Y", 0);

	iTransportType = pCfg->GetIntParameter("MAIN", "Transport", IPPROTO_TCP);

	pCfg->GetStringParameter("MAIN", "DTMF", wstrBuf);
	if(!wstrBuf.compare(L"RFC2833")) bDTMF_2833 = true;
	else bDTMF_2833 = false;

	uiRingToneNumber = pCfg->GetIntParameter("MAIN", "RingNum", 0);
	dwMicLevel = pCfg->GetIntParameter("MAIN", "MicLevel", ci_LevelMAX);
	dwSndLevel = pCfg->GetIntParameter("MAIN", "SndLevel", ci_LevelMAX);
	bMinimizeOnStart = pCfg->GetIntParameter("MAIN", "MinOnStart", 0) ? true : false;
	bAutoStart = pCfg->GetIntParameter("MAIN", "Autostart", 0) ? true : false;
	pCfg->GetStringParameter("MAIN", "WS", wstrWEBServer);

	pCfg->GetCodedStringParameter("MAIN", "DataStart", wstrBuf);
	if(wstrBuf.size())
	{
		auto iCurrPars = 0;
		auto iNextPars = wstrBuf.find(L'\n');
		if(iNextPars == wstring::npos) return;
		auto wstrPCName = wstrBuf.substr(iCurrPars, iNextPars - iCurrPars);
		if(wstrPCName.length())
		{
			//CompName
			vector<char> vCN(wstrPCName.length() + 1);
			WideCharToMultiByte(1251, WC_COMPOSITECHECK, wstrPCName.c_str(), -1, vCN.data(), vCN.size(), NULL, NULL);

			if(!strcmp(szCompName.data(), vCN.data()))
			{
				iCurrPars = iNextPars + 1;
				iNextPars = wstrBuf.find(L'\n', iCurrPars);
				if(iNextPars != wstring::npos)
				{
					//Login
					wstrLogin = wstrBuf.substr(iCurrPars, iNextPars - iCurrPars);

					iCurrPars = iNextPars + 1;
					iNextPars = wstrBuf.find(L'\n', iCurrPars);
					if(iNextPars != wstring::npos)
					{
						//Password
						wstrPassword = wstrBuf.substr(iCurrPars, iNextPars - iCurrPars);
						iCurrPars = iNextPars + 1;
						iNextPars = wstrBuf.find(L'\n', iCurrPars);
						if(iNextPars != wstring::npos)
						{
							//Server
							wstrServer = wstrBuf.substr(iCurrPars, iNextPars - iCurrPars);
						}
					}
				}
			}
		}
	}
}

int CheckCopyStart()
{
	int iRes = 0;

	auto hWndRes = FindWindow(NULL, L"CLOFF SIP phone");
	if(hWndRes)
	{
		MessageBox(NULL, L"Приложение уже запущено!", L"SIP Phone", MB_OK | MB_ICONEXCLAMATION);
		iRes = 1;
	}
	return iRes;
}

char szDefault_key[] = "3igcZhRdWq01M3G4mTAiv9";

bool PutCodedStringParameter(const char* szValue, size_t stValueLength, string& strCoded)
{
	bool bRet = false;
	size_t stCount = stValueLength;
	vector<BYTE> szData((stCount / CHUNK_SIZE + 1) * CHUNK_SIZE);
	//CompName
	memcpy_s(szData.data(), szData.size(), szValue, stCount);

	char* key_str = szDefault_key;
	char info[] = "Microsoft Enhanced RSA and AES Cryptographic Provider";
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HCRYPTKEY hKey;

	if(!CryptAcquireContextA(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) m_Log._LogWrite(L"CryptAcquireContext failed: %#x", GetLastError());
	else
	{
		if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))	m_Log._LogWrite(L"CryptCreateHash failed: %#x", GetLastError());
		else
		{
			if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenA(key_str), 0))	m_Log._LogWrite(L"CryptHashData Failed : %#x", GetLastError());
			else
			{
				if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) m_Log._LogWrite(L"CryptDeriveKey failed: %#x", GetLastError());
				else
				{
					DWORD out_len = stCount;
					if(!CryptEncrypt(hKey, NULL, TRUE, 0, szData.data(), &out_len, szData.size()))
					{
						long long dwStatus = GetLastError();
						switch(dwStatus)
						{
							case ERROR_INVALID_HANDLE:		m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		 break;
							case ERROR_INVALID_PARAMETER:	m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	 break;
							case NTE_BAD_ALGID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_ALGID");				 break;
							case NTE_BAD_DATA:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_DATA");				 break;
							case NTE_BAD_FLAGS:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_FLAGS");				 break;
							case NTE_BAD_HASH:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_HASH");				 break;
							case NTE_BAD_KEY:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_KEY");				 break;
							case NTE_BAD_LEN:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_LEN");				 break;
							case NTE_BAD_UID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_UID");				 break;
							case NTE_DOUBLE_ENCRYPT:		m_Log._LogWrite(L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");			 break;
							case NTE_FAIL:					m_Log._LogWrite(L"CryptEncrypt failed err=NTE_FAIL");					 break;
							default:						m_Log._LogWrite(L"CryptEncrypt failed err=%d(%#x)", dwStatus, dwStatus); break;
						}
					}
					else
					{
						strCoded.clear();
						strCoded.resize(out_len * 2 + 1);
						for(DWORD i = 0; i < out_len; ++i)
						{
							auto size = strCoded.size() - strlen(strCoded.c_str());
							sprintf_s(&strCoded[i * 2], strCoded.size() - strlen(strCoded.c_str()), "%02x", szData[i]);
						}
						bRet = true;
					}
					CryptDestroyKey(hKey);
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return bRet;
}

void SaveJSONMainParameters()
{
	auto hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	rapidjson::Document cfg;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL)
	{
		m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
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
			m_Log._LogWrite(L"Main: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
			cfg.Clear();
		}
		::fclose(fCfgFile);
	}
	auto& allocator = cfg.GetAllocator();

	if(!cfg.IsObject()) cfg.SetObject();
	if(!cfg.HasMember(MAIN_OBJ_NAME))
	{
		rapidjson::Value main_obj(rapidjson::kObjectType);
		cfg.AddMember(MAIN_OBJ_NAME, main_obj, allocator);
	}

	RECT sCoord{ 0 };
	GetWindowRect(hDlgPhoneWnd, &sCoord);

	rapidjson::Value vVal;

	if(!cfg[MAIN_OBJ_NAME].HasMember(X_COORD_NAME)) cfg[MAIN_OBJ_NAME].AddMember(X_COORD_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][X_COORD_NAME].SetInt(sCoord.left);

	if(!cfg[MAIN_OBJ_NAME].HasMember(Y_COORD_NAME)) cfg[MAIN_OBJ_NAME].AddMember(Y_COORD_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][Y_COORD_NAME].SetInt(sCoord.top);

	if(!cfg[MAIN_OBJ_NAME].HasMember(W_WIDTH_NAME)) cfg[MAIN_OBJ_NAME].AddMember(W_WIDTH_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][W_WIDTH_NAME].SetInt(sCoord.right - sCoord.left);

	if(!cfg[MAIN_OBJ_NAME].HasMember(H_HIGH_NAME)) cfg[MAIN_OBJ_NAME].AddMember(H_HIGH_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][H_HIGH_NAME].SetInt(sCoord.bottom - sCoord.top);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_TRANSPORT_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_TRANSPORT_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_TRANSPORT_NAME].SetInt(iTransportType);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_DTMF_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_DTMF_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_DTMF_NAME].SetString(bDTMF_2833 ? rapidjson::StringRef("RFC2833") : rapidjson::StringRef("INFO"));

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_RING_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_RING_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_RING_NAME].SetUint(uiRingToneNumber);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_MIC_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_MIC_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_MIC_NAME].SetUint(dwMicLevel);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_SND_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_SND_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_SND_NAME].SetUint(dwSndLevel);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_MINONSTART_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_MINONSTART_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_MINONSTART_NAME].SetBool(bMinimizeOnStart);

	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_AUTOSTART_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_AUTOSTART_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_AUTOSTART_NAME].SetBool(bAutoStart);

	string strBuffer;
	strBuffer.resize(wstrWEBServer.length() + 1);
	sprintf_s(strBuffer.data(), strBuffer.size(), "%ls", wstrWEBServer.c_str());
	if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_WS_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_WS_NAME, vVal, allocator);
	cfg[MAIN_OBJ_NAME][MAIN_WS_NAME].SetString(strBuffer.data(), strBuffer.length(), allocator);

	string strData(szCompName.data());
	strBuffer.resize(wstrLogin.length() + 1);
	sprintf_s(strBuffer.data(), strBuffer.size(), "%ls", wstrLogin.c_str());
	strData = strData + "\n" + strBuffer;
	strBuffer.resize(wstrPassword.length() + 1);
	sprintf_s(strBuffer.data(), strBuffer.size(), "%ls", wstrPassword.c_str());
	strData = strData + "\n" + strBuffer;
	strBuffer.resize(wstrServer.length() + 1);
	sprintf_s(strBuffer.data(), strBuffer.size(), "%ls", wstrServer.c_str());
	strData = strData + "\n" + strBuffer + "\n";

	if(PutCodedStringParameter(strData.c_str(), strData.length(), strBuffer))
	{
		if(!cfg[MAIN_OBJ_NAME].HasMember(MAIN_DATA_NAME)) cfg[MAIN_OBJ_NAME].AddMember(MAIN_DATA_NAME, vVal, allocator);
		cfg[MAIN_OBJ_NAME][MAIN_DATA_NAME].SetString(strBuffer.data(), strBuffer.length(), allocator);
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
	else m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);

	SetCursor(hPrevCursor);
}


size_t GetCodedStringParameter(const char* szValue, string& strResult)
{
	strResult.clear();

	vector<BYTE> vData;
	auto len = strlen(szValue);
	BYTE bVar{ 0 };
	for(UINT i = 0; i < len; i += 2)
	{
		unsigned int uiVar{ 0 };
		auto iRes = sscanf(&szValue[i], "%02x", &uiVar);
		vData.push_back(uiVar);
	}

	char* key_str = szDefault_key;
	char info[] = "Microsoft Enhanced RSA and AES Cryptographic Provider";
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HCRYPTKEY hKey;

	if(!CryptAcquireContextA(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) m_Log._LogWrite(L"CryptAcquireContext failed: %#x", GetLastError());
	else
	{
		if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))	m_Log._LogWrite(L"CryptCreateHash failed: %#x", GetLastError());
		else
		{
			if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenA(key_str), 0))	m_Log._LogWrite(L"CryptHashData Failed : %#x", GetLastError());
			else
			{
				if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) m_Log._LogWrite(L"CryptDeriveKey failed: %#x", GetLastError());
				else
				{
					DWORD out_len = vData.size();
					if(!CryptDecrypt(hKey, NULL, TRUE, 0, vData.data(), &out_len))
					{
						long long dwStatus = GetLastError();
						switch(dwStatus)
						{
							case ERROR_INVALID_HANDLE:		m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		break;
							case ERROR_INVALID_PARAMETER:	m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	break;
							case NTE_BAD_ALGID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_ALGID");				break;
							case NTE_BAD_DATA:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_DATA");				break;
							case NTE_BAD_FLAGS:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_FLAGS");				break;
							case NTE_BAD_HASH:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_HASH");				break;
							case NTE_BAD_KEY:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_KEY");				break;
							case NTE_BAD_LEN:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_LEN");				break;
							case NTE_BAD_UID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_UID");				break;
							case NTE_DOUBLE_ENCRYPT:		m_Log._LogWrite(L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");			break;
							case NTE_FAIL:					m_Log._LogWrite(L"CryptEncrypt failed err=NTE_FAIL");					break;
							default:						m_Log._LogWrite(L"CryptEncrypt failed err=%d(%x)", dwStatus, dwStatus); break;
						}
					}
					else
					{
						strResult.append(reinterpret_cast<char*>(vData.data()), out_len);
					}
					CryptDestroyKey(hKey);
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return strResult.length();
}

void LoadJSONMainParameters()
{
	string strBuf;
	int w{ ci_PhBWidth }, h{ ci_PhBHeight };

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
			if(cfg.HasMember(MAIN_OBJ_NAME) && cfg[MAIN_OBJ_NAME].IsObject())
			{
				auto main_obj = cfg[MAIN_OBJ_NAME].GetObject();
				if(main_obj.HasMember(X_COORD_NAME) && main_obj[X_COORD_NAME].IsInt()) iMainX = main_obj[X_COORD_NAME].GetInt();
				if(main_obj.HasMember(Y_COORD_NAME) && main_obj[Y_COORD_NAME].IsInt()) iMainY = main_obj[Y_COORD_NAME].GetInt();
				if(main_obj.HasMember(W_WIDTH_NAME) && main_obj[W_WIDTH_NAME].IsInt()) w = main_obj[W_WIDTH_NAME].GetInt();
				if(main_obj.HasMember(H_HIGH_NAME) && main_obj[H_HIGH_NAME].IsInt())  h = main_obj[H_HIGH_NAME].GetInt();

				if(main_obj.HasMember(MAIN_TRANSPORT_NAME) && main_obj[MAIN_TRANSPORT_NAME].IsInt())  iTransportType = main_obj[MAIN_TRANSPORT_NAME].GetInt();
				if(main_obj.HasMember(MAIN_RING_NAME) && main_obj[MAIN_RING_NAME].IsUint())  uiRingToneNumber = main_obj[MAIN_RING_NAME].GetUint();
				if(main_obj.HasMember(MAIN_MIC_NAME) && main_obj[MAIN_MIC_NAME].IsUint())  dwMicLevel = main_obj[MAIN_MIC_NAME].GetUint();
				if(main_obj.HasMember(MAIN_SND_NAME) && main_obj[MAIN_SND_NAME].IsUint())  dwSndLevel = main_obj[MAIN_SND_NAME].GetUint();
				if(main_obj.HasMember(MAIN_MINONSTART_NAME) && main_obj[MAIN_MINONSTART_NAME].IsBool())  bMinimizeOnStart = main_obj[MAIN_MINONSTART_NAME].GetBool(); 
				if(main_obj.HasMember(MAIN_AUTOSTART_NAME) && main_obj[MAIN_AUTOSTART_NAME].IsBool())    bAutoStart = main_obj[MAIN_AUTOSTART_NAME].GetBool(); 
				if(main_obj.HasMember(MAIN_DTMF_NAME) && main_obj[MAIN_DTMF_NAME].IsString())  bDTMF_2833 = !strcmp(main_obj[MAIN_DTMF_NAME].GetString(), "RFC2833") ? true : false;
				if(!wstrLogin.length())
				{
					if(main_obj.HasMember(MAIN_DATA_NAME) && main_obj[MAIN_DATA_NAME].IsString())
					{
						if(GetCodedStringParameter(main_obj[MAIN_DATA_NAME].GetString(), strBuf))
						{
							array<wchar_t, 256> wszWBUF{ 0 };
							auto iCurrPars = 0;
							auto iNextPars = strBuf.find('\n');
							if(iNextPars == string::npos) return;
							auto strPCName = strBuf.substr(iCurrPars, iNextPars - iCurrPars);
							if(strPCName.length())
							{
								//CompName
								if(!strcmp(szCompName.data(), strPCName.c_str()))
								{
									iCurrPars = iNextPars + 1;
									iNextPars = strBuf.find('\n', iCurrPars);
									if(iNextPars != string::npos)
									{
										//Login
										MultiByteToWideChar(1251, 0, &strBuf[iCurrPars], iNextPars - iCurrPars, wszWBUF.data(), wszWBUF.size());
										wstrLogin = wszWBUF.data();

										iCurrPars = iNextPars + 1;
										iNextPars = strBuf.find('\n', iCurrPars);
										if(iNextPars != string::npos)
										{
											//Password
											MultiByteToWideChar(1251, 0, &strBuf[iCurrPars], iNextPars - iCurrPars, wszWBUF.data(), wszWBUF.size());
											wstrPassword = wszWBUF.data();
											iCurrPars = iNextPars + 1;
											iNextPars = strBuf.find(L'\n', iCurrPars);
											if(iNextPars != string::npos)
											{
												//Server
												MultiByteToWideChar(1251, 0, &strBuf[iCurrPars], iNextPars - iCurrPars, wszWBUF.data(), wszWBUF.size());
												wstrServer = wszWBUF.data();
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else m_Log._LogWrite(L"   PhoneBook: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
		::fclose(fCfgFile);
	}
}

void CorrectBoolParameter(const char* szParamName,bool bValue)
{
	rapidjson::Document cfg;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL)
	{
		m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
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
			m_Log._LogWrite(L"Main: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
			cfg.Clear();
		}
		::fclose(fCfgFile);
	}
	auto& allocator = cfg.GetAllocator();

	if(!cfg.IsObject()) cfg.SetObject();
	if(!cfg.HasMember(MAIN_OBJ_NAME))
	{
		rapidjson::Value main_obj(rapidjson::kObjectType);
		cfg.AddMember(MAIN_OBJ_NAME, main_obj, allocator);
	}

	rapidjson::Value vVal;

	if(!cfg[MAIN_OBJ_NAME].HasMember(szParamName)) cfg[MAIN_OBJ_NAME].AddMember(rapidjson::StringRef(szParamName), vVal, allocator);
	cfg[MAIN_OBJ_NAME][szParamName].SetBool(bValue);

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
	else m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
}

void CorrectStringParameter(const char* szParamName, const wchar_t* wszValue)
{
	rapidjson::Document cfg;

	auto fCfgFile = fopen("cloff_sip_phone.cfg", "r");
	if(fCfgFile == NULL)
	{
		m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
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
			m_Log._LogWrite(L"Main: ошибка JSON файла конфигурации cloff_sip_phone.cfg. error=%d", cfg.GetParseError());
			cfg.Clear();
		}
		::fclose(fCfgFile);
	}
	auto& allocator = cfg.GetAllocator();

	if(!cfg.IsObject()) cfg.SetObject();
	if(!cfg.HasMember(MAIN_OBJ_NAME))
	{
		rapidjson::Value main_obj(rapidjson::kObjectType);
		cfg.AddMember(MAIN_OBJ_NAME, main_obj, allocator);
	}

	rapidjson::Value vVal;
	array<char, 1024> sString;

	sprintf_s(sString.data(), sString.size(), "%ls", wszValue);
	vVal.SetString(sString.data(), strlen(sString.data()), allocator);

	if(!cfg[MAIN_OBJ_NAME].HasMember(szParamName)) cfg[MAIN_OBJ_NAME].AddMember(rapidjson::StringRef(szParamName), vVal, allocator);
	else cfg[MAIN_OBJ_NAME][szParamName].SetString(sString.data(),allocator);

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
	else m_Log._LogWrite(L"Main: ошибка открытия файла конфигурации cloff_sip_phone.cfg. error=%d", errno);
}
void SetAutostart(bool bSet)
{
	if(bSet)
	{
		HKEY hkey = NULL;
		HRESULT hres = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
		hres = RegSetValueEx(hkey, L"CLOFF SIP Phone", 0, REG_SZ, (BYTE*)csEXEPath.data(), (wcslen(csEXEPath.data()) + 1) * 2);
		RegCloseKey(hkey);
	}
	else
	{
		HKEY hKey;
		long lReturn = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0L, KEY_ALL_ACCESS, &hKey);
		lReturn = RegDeleteValue(hKey, (LPCWSTR)L"CLOFF SIP Phone");
		lReturn = RegCloseKey(hKey);
	}

}