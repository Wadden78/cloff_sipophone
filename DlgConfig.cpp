#include "Main.h"

HWND hEditServer{ 0 };
HWND hTCP_RB{ 0 };
HWND hUDP_RB{ 0 };
HWND hDTMF_RFC2833_RB{ 0 };
HWND hDTMF_INFO_RB{ 0 };

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

					SaveJSONMainParameters();
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
						long lReturn = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0L, KEY_ALL_ACCESS, &hKey);
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
