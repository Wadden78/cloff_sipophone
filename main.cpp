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

std::unique_ptr<CSIPProcess> m_SIPProcess;
std::wstring	wstrLogin{ L"" };
std::wstring	wstrPassword{ L"" };
std::wstring	wstrServer{ L"sip.cloff.ru:5060" };
int iTransportType{ IPPROTO_TCP };
uint16_t uiRingToneNumber{ 0 };
bool bDTMF_2833{ true };
bool bDebugMode{ false };
bool bConfigMode{ false };

const int ci_LevelMIN = 0;
const int ci_LevelMAX = 100;
const int ci_HistoryMAX = 20;

bool bMicrofonOn{ true };
HBITMAP hBtMpMicON{ 0 };
HBITMAP hBtMpMicOFF{ 0 };
DWORD dwMicLevel{ ci_LevelMAX };

bool bSndOn{ true };
HBITMAP hBtMpSndON{ 0 };
HBITMAP hBtMpSndOFF{ 0 };
DWORD dwSndLevel{ ci_LevelMAX };

HWND sliderMic{ 0 };
HWND sliderSnd{ 0 };

HWND hComboBox{ 0 };
HWND hStatusInfo{ 0 };
HWND hDlgPhoneWnd{ 0 };
HINSTANCE hInstance{ 0 };
array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName{ 0 };

wchar_t default_key[] = L"3igcZhRdWq01M3G4mTAiv9";

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(HINSTANCE curInst, HINSTANCE hPrev, LPSTR lpstrCmdLine, int nCmdShow)
{
	hInstance = curInst;
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
//	InitCommonControls();

	if(strstr(lpstrCmdLine, "-debug")) bDebugMode = true;
	if(strstr(lpstrCmdLine, "-config")) bConfigMode = true;
	if(strstr(lpstrCmdLine, "-default")) DeleteFile(L"cloff_sip_phone.cfg");
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

BOOL CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			hDlgPhoneWnd = hWnd;
			hComboBox = GetDlgItem(hWnd, IDC_COMBO_DIAL);

			LoadConfig(hWnd);
			SetWindowText(hWnd, wstrLogin.c_str());
			SetDlgItemText(hWnd, IDC_COMBO_DIAL, L"");
			if(!bConfigMode)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SERVER), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_TCP), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_UDP), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DTMF2833), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DTMFINFO), false);
			}

			hBtMpMicON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICON));
			hBtMpMicOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_MICOFF));
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_MUTE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpMicON);

			hBtMpSndON = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDON));
			hBtMpSndOFF = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_SNDOFF));
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_SILENCE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpSndON);

			auto hBtMpConfig = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_CONFIG));
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_CONFIG), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpConfig);

			auto hBtMpBS = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP_BS));
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_BACKSPACE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpBS);

			if(wstrLogin.length())
			{
				m_SIPProcess = std::make_unique<CSIPProcess>(hWnd);
				m_SIPProcess->_Start();
			}
			else
			{
				SetDlgItemText(hWnd, IDC_STATIC_REGSTATUS, L"Требуется настройка аккаунта.");
				DefDlgProc(hDlgPhoneWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
			}
			SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_PROGRESS_CALL), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(GetDlgItem(hDlgPhoneWnd, IDC_PROGRESS_CALL), PBM_SETSTEP, (WPARAM)1, 0);

			sliderMic = GetDlgItem(hDlgPhoneWnd, IDC_SLIDER_MIC);
			sliderSnd = GetDlgItem(hDlgPhoneWnd, IDC_SLIDER_SND);
			SendMessage(sliderMic, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
			SendMessage(sliderSnd, TBM_SETRANGE, (WPARAM)1, (LPARAM)MAKELONG(ci_LevelMIN, ci_LevelMAX));
			SendMessage(sliderMic, TBM_SETPOS, (WPARAM)TRUE, dwMicLevel);
			SendMessage(sliderSnd, TBM_SETPOS, (WPARAM)TRUE, dwSndLevel);

			RECT rect;
			GetClientRect(hComboBox, &rect);
			MapDialogRect(hComboBox, &rect);
			SetWindowPos(hComboBox, 0, 0, 0, rect.right, (10 + 1) * rect.bottom, SWP_NOMOVE);

//			NOTIFYICONDATA niData{0};

			bRet = TRUE;
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDABORT:
					EndDialog(hWnd, LOWORD(wParam));
					SaveConfig(hWnd);
					bRet = TRUE;
					break;
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
						if(iLen) if(m_SIPProcess) m_SIPProcess->_MakeCall(vCalledPNUM.data());

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
						SendMessage(sliderMic, TBM_SETPOS, (WPARAM)TRUE, bMicrofonOn ? (WPARAM)dwMicLevel : (WPARAM)0);
						m_SIPProcess->_Microfon(bMicrofonOn ? dwMicLevel : 0);
						SendMessage(GetDlgItem(hWnd, IDC_BUTTON_MUTE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
					}
					break;
				}
				case IDC_BUTTON_SILENCE:
				{
					if(m_SIPProcess)
					{
						bSndOn = !bSndOn;
						SendMessage(sliderSnd, TBM_SETPOS, (WPARAM)TRUE, bSndOn ? (WPARAM)dwSndLevel : (WPARAM)0);
						m_SIPProcess->_Sound(bSndOn ? dwSndLevel : 0);
						SendMessage(GetDlgItem(hWnd, IDC_BUTTON_SILENCE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
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
					if((HWND)lParam == sliderMic)
					{
						dwMicLevel = SendMessage(sliderMic, TBM_GETPOS, 0, 0);
						if(dwMicLevel == ci_LevelMIN)	bMicrofonOn = false;
						else							bMicrofonOn = true;
						SendMessage(GetDlgItem(hWnd, IDC_BUTTON_MUTE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bMicrofonOn ? (LPARAM)hBtMpMicON : (LPARAM)hBtMpMicOFF);
						if(m_SIPProcess) m_SIPProcess->_Microfon(dwMicLevel);
					}
					else if((HWND)lParam == sliderSnd)
					{
						dwSndLevel = SendMessage(sliderSnd, TBM_GETPOS, 0, 0);
						if(dwSndLevel == ci_LevelMIN)	bSndOn = false;
						else							bSndOn = true;
						SendMessage(GetDlgItem(hWnd, IDC_BUTTON_SILENCE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, bSndOn ? (LPARAM)hBtMpSndON : (LPARAM)hBtMpSndOFF);
						if(m_SIPProcess) m_SIPProcess->_Sound(dwSndLevel);
					}
					break;
				}
				default:  break;
			}
			break;
		}
		case WM_CLOSE:
		{
			if(MessageBox(hWnd, L"Закрыть приложение?", L"SIP Phone", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				SaveConfig(hWnd);
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
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_DIAL), true);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_DISCONNECT), false);
			bMicrofonOn = true;
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_MUTE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpMicON);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_MUTE), false);
			bSndOn = true;
			SendMessage(GetDlgItem(hWnd, IDC_BUTTON_SILENCE), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBtMpSndON);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_SILENCE), false);

			ShowWindow(GetDlgItem(hWnd, IDC_PROGRESS_CALL), SW_HIDE);

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
		default: break;
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
			SetDlgItemText(hWnd, IDC_EDIT_LOGIN, wstrLogin.c_str());
			SetDlgItemText(hWnd, IDC_EDIT_PASSWORD, wstrPassword.c_str());
			SetDlgItemText(hWnd, IDC_EDIT_SERVER, wstrServer.c_str());

			if(iTransportType == IPPROTO_TCP) CheckRadioButton(hWnd, IDC_RADIO_UDP, IDC_RADIO_TCP, IDC_RADIO_TCP);

			switch(uiRingToneNumber)
			{
				case 0: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT1); break;
				case 1: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT2); break;
				case 2: CheckRadioButton(hWnd, IDC_RADIO_RT1, IDC_RADIO_RT3, IDC_RADIO_RT3); break;
			}


			if(bDTMF_2833) CheckRadioButton(hWnd, IDC_RADIO_DTMF2833, IDC_RADIO_DTMFINFO, IDC_RADIO_DTMF2833);
			else CheckRadioButton(hWnd, IDC_RADIO_DTMF2833, IDC_RADIO_DTMFINFO, IDC_RADIO_DTMFINFO);

			if(!bConfigMode)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SERVER), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_UDP), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_TCP), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DTMF2833), false);
				EnableWindow(GetDlgItem(hWnd, IDC_RADIO_DTMFINFO), false);
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
					SetWindowText(hWnd, wstrLogin.c_str());

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
					SaveConfig(hWnd);

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

void LoadConfig(HWND hWnd)
{
	FILE* fInit = nullptr;
	fopen_s(&fInit, "cloff_sip_phone.cfg", "rb");
	if(!fInit)
	{
		if(errno != ENOENT)
		{
			std::array<wchar_t, 256> chMsg;
			swprintf_s(chMsg.data(), chMsg.size(), L"Config file open error %u(%S)", errno, strerror(errno));
			SetDlgItemText(hWnd, IDC_STATIC_REGSTATUS, chMsg.data());
		}
	}
	else
	{
		vector<BYTE> vData;
		fseek(fInit, 0, SEEK_END);
		auto stFLen = ftell(fInit);
		vData.resize(stFLen);
		fseek(fInit, 0, SEEK_SET);
		fread(vData.data(), sizeof(char), vData.size(), fInit);
		wchar_t* key_str = default_key;
		DWORD dwStatus = 0;
		wchar_t info[] = L"Microsoft Enhanced RSA and AES Cryptographic Provider";
		HCRYPTPROV hProv;
		HCRYPTHASH hHash;
		HCRYPTKEY hKey;
		array<wchar_t, 256> wszMessage;

		if(!CryptAcquireContextW(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			dwStatus = GetLastError();
			swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptAcquireContext failed: %x", dwStatus);
			SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
		}
		else
		{
			if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
			{
				dwStatus = GetLastError();
				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptCreateHash failed: %x", dwStatus);
				SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
			}
			else
			{
				if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenW(key_str), 0))
				{
					DWORD err = GetLastError();
					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptHashData Failed : %#x", err);
					SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
				}
				else
				{
					if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
					{
						dwStatus = GetLastError();
						swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptDeriveKey failed: %x", dwStatus);
						SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
					}
					else
					{
						DWORD out_len = stFLen;
						if(!CryptDecrypt(hKey, NULL, TRUE, 0, vData.data(), &out_len))
						{
							dwStatus = GetLastError();
							switch(dwStatus)
							{
								case ERROR_INVALID_HANDLE:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		break;
								case ERROR_INVALID_PARAMETER:	swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	break;
								case NTE_BAD_ALGID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_ALGID");				break;
								case NTE_BAD_DATA:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_DATA");				break;
								case NTE_BAD_FLAGS:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_FLAGS");				break;
								case NTE_BAD_HASH:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_HASH");				break;
								case NTE_BAD_KEY:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_KEY");				break;
								case NTE_BAD_LEN:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_LEN");				break;
								case NTE_BAD_UID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_UID");				break;
								case NTE_DOUBLE_ENCRYPT:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");		break;
								case NTE_FAIL:					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_FAIL");					break;
								default:						swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=%d(%x)", dwStatus, dwStatus); break;
							}
							SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
						}
						else
						{
							auto bPtr = vData.data();
							RECT sCoord{ 0 };
							sCoord.left = *reinterpret_cast<LONG*>(bPtr);
							bPtr += sizeof(sCoord.left);
							sCoord.right = *reinterpret_cast<LONG*>(bPtr);
							bPtr += sizeof(sCoord.right);
							sCoord.top = *reinterpret_cast<LONG*>(bPtr);
							bPtr += sizeof(sCoord.top);
							sCoord.bottom = *reinterpret_cast<LONG*>(bPtr);
							bPtr += sizeof(sCoord.bottom);
							SetWindowPos(hDlgPhoneWnd, HWND_TOP, sCoord.left, sCoord.top, sCoord.right - sCoord.left, sCoord.bottom - sCoord.top, SWP_SHOWWINDOW);

							auto bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', stFLen));
							if(bParameterEnd)
							{
								//CompName
								string strCN;
								strCN.append(reinterpret_cast<char*>(bPtr), bParameterEnd - bPtr);
								bPtr = bParameterEnd + sizeof('\n');

								if(!strCN.compare(szCompName.data()))
								{
									bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', stFLen));
									if(bParameterEnd)
									{
										//Login
										wstrLogin.clear();
										wstrLogin.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
										bPtr = bParameterEnd + sizeof('\n');

										bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', stFLen));
										if(bParameterEnd)
										{
											//Password
											wstrPassword.clear();
											wstrPassword.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
											bPtr = bParameterEnd + sizeof('\n');

											bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', stFLen));
											if(bParameterEnd)
											{
												//Server
												wstrServer.clear();
												wstrServer.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
												bPtr = bParameterEnd + sizeof('\n');
												if((bPtr - vData.data()) < out_len)
												{
													//Transport
													iTransportType = *reinterpret_cast<int*>(bPtr);
													if(iTransportType != IPPROTO_TCP && iTransportType != IPPROTO_UDP) iTransportType = IPPROTO_TCP;
													bPtr += sizeof(iTransportType);
													if((bPtr - vData.data()) < out_len)
													{
														//DTMF
														bDTMF_2833 = *reinterpret_cast<bool*>(bPtr);
														bPtr += sizeof(bDTMF_2833);
														if((bPtr - vData.data()) < out_len)
														{
															//Ring
															uiRingToneNumber = *reinterpret_cast<uint16_t*>(bPtr);
															if(uiRingToneNumber >= 3) uiRingToneNumber = 0;
															bPtr += sizeof(uiRingToneNumber);
															//Microfon level
															if((bPtr - vData.data()) < out_len)
															{
																dwMicLevel = *reinterpret_cast<DWORD*>(bPtr);
																if(dwMicLevel > ci_LevelMAX) dwMicLevel = ci_LevelMAX;
																bPtr += sizeof(dwMicLevel);
																//Sound level
																if((bPtr - vData.data()) < out_len)
																{
																	dwSndLevel = *reinterpret_cast<DWORD*>(bPtr);
																	if(dwSndLevel > ci_LevelMAX) dwSndLevel = ci_LevelMAX;
																	bPtr += sizeof(dwSndLevel);

																	wstring wstrCalledPNUM;
																	while((bPtr - vData.data()) < out_len)
																	{
																		auto bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', stFLen));
																		if(bParameterEnd)
																		{
																			wstrCalledPNUM.clear();
																			wstrCalledPNUM.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
																			bPtr = bParameterEnd + sizeof('\n');
																			SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)0, (LPARAM)wstrCalledPNUM.c_str());
																		}
																		else break;
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
							}
						}
						CryptDestroyKey(hKey);
					}
				}
				CryptDestroyHash(hHash);
			}
			CryptReleaseContext(hProv, 0);
		}
		fclose(fInit);
	}

}

void SaveConfig(HWND hWnd)
{
	array<BYTE, CHUNK_SIZE * 10> szData{ 0 };
	size_t stCount = 0;
	//Coordinate
	RECT sCoord{ 0 };
	GetWindowRect(hDlgPhoneWnd, &sCoord);
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &sCoord.left, sizeof(sCoord.left));
	stCount += sizeof(sCoord.left);
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &sCoord.right, sizeof(sCoord.right));
	stCount += sizeof(sCoord.right);
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &sCoord.top, sizeof(sCoord.top));
	stCount += sizeof(sCoord.top);
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &sCoord.bottom, sizeof(sCoord.bottom));
	stCount += sizeof(sCoord.bottom);
	//CompName
	memcpy_s(szData.data() + stCount, szData.size() - stCount, szCompName.data(), strlen(szCompName.data()));
	stCount += strlen(szCompName.data());
	szData[stCount++] = '\n';
	//Login
	memcpy_s(szData.data() + stCount, szData.size(), wstrLogin.c_str(), wstrLogin.length() * sizeof(wchar_t));
	stCount += wstrLogin.length() * sizeof(wchar_t);
	szData[stCount++] = '\n';
	//Password
	memcpy_s(szData.data() + stCount, szData.size() - stCount, wstrPassword.c_str(), wstrPassword.length() * sizeof(wchar_t));
	stCount += wstrPassword.length() * sizeof(wchar_t);
	szData[stCount++] = '\n';
	//Server
	memcpy_s(szData.data() + stCount, szData.size() - stCount, wstrServer.c_str(), wstrServer.length() * sizeof(wchar_t));
	stCount += wstrServer.length() * sizeof(wchar_t);
	szData[stCount++] = '\n';
	//Transport
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &iTransportType, sizeof(iTransportType));
	stCount += sizeof(iTransportType);
	//DTMF
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &bDTMF_2833, sizeof(bDTMF_2833));
	stCount += sizeof(bDTMF_2833);
	//Ring
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &uiRingToneNumber, sizeof(uiRingToneNumber));
	stCount += sizeof(uiRingToneNumber);
	//Mic level
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &dwMicLevel, sizeof(dwMicLevel));
	stCount += sizeof(dwMicLevel);
	//Snd level
	memcpy_s(szData.data() + stCount, szData.size() - stCount, &dwSndLevel, sizeof(dwSndLevel));
	stCount += sizeof(dwSndLevel);

	std::vector<wchar_t> vCalledPNUM;
	auto iCount = SendMessage(hComboBox, (UINT)CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
	while(iCount-- >=0 )
	{
		auto iTxtLen = SendMessage(hComboBox, (UINT)CB_GETLBTEXTLEN, (WPARAM)iCount, (LPARAM)0);
		if(iTxtLen != CB_ERR)
		{
			vCalledPNUM.resize(iTxtLen+1);
			if(SendMessage(hComboBox, (UINT)CB_GETLBTEXT, (WPARAM)iCount, (LPARAM)vCalledPNUM.data()) != CB_ERR)
			{
				memcpy_s(szData.data() + stCount, szData.size() - stCount, vCalledPNUM.data(), iTxtLen * sizeof(wchar_t));
				stCount += iTxtLen * sizeof(wchar_t);
				szData[stCount++] = '\n';
			}
		}
	}

	FILE* fInit = nullptr;
	fopen_s(&fInit, "cloff_sip_phone.cfg", "wb");
	if(!fInit)
	{
		if(errno != ENOENT)
		{
			std::array<wchar_t, 256> chMsg;
			swprintf_s(chMsg.data(), chMsg.size(), L"Config file open error %u(%S)", errno, strerror(errno));
			SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, chMsg.data());
		}
	}
	else
	{
		wchar_t* key_str = default_key;
		DWORD dwStatus = 0;
		wchar_t info[] = L"Microsoft Enhanced RSA and AES Cryptographic Provider";
		HCRYPTPROV hProv;
		HCRYPTHASH hHash;
		HCRYPTKEY hKey;
		array<wchar_t, 256> wszMessage;

		if(!CryptAcquireContextW(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
		{
			dwStatus = GetLastError();
			swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptAcquireContext failed: %x", dwStatus);
			SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
		}
		else
		{
			if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
			{
				dwStatus = GetLastError();
				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptCreateHash failed: %x", dwStatus);
				SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
			}
			else
			{
				if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenW(key_str), 0))
				{
					DWORD err = GetLastError();
					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptHashData Failed : %#x", err);
					SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
				}
				else
				{
					if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
					{
						dwStatus = GetLastError();
						swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptDeriveKey failed: %x", dwStatus);
						SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
					}
					else
					{
						DWORD out_len = stCount;
						if(!CryptEncrypt(hKey, NULL, TRUE, 0, szData.data(), &out_len, szData.size()))
						{
							dwStatus = GetLastError();
							switch(dwStatus)
							{
								case ERROR_INVALID_HANDLE:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		break;
								case ERROR_INVALID_PARAMETER:	swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	break;
								case NTE_BAD_ALGID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_ALGID");				break;
								case NTE_BAD_DATA:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_DATA");				break;
								case NTE_BAD_FLAGS:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_FLAGS");				break;
								case NTE_BAD_HASH:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_HASH");				break;
								case NTE_BAD_KEY:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_KEY");				break;
								case NTE_BAD_LEN:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_LEN");				break;
								case NTE_BAD_UID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_UID");				break;
								case NTE_DOUBLE_ENCRYPT:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");		break;
								case NTE_FAIL:					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_FAIL");					break;
								default:						swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=%d(%x)", dwStatus, dwStatus); break;
							}
							SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
						}
						else fwrite(szData.data(), out_len, 1, fInit);
						CryptDestroyKey(hKey);
					}
				}
				CryptDestroyHash(hHash);
			}
			CryptReleaseContext(hProv, 0);
		}
		fclose(fInit);
	}

}