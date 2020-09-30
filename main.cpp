#include <winsock2.h>
#include <Windows.h>
#include <Windowsx.h>
#include <conio.h>
#include <string>
#include <vector>
#include "resource.h"
#include "Main.h"
#include <wincrypt.h>
#include <shellapi.h>
#include "CWebSocket.h"
#include "History.h"
#include "Config.h"

std::unique_ptr<CSIPProcess> m_SIPProcess;
std::unique_ptr<CWebSocket> m_WebSocketServer;
std::unique_ptr<CHistory> m_History;
in_port_t ipWSPort = 20005;
string strProductName;
string strProductVersion;
array<wchar_t, 1024> csEXEPath;

bool bWebMode{ false };			/** признак запуска из веб*/
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

HWND hComboBox{ 0 };
HWND hStatusInfo{ 0 };
HWND hDlgPhoneWnd{ 0 };
HWND hEditServer{ 0 };
HWND hTCP_RB{ 0 };
HWND hUDP_RB{ 0 };
HWND hDTMF_RFC2833_RB{ 0 };
HWND hDTMF_INFO_RB{ 0 };
HWND hBtMute{ 0 };
HWND hBtSilence{ 0 };
HWND hBtConfig{ 0 };
HWND hBtBackSpace{ 0 };
HWND hBtDial{ 0 };
HWND hBtDisconnect{ 0 };

HINSTANCE hInstance{ 0 };

list<wstring>	lSubscriberAccounts;

array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName{ 0 };

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitDialog();

int CALLBACK WinMain(HINSTANCE curInst, HINSTANCE hPrev, LPSTR lpstrCmdLine, int nCmdShow)
{
	hInstance = curInst;

	GetModuleFileName(NULL, csEXEPath.data(), csEXEPath.size());
	//array<wchar_t, MAX_PATH + 1> wszAppData;
	//auto dwLen = GetEnvironmentVariable(L"AppData", wszAppData.data(), MAX_PATH);
	//if(dwLen > 0 && dwLen < MAX_PATH)
	//{
	//	swprintf_s(&wszAppData[wcslen(wszAppData.data())], MAX_PATH - wcslen(wszAppData.data()), L"\\Cloff");
	//	if(!CreateDirectory(wszAppData.data(), NULL))
	//	{
	//		auto err = GetLastError();
	//		if(err != ERROR_ALREADY_EXISTS) wszAppData[0] = 0;
	//	}
	//	if(wcslen(wszAppData.data())) SetCurrentDirectory(wszAppData.data());
	//}

	m_Log._Start();

	GetProductAndVersion();

	m_Log._LogWrite(L"---------------------------------------------------------------");
	m_Log._LogWrite(L"Main: *************** New Start  V.%S key=%S ****************", strProductVersion.c_str(), lpstrCmdLine);

	if(strstr(lpstrCmdLine, "-debug")) bDebugMode = true;
	if(strstr(lpstrCmdLine, "-config")) bConfigMode = true;
	if(strstr(lpstrCmdLine, "-default")) DeleteFile(L"cloff_sip_phone.cfg");
	if(strstr(lpstrCmdLine, "-web")) bWebMode = true;
	if(strstr(lpstrCmdLine, "-help") || strstr(lpstrCmdLine, "-h") || strstr(lpstrCmdLine, "\\help") || strstr(lpstrCmdLine, "\\h"))
	{
		printf("-debug\r\n-config\r\n-default\r\n-help\r\n");
		while(!_kbhit());
	}
	else
	{
		DWORD dwLen = MAX_COMPUTERNAME_LENGTH;
		if(!GetComputerNameA((LPSTR)szCompName.data(), &dwLen)) sprintf_s(szCompName.data(), szCompName.size(), "Undefined");

		DialogBoxParam(curInst, MAKEINTRESOURCE(IDD_DIALOG_PHONE), 0, (DlgProc), 0);
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
	SetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data());
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

BOOL CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			hDlgPhoneWnd = hWnd;
			InitDialog();

			bRet = TRUE;
			break;
		}
		case WM_SYSCOMMAND:
		{
			if(wParam == SC_MINIMIZE)
			{
				MinimizeToTray();
				ShowNotifyIcon(TRUE);
				// Return TRUE to tell DefDlgProc that we handled the message, and set
				// DWL_MSGRESULT to 0 to say that we handled WM_SYSCOMMAND
				SetWindowLong(hWnd, DWL_MSGRESULT, 0);
				return TRUE;
			}
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDABORT:
				{
					auto pcCfg = make_unique<CConfigFile>(true);
					SaveMainParameters(pcCfg);
					if(m_History) m_History->_SaveParameters(pcCfg);
					m_Log._LogWrite(L"IDABORT");

					ShowNotifyIcon(FALSE);
					if(m_WebSocketServer) m_WebSocketServer->_Stop(5000);
					EndDialog(hWnd, LOWORD(wParam));
					if(m_SIPProcess) m_SIPProcess->_Stop(15000);
					if(m_History) m_History.release();
					bRet = TRUE;
					break;
				}
				case IDOK:
				case IDC_BUTTON_DIAL:
				{
					std::array<wchar_t, 32> vCommand;
					auto iLen = GetDlgItemText(hWnd, IDC_BUTTON_DIAL, vCommand.data(), vCommand.size());
					if(!wcscmp(vCommand.data(), L"Вызов"))
					{
						std::vector<wchar_t> vCalledPNUM;
						vCalledPNUM.resize(256);
						auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, &vCalledPNUM[0], vCalledPNUM.size());
						if(iLen && m_SIPProcess)
						{
							m_SIPProcess->_MakeCall(vCalledPNUM.data());
						}
						auto iRes = SendMessage(hComboBox, (UINT)CB_FINDSTRINGEXACT, (WPARAM)1, (LPARAM)vCalledPNUM.data());
						if(iRes == CB_ERR)
						{
							auto iCount = SendMessage(hComboBox, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
							if(iCount != CB_ERR && iCount >= ci_HistoryMAX) SendMessage(hComboBox, (UINT)CB_DELETESTRING, (WPARAM)--iCount, (LPARAM)0);
							SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)0, (LPARAM)vCalledPNUM.data());
						}
					}
					else if(!wcscmp(vCommand.data(), L"Ответить"))	if(m_SIPProcess)m_SIPProcess->_Answer();
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
					if(m_SIPProcess)
					{
						bMicrofonOn = !bMicrofonOn;
						SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, bMicrofonOn ? (WPARAM)dwMicLevel : (WPARAM)0);
						m_SIPProcess->_Microfon(bMicrofonOn ? dwMicLevel : 0);
						SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
					}
					break;
				}
				case IDC_BUTTON_SILENCE:
				{
					if(m_SIPProcess)
					{
						bSndOn = !bSndOn;
						SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, bSndOn ? (WPARAM)dwSndLevel : (WPARAM)0);
						m_SIPProcess->_Sound(bSndOn ? dwSndLevel : 0);
						SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
					}
					break;
				}
				case IDC_BUTTON_BACKSPACE:
				{
					std::array<wchar_t, 1024> vText; vText[0] = 0;
					auto iLen = GetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data(), vText.size());
					if(iLen) vText[iLen - 1] = 0;
					SetDlgItemText(hWnd, IDC_COMBO_DIAL, vText.data());
					break;
				}
				case IDC_BUTTON_CONFIG:
				{
					DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PARAMS), hWnd, (ConfigDlgProc), 0);
					break;
				}
				case IDC_BUTTON_HISTORY:
				{
					if(m_History) m_History->_Show(); 
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
					SendMessage(hDlgPhoneWnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
					//if(bWebMode || MessageBox(hWnd, L"Закрыть приложение?", L"SIP Phone", MB_YESNO | MB_ICONQUESTION) == IDYES)
					//{
					//	m_Log._LogWrite(L"ID_MIN_MENU_EXIT");
					//	ShowNotifyIcon(FALSE);
					//	if(m_WebSocketServer) m_WebSocketServer->_Stop(5000);
					//	SaveConfig(hWnd);
					//	if(m_SIPProcess) m_SIPProcess->_Stop(15000);
					//	EndDialog(hWnd, 0);
					//}
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
								SetDlgItemText(hWnd, IDC_STATIC_REGSTATUS, wstrStatus.c_str());
							}
							break;
						}
					}
				}
				default: break;
			}
			bRet = TRUE;
			break;
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
						SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
						if(m_SIPProcess) m_SIPProcess->_Microfon(dwMicLevel);
					}
					else if((HWND)lParam == hSliderSnd)
					{
						dwSndLevel = SendMessage(hSliderSnd, TBM_GETPOS, 0, 0);
						if(dwSndLevel == ci_LevelMIN)	bSndOn = false;
						else							bSndOn = true;
						SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
						if(m_SIPProcess) m_SIPProcess->_Sound(dwSndLevel);
					}
					break;
				}
				default:  break;
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

					RestoreFromTray();
				  // The user has previously double-clicked the icon, remove it in
				  // response to this second WM_LBUTTONUP
//					if(bHideIcon)
					{
						ShowNotifyIcon(FALSE);
						bHideIcon = false;
					}
					return TRUE;
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
			if(bWebMode || MessageBox(hWnd, L"Закрыть приложение?", L"SIP Phone", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				auto pcCfg = make_unique<CConfigFile>(true);
				SaveMainParameters(pcCfg);
				if(m_History) m_History->_SaveParameters(pcCfg);

				if(m_History) m_History.release();
				ShowNotifyIcon(FALSE);
				if(m_WebSocketServer) m_WebSocketServer->_Stop(5000);
				if(m_SIPProcess) m_SIPProcess->_Stop(15000);
				EndDialog(hWnd, 0);
			}
			bRet = TRUE;
			break;
		}
		case WM_USER_REGISTER_DS:
		{
			PlaySound(NULL, NULL, 0);
			SetDlgItemText(hWnd, IDC_COMBO_DIAL, L"");
			SetDlgItemText(hWnd, IDC_BUTTON_DIAL, L"Вызов");
			SetDlgItemText(hWnd, IDC_BUTTON_DISCONNECT, L"Отбой");
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
			if(m_SIPProcess && m_SIPProcess->_DTMF((wchar_t)lParam)) Beep(450, 100);
			break;
		}
		case WM_USER_REGISTER_OC: { break; }
		case WM_USER_CALL_ANM:	if(m_SIPProcess)m_SIPProcess->_Answer();		break;
		case WM_USER_CALL_DSC:	if(m_SIPProcess) m_SIPProcess->_Disconnect();	break;
		case WM_USER_ACC_MOD:
		{
			if(m_SIPProcess) m_SIPProcess->_Modify();
			else
			{
				if(wstrLogin.length())
				{
					m_SIPProcess = std::make_unique<CSIPProcess>(hDlgPhoneWnd);
					m_SIPProcess->_Start();
				}
			}
			break;
		}
		case WM_USER_MIC_LEVEL:
		{
			SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, dwMicLevel);
			if(dwMicLevel == ci_LevelMIN)	bMicrofonOn = false;
			else							bMicrofonOn = true;
			SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
			if(m_SIPProcess) m_SIPProcess->_Microfon(dwMicLevel);
			break;
		}
		case WM_USER_SND_LEVEL:
		{
			SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, dwSndLevel);
			if(dwSndLevel == ci_LevelMIN)	bSndOn = false;
			else							bSndOn = true;
			SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
			if(m_SIPProcess) m_SIPProcess->_Sound(dwSndLevel);
			break;
		}
		default:
		{
			break;
		}
	}
	return bRet;
}

BOOL CALLBACK ConfigDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			auto hFnt = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_CFG_ACC), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_CFG_TR), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_CFG_RT), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);
			SendMessage(GetDlgItem(hWnd, IDC_STATIC_CFG_DTMF), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

			SetDlgItemText(hWnd, IDC_EDIT_LOGIN, wstrLogin.c_str());
			SetDlgItemText(hWnd, IDC_EDIT_PASSWORD, wstrPassword.c_str());
			SetDlgItemText(hWnd, IDC_EDIT_SERVER, wstrServer.c_str());

			if(iTransportType == IPPROTO_TCP) CheckRadioButton(hWnd, IDC_RADIO_UDP, IDC_RADIO_TCP, IDC_RADIO_TCP);
			else CheckRadioButton(hWnd, IDC_RADIO_UDP, IDC_RADIO_TCP, IDC_RADIO_UDP);

			switch(uiRingToneNumber)
			{
				case 0: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT1); break;
				case 1: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT2); break;
				case 2: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT3); break;
			}


			if(bDTMF_2833) CheckRadioButton(hWnd, IDC_RADIO_DTMF2833, IDC_RADIO_DTMFINFO, IDC_RADIO_DTMF2833);
			else CheckRadioButton(hWnd, IDC_RADIO_DTMF2833, IDC_RADIO_DTMFINFO, IDC_RADIO_DTMFINFO);

			SendMessage(GetDlgItem(hWnd, IDC_CHECK_MINONSTART), BM_SETCHECK, bMinimizeOnStart ? BST_CHECKED : BST_UNCHECKED, 0);
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_AUTOSTART), BM_SETCHECK, bAutoStart ? BST_CHECKED : BST_UNCHECKED, 0);

			if(!bConfigMode)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SERVER), false);
				EnableWindow(hUDP_RB, false);
				EnableWindow(hTCP_RB, false);
				EnableWindow(hDTMF_RFC2833_RB, false);
				EnableWindow(hDTMF_INFO_RB, false);
			}

			bRet = TRUE;
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
				case IDABORT:
				{
					EndDialog(hWnd, LOWORD(wParam));
					bRet = TRUE;
					DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);
					DefDlgProc(hDlgPhoneWnd, (UINT)BM_SETSTYLE, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
				}
				break;
				case IDOK:
				{
					std::vector<wchar_t> vLogin;
					vLogin.resize(256);
					auto iLen = GetDlgItemText(hWnd, IDC_EDIT_LOGIN, &vLogin[0], vLogin.size());
					wstrLogin = vLogin.data();

					std::vector<wchar_t> vPassword;
					vPassword.resize(256);
					iLen = GetDlgItemText(hWnd, IDC_EDIT_PASSWORD, &vPassword[0], vPassword.size());
					wstrPassword = vPassword.data();

					std::vector<wchar_t> vServer;
					vServer.resize(256);
					iLen = GetDlgItemText(hWnd, IDC_EDIT_SERVER, &vServer[0], vServer.size());
					wstrServer = vServer.data();

					if(IsDlgButtonChecked(hWnd, IDC_RADIO_RT1)) uiRingToneNumber = 0;
					else if(IsDlgButtonChecked(hWnd, IDC_RADIO_RT2)) uiRingToneNumber = 1;
					else if(IsDlgButtonChecked(hWnd, IDC_RADIO_RT3)) uiRingToneNumber = 2;

					if(IsDlgButtonChecked(hWnd, IDC_RADIO_TCP)) iTransportType = IPPROTO_TCP;
					else if(IsDlgButtonChecked(hWnd, IDC_RADIO_UDP)) iTransportType = IPPROTO_UDP;

					if(IsDlgButtonChecked(hWnd, IDC_RADIO_DTMF2833)) bDTMF_2833 = true;
					else bDTMF_2833 = false;

					if(m_SIPProcess) m_SIPProcess->_Modify();
					else
					{
						if(wstrLogin.length())
						{
							m_SIPProcess = std::make_unique<CSIPProcess>(hDlgPhoneWnd);
							m_SIPProcess->_Start();
						}
					}

					EndDialog(hWnd, LOWORD(wParam));
					bRet = TRUE;
					DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);
					DefDlgProc(hDlgPhoneWnd, (UINT)BM_SETSTYLE, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
					break;
				}
				case IDC_RADIO_RT1:
				{
					PlaySound(L"RingToneName1", NULL, SND_RESOURCE | SND_ASYNC);
					Sleep(3000);
					PlaySound(NULL, NULL, 0);
					break;
				}
				case IDC_RADIO_RT2:
				{
					PlaySound(L"RingToneName2", NULL, SND_RESOURCE | SND_ASYNC);
					Sleep(3000);
					PlaySound(NULL, NULL, 0);
					break;
				}
				case IDC_RADIO_RT3:
				{
					PlaySound(L"RingToneName3", NULL, SND_RESOURCE | SND_ASYNC);
					Sleep(3000);
					PlaySound(NULL, NULL, 0);
					break;
				}
				case IDC_CHECK_MINONSTART:
				{
					bMinimizeOnStart = IsDlgButtonChecked(hWnd, IDC_CHECK_MINONSTART) ? true : false;
					break;
				}
				case IDC_CHECK_AUTOSTART:
				{
					bAutoStart = IsDlgButtonChecked(hWnd, IDC_CHECK_AUTOSTART) ? true : false;
					if(bAutoStart)
					{
						HKEY hkey = NULL;
						HRESULT hres = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hkey);
						hres = RegSetValueEx(hkey, L"CLOFF SIP Phone", 0, REG_SZ, (BYTE*)csEXEPath.data(), (wcslen(csEXEPath.data()) + 1) * 2);
						RegCloseKey(hkey);
					}
					else
					{
						HKEY hKey;
						long lReturn = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0L, KEY_ALL_ACCESS, &hKey);
						lReturn = RegDeleteValue(hKey, (LPCWSTR)L"CLOFF SIP Phone");
						lReturn = RegCloseKey(hKey);
					}
					break;
				}
				default: break;
			}
			bRet = TRUE;
			break;
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			bRet = TRUE;
			DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);
			DefDlgProc(hDlgPhoneWnd, (UINT)BM_SETSTYLE, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
			break;
		default:
			break;
	}
	return bRet;
}

void InitDialog()
{
	hComboBox = GetDlgItem(hDlgPhoneWnd, IDC_COMBO_DIAL);
	hStatusInfo = GetDlgItem(hDlgPhoneWnd, IDC_STATIC_REGSTATUS);
	hEditServer = GetDlgItem(hDlgPhoneWnd, IDC_EDIT_SERVER);
	hTCP_RB = GetDlgItem(hDlgPhoneWnd, IDC_RADIO_TCP);
	hUDP_RB = GetDlgItem(hDlgPhoneWnd, IDC_RADIO_UDP);
	hDTMF_RFC2833_RB = GetDlgItem(hDlgPhoneWnd, IDC_RADIO_DTMF2833);
	hDTMF_INFO_RB = GetDlgItem(hDlgPhoneWnd, IDC_RADIO_DTMFINFO);
	hBtMute = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_MUTE);
	hBtSilence = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_SILENCE);
	hBtConfig = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_CONFIG);
	hBtBackSpace = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_BACKSPACE);
	hSliderMic = GetDlgItem(hDlgPhoneWnd, IDC_SLIDER_MIC);
	hSliderSnd = GetDlgItem(hDlgPhoneWnd, IDC_SLIDER_SND);
	hBtDial = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_DIAL);
	hBtDisconnect = GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_DISCONNECT);
	m_History = make_unique<CHistory>();

	unique_ptr<CConfigFile> pCfg;
	if(!LoadConfig(hDlgPhoneWnd))
	{
		pCfg = make_unique<CConfigFile>(false);
		LoadMainParameters(pCfg);
		m_History->_LoadParameters(pCfg);
	}

	SetDlgItemText(hDlgPhoneWnd, IDC_COMBO_DIAL, L"");
	if(!bConfigMode)
	{
		EnableWindow(hEditServer, false);
		EnableWindow(hTCP_RB, false);
		EnableWindow(hUDP_RB, false);
		EnableWindow(hDTMF_RFC2833_RB, false);
		EnableWindow(hDTMF_INFO_RB, false);
	}

	SetWindowPos(hBtConfig, HWND_TOP, 245, 107, 80, 40, SWP_SHOWWINDOW);
	auto hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONFIG));
	SendMessage(hBtConfig, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(hBtBackSpace, HWND_TOP, 245, 66, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_BS));
	SendMessage(hBtBackSpace, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(hBtDial, HWND_TOP, 2, 25, 120, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ACCEPT_N));
	SendMessage(hBtDial, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(hBtDisconnect, HWND_TOP, 124, 25, 120, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CANCEL_N));
	SendMessage(hBtDisconnect, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_7), HWND_TOP, 2, 66, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_7));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_7), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_8), HWND_TOP, 83, 66, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_8));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_8), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_9), HWND_TOP, 164, 66, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_9));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_9), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_4), HWND_TOP, 2, 107, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_4));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_4), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_5), HWND_TOP, 83, 107, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_5));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_5), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_6), HWND_TOP, 164, 107, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_6));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_6), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_1), HWND_TOP, 2, 148, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_1));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_1), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_2), HWND_TOP, 83, 148, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_2));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_2), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_3), HWND_TOP, 164, 148, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_3));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_3), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_DIES), HWND_TOP, 2, 189, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_DIES));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_DIES), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_0), HWND_TOP, 83, 189, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_NULL));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_0), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_ASTERISK), HWND_TOP, 164, 189, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_ASTERISK));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_ASTERISK), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_PAUSE), HWND_TOP, 2, 230, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_P));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_PAUSE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_WAIT), HWND_TOP, 164, 230, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_W));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_WAIT), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_HISTORY), HWND_TOP, 245, 25, 80, 40, SWP_SHOWWINDOW);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_HISTORY));
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_BUTTON_HISTORY), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);

	SetWindowPos(hBtMute, HWND_TOP, 245, 271, 80, 40, SWP_SHOWWINDOW);
	hBtMpMicON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICON));
	hBtMpMicOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICOFF));
	if(bMicrofonOn) SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpMicON);
	else			SendMessage(hBtMute, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpMicOFF);

	SetWindowPos(hBtSilence, HWND_TOP, 245, 312, 80, 40, SWP_SHOWWINDOW);
	hBtMpSndON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDON));
	hBtMpSndOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDOFF));
	if(bSndOn)	SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpSndON);
	else		SendMessage(hBtSilence, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpSndOFF);

	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_STATIC_D1), HWND_TOP, 245, 148, 80, 40, SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_STATIC_D2), HWND_TOP, 245, 189, 80, 40, SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_STATIC_D3), HWND_TOP, 245, 230, 80, 40, SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hDlgPhoneWnd, IDC_STATIC_D4), HWND_TOP, 83, 230, 80, 40, SWP_SHOWWINDOW);

	auto hFnt = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_STATIC_REGSTATUS), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

	auto mtt = CreateToolTip(hBtConfig, hDlgPhoneWnd, (PTSTR)TEXT("Настройки"));

	if(wstrLogin.length())
	{
		m_SIPProcess = std::make_unique<CSIPProcess>(hDlgPhoneWnd);
		m_SIPProcess->_Start();
	}
	else
	{
		SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, L"Требуется настройка аккаунта.");
		DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
	}

	SendMessage(hSliderMic, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
	SendMessage(hSliderSnd, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
	SendMessage(hSliderMic, TBM_SETPOS, (WPARAM)TRUE, dwMicLevel);
	SendMessage(hSliderSnd, TBM_SETPOS, (WPARAM)TRUE, dwSndLevel);

	RECT rect;
	GetClientRect(hComboBox, &rect);
	MapDialogRect(hComboBox, &rect);
	SetWindowPos(hComboBox, 0, 0, 0, rect.right, (10 + 1) * rect.bottom, SWP_NOMOVE);
	hFnt = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	SendMessage(hComboBox, WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

	m_WebSocketServer = make_unique<CWebSocket>();
	m_WebSocketServer->_Start();

	m_Log._LogWrite(L"InitDialogEnd");
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
	swprintf_s(wsMsg.data(), wsMsg.size(),L"CloffSIP %s\r\nclick to open",wstrLogin.c_str());
	lstrcpy(nid.szTip, wsMsg.data());

	if(bAdd) Shell_NotifyIcon(NIM_ADD, &nid);
	else	 Shell_NotifyIcon(NIM_DELETE, &nid);
}

void MinimizeToTray()
{
	if(m_History) m_History->_Hide();
	ShowWindow(hDlgPhoneWnd, SW_HIDE);
}

void RestoreFromTray()
{
// Show the window, and make sure we're the foreground window
	ShowWindow(hDlgPhoneWnd, SW_SHOW);
	SetActiveWindow(hDlgPhoneWnd);
	SetForegroundWindow(hDlgPhoneWnd);
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

void SaveMainParameters(unique_ptr<CConfigFile>& pCfg)
{
	RECT sCoord{ 0 };
	GetWindowRect(hDlgPhoneWnd, &sCoord);

	vector<wchar_t> vCN(strlen(szCompName.data()) * sizeof(wchar_t) + 1);
	MultiByteToWideChar(1251, MB_PRECOMPOSED, szCompName.data(), strlen(szCompName.data()), vCN.data(), vCN.size());

	wstring wstrData(vCN.data());
	wstrData += L"\n" + wstrLogin + L"\n" + wstrPassword + L"\n" + wstrServer + L"\n";
	pCfg->PutCodedStringParameter("DataStart", wstrData.c_str(), "MAIN");
	pCfg->PutIntParameter("X", sCoord.left);
	pCfg->PutIntParameter("Y", sCoord.top);
	pCfg->PutIntParameter("Transport", iTransportType);
	pCfg->PutStringParameter("DTMF", bDTMF_2833 ? L"RFC2833" : L"INFO");
	pCfg->PutIntParameter("RingNum", uiRingToneNumber);
	pCfg->PutIntParameter("MicLevel", dwMicLevel);
	pCfg->PutIntParameter("SndLevel", dwSndLevel);
	pCfg->PutIntParameter("MinOnStart", bMinimizeOnStart ? 1 : 0);
	pCfg->PutIntParameter("Autostart", bAutoStart ? 1 : 0);
}

void LoadMainParameters(unique_ptr<CConfigFile>& pCfg)
{
	wstring wstrBuf;
	RECT sCoord{ 0 };

	sCoord.left = pCfg->GetIntParameter("MAIN", "X", 0);
	sCoord.top = pCfg->GetIntParameter("MAIN", "Y", 0);
	SetWindowPos(hDlgPhoneWnd, HWND_TOP, sCoord.left, sCoord.top, ci_MainWidth, ci_MainHeight, SWP_SHOWWINDOW);

	iTransportType = pCfg->GetIntParameter("MAIN", "Transport", IPPROTO_TCP);

	pCfg->GetStringParameter("MAIN", "DTMF",wstrBuf);
	if(!wstrBuf.compare(L"RFC2833")) bDTMF_2833 = true;
	else bDTMF_2833 = false;

	uiRingToneNumber = pCfg->GetIntParameter("MAIN", "RingNum", 0);
	dwMicLevel = pCfg->GetIntParameter("MAIN", "MicLevel", ci_LevelMAX);
	dwSndLevel = pCfg->GetIntParameter("MAIN", "SndLevel", ci_LevelMAX);
	bMinimizeOnStart = pCfg->GetIntParameter("MAIN", "MinOnStart", 0) ? true : false;
	bAutoStart = pCfg->GetIntParameter("MAIN", "Autostart", 0) ? true : false;

	pCfg->GetCodedStringParameter("MAIN", "DataStart",wstrBuf);
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
			WideCharToMultiByte(1251, WC_COMPOSITECHECK, wstrPCName.c_str(), -1, vCN.data(), vCN.size(),NULL,NULL);

			if(!strcmp(szCompName.data(), vCN.data()))
			{
				iCurrPars = iNextPars +1;
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
/*
	auto hFnt = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, nullptr);
	SendMessage(GetDlgItem(hWnd, IDC_STATIC_CFG_ACC), WM_SETFONT, (WPARAM)hFnt, (LPARAM)TRUE);

	//чтение списка
	std::vector<wchar_t> vCalledPNUM;
	auto iCount = SendMessage(hComboBox, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	while(iCount-- >= 0)
	{
		auto iTxtLen = SendMessage(hComboBox, (UINT)CB_GETLBTEXTLEN, (WPARAM)iCount, (LPARAM)0);
		if(iTxtLen != CB_ERR)
		{
			vCalledPNUM.resize(iTxtLen + 1);
			if(SendMessage(hComboBox, (UINT)CB_GETLBTEXT, (WPARAM)iCount, (LPARAM)vCalledPNUM.data()) != CB_ERR)
			{
				memcpy_s(szData.data() + stCount, szData.size() - stCount, vCalledPNUM.data(), iTxtLen * sizeof(wchar_t));
				stCount += iTxtLen * sizeof(wchar_t);
				szData[stCount++] = '\n';
			}
		}
	}
*/
