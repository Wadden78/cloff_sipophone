#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <list>
#include <commctrl.h>
#include <locale.h>
#include "resource.h"
#include "CLogFile.h"
#include "CSIPProcess.h"
#include "id_list.h"
#include "CSIPWEB.h"

extern HINSTANCE hInstance;

extern std::unique_ptr<CSIPWEB> m_SIPWEB;
extern CLogFile m_Log;
extern std::unique_ptr<CSIPProcess> m_SIPProcess;
class CWebSocket;
extern std::unique_ptr<CWebSocket> m_WebSocketServer;
class CHistory;
extern std::unique_ptr<CHistory> m_History;
class CPhoneBook;
extern std::unique_ptr<CPhoneBook> m_PhoneBook;
extern in_port_t ipWSPort;
extern string strProductName;
extern string strProductVersion;
extern bool bMinimizeOnStart;	/** Минимизация после старта*/
extern bool bAutoStart;			/** Автоматический запуск после старта ОС*/
extern std::wstring wstrWEBServer;
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
extern HWND hStatusInfo;
extern HWND hBtMute;
extern HWND hBtSilence;
extern HWND hBtDial;
extern HWND hBtDisconnect;
extern HWND hBtPhoneBook;
extern HWND hBtContactState;

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
#define WM_INFO_MESSAGE		(WM_USER + 14)	//Запрос каталога и аккаунта для запрета запуска второй копии
//#define WM_USER_INITCALL	(WM_USER + 15)	//Инициировать вызов
#define WM_CCCLOSE_MESSAGE	(WM_USER + 16)	//Карточка клиента сообщает о своём закрытии 
#define WM_USER_SIPWEB_ON	(WM_USER + 17)	//Создать поток SIP веб режима
#define WM_USER_SIPWEB_OFF	(WM_USER + 18)	//Удалить поток SIP веб режима
#define WM_USER_SIPWEB_MC	(WM_USER + 19)	//Инициировать исходящий вызов SIP веб режима

#define AES_KEY_SIZE 16
#define CHUNK_SIZE (AES_KEY_SIZE*3) // an output buffer must be a multiple of the key size

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

class CConfigFile;
void LoadMainParameters(unique_ptr<CConfigFile>& pCfg);
void SaveJSONMainParameters();
void LoadJSONMainParameters();
void SetButtonStyle(LPDRAWITEMSTRUCT& pDIS);
void DrawButton(HWND hCurrentButton, HBITMAP hBitmap);
void CorrectBoolParameter(const char* szParamName, bool bValue);
void CorrectStringParameter(const char* szParamName, const wchar_t* wszValue);
void SetAutostart(bool bSet);

extern array<wchar_t, 1024> csEXEPath;
extern bool bDTMF_2833;
extern bool bDebugMode;
extern bool bConfigMode;

enum class enCallDir {enCallIn, enCallOut, enCallMissed};

extern list<wstring>	lSubscriberAccounts;

int CheckCopyStart();

#define X_COORD_NAME "x"
#define Y_COORD_NAME "y"
#define W_WIDTH_NAME "w"
#define H_HIGH_NAME  "h"

#define MAIN_OBJ_NAME			"main"
#define MAIN_TRANSPORT_NAME		"transport"
#define MAIN_DTMF_NAME			"dtmf"
#define MAIN_RING_NAME			"ring"
#define MAIN_MIC_NAME			"mic_level"
#define MAIN_SND_NAME			"snd_level"
#define MAIN_MINONSTART_NAME	"minimize_on_start"
#define MAIN_AUTOSTART_NAME		"autostart"
#define MAIN_WS_NAME			"webserver"
#define MAIN_DATA_NAME			"start_data"

const int ci_MainBigButton_Width = 120;		/**< Ширина больших кнопок */
const int ci_MainBigButton_High = 40;		/**< Высота больших кнопок */
const int ci_MainMiddleButton_Width = 80;	/**< Ширина средних кнопок */
const int ci_MainMiddleButton_High = 40;	/**< Высота средних кнопок */

const int ci_MainXInterval = 2;				/**< Интервал между элементами по горизонтали */
const int ci_MainYInterval = 2;				/**< Интервал между элементами по вертикали */

const int ci_MainComboBox_High = 40;		/**< Высота выпадающего списка набранных номеров */

const int ci_MainWidth = ci_MainMiddleButton_Width * 4 + ci_MainXInterval * 5 + 16;
const int ci_MainHeight = ci_MainMiddleButton_High * 11 + ci_MainYInterval * 11;

const int ci_MainSlider_Width = ci_MainMiddleButton_Width*3 + ci_MainXInterval * 2;	/**< Ширина ползунков */

