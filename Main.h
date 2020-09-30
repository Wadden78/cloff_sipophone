#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <list>
#include <commctrl.h>
#include "resource.h"
#include "CLogFile.h"
#include "CSIPProcess.h"

extern HINSTANCE hInstance;

extern CLogFile m_Log;
extern std::unique_ptr<CSIPProcess> m_SIPProcess;
class CWebSocket;
extern std::unique_ptr<CWebSocket> m_WebSocketServer;
class CHistory;
extern std::unique_ptr<CHistory> m_History;
extern in_port_t ipWSPort;
extern string strProductName;
extern string strProductVersion;
extern bool bWebMode;			/** признак запуска из веб*/
extern bool bMinimizeOnStart;	/** Минимизация после старта*/
extern bool bAutoStart;			/** Автоматический запуск после старта ОС*/

extern std::wstring	wstrLogin;
extern std::wstring	wstrPassword;
extern std::wstring	wstrServer;
extern int iTransportType;
extern uint16_t uiRingToneNumber;
extern bool bDTMF_2833;
extern bool bDebugMode;
extern array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName;

extern HWND hDlgPhoneWnd;

extern HWND sliderMic;
extern HWND sliderSnd;

extern HWND hComboBox;

extern DWORD dwMicLevel;
extern DWORD dwSndLevel;

const int ci_LevelMIN = 0;
const int ci_LevelMAX = 100;
const int ci_HistoryMAX = 20;

#define WM_USER_REGISTER_OK	(WM_USER + 1)	//аккаунт зарегистрирован	
#define WM_USER_REGISTER_ER	(WM_USER + 2)	//ошибка регистрации аккаунта
#define WM_USER_REGISTER_IC	(WM_USER + 3)	//входящий вызов
#define WM_USER_REGISTER_OC	(WM_USER + 4)	//исходящий вызов
#define WM_USER_REGISTER_CN	(WM_USER + 5)	//разговорное состояние
#define WM_USER_REGISTER_DS	(WM_USER + 6)	//разговор окончен
#define WM_USER_DIGIT		(WM_USER + 7)	//отправить DTMF
#define WM_USER_CALL_ANM	(WM_USER + 8)	//ответ
#define WM_USER_CALL_DSC	(WM_USER + 9)	//отбой
#define WM_USER_ACC_MOD		(WM_USER + 10)	//изменить параметры регистрации
#define WM_USER_MIC_LEVEL	(WM_USER + 11)	//уровень микрофона
#define WM_USER_SND_LEVEL	(WM_USER + 12)	//уровень динамика
#define WM_TRAY_MESSAGE		(WM_USER + 13)

#define AES_KEY_SIZE 16
#define CHUNK_SIZE (AES_KEY_SIZE*3) // an output buffer must be a multiple of the key size

const int ci_MainHeight = 451;
const int ci_MainWidth = 344;

bool LoadConfig(HWND hWnd);

int iGetRnd(const int iMin, const int iMax);

/** \fn GetProductAndVersion(std::string& strProductVersion)
	* Получить версию приложения
	*
	* \param strProductVersion
	* \return
	*/
bool GetProductAndVersion();
void MinimizeToTray();
void RestoreFromTray();
static VOID ShowNotifyIcon(BOOL bAdd);
BOOL WINAPI OnContextMenu(HWND hwnd, int x, int y);
HWND CreateToolTip(HWND hwndTool, HWND hDlg, PTSTR pszText);
LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CConfigFile;
void SaveMainParameters(unique_ptr<CConfigFile>& pCfg);
void LoadMainParameters(unique_ptr<CConfigFile>& pCfg);

enum class enCallDir {enCallIn, enCallOut, enCallMissed};

extern list<wstring>	lSubscriberAccounts;

