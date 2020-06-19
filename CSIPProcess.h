#pragma once

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
#include <pjsua2.hpp>
#include "CPJLogWriter.h"
#include "Account.h"

using namespace pj;
using namespace std;

typedef BYTE byte;
typedef WORD word;
typedef unsigned __int32 dword;
typedef unsigned short in_port_t;

namespace nsMsg
{
	enum class enMsgType
	{ 
		enMT_IncomingCall,	/**< индикация о входящем вызове*/
		enMT_Progress,		/**< индикация о принятии исходящего вызова в обработку встречной стороной*/
		enMT_Alerting,		/**< индикация о том, что удалённый абонент свободен*/
		enMT_Answer,		/**< индикация об ответе удалённого абонента*/
		enMT_Decline,		/**< индикация об отклонении вызова встречной стороной*/
		enMT_Cancel,		/**< индикация о сбросе входящего вызова удалённой стороной*/
		enMT_Disconnect,	/**< индикация отбоя вызова удалённой стороной*/
		enMT_Error,			/**< индикация об ошибке установки соединения*/
		enMT_RegSet,		/**< индикация о успешной регистрации аккаунта*/
		enMT_RegError,		/**< индикация об ошибке при регистрации аккаунта*/
		enMT_Log			/**< сообщение в лог файл*/
	};
	struct SMsg
	{
		enMsgType iCode;	/**< код сообщения*/
	};
	struct SIncCall : SMsg
	{
		wstring wstrCallingPNum;	/**< код сообщения*/
	};
	struct SProgress : SMsg 
	{
		bool bSignal;	/**< флаг необходимости включения акустического сигнала при отсутствии SDP в сообщении*/
	};
	struct SError : SMsg
	{
		int iErrorCode = 0;			/**< код ошибки*/
		wstring wstrErrorInfo;	/**< расшифровка ошибки*/
	};
	struct SDisconnect : SMsg
	{
		wstring wstrInfo;		/**< строка для записи в лог файл*/
	};
	struct SLog : SMsg
	{
		wstring wstrInfo;		/**< строка для записи в лог файл*/
	};
}

/** \fn bool TestRandomGenerator()
\brief Тест ГСЧ
\return true - ГСЧ выбран правильно/false - ГСЧ не работает
*/
bool TestRandomGenerator();
/** \fn iGetRnd(const int iMin=0,const int iMax=MAXINT)
\brief Получить случайное число из указанного диапазона

Функция предназначена для получения случайного числа из указанного диапазона. Заменяет стандартную функцию rand().

\param[in] iMin минимальное значение диапазона
\param[in] iMax максимальное значение диапазона
\return целое число из диапазона
*/
int iGetRnd(const int iMin = 0, const int iMax = MAXINT);

class CSIPProcess
{
public:
	CSIPProcess(HWND hParentWnd);
	~CSIPProcess();

	/** \fn bool _Start(void)
	\brief Запуск потока
	\return true - удачный запуск/false - поток не запущен
	*/
	bool _Start(void);
	/** \fn bool _Stop(int time_out)
	\brief Останов потока
	\param[in] time_out таймаут на завершение потока
	\return true - удачный останов/false - принудительный останов
	*/
	bool _Stop(int time_out);

	/** \fn bool _MakeCall(const wchar_t* wszNumber)
	\brief Инициировать исходящий вызов
	\param[in] wszNumber - указатель на строку с номером
	\return true - удачный вызов/false - ошибка вызова
	*/
	bool _MakeCall(const wchar_t* wszNumber);

	/** \fn bool _Answer()
	\brief Ответить на входящий вызов
	\return true - удачный вызов/false - ошибка вызова
	*/
	bool _Answer();

	/** \fn void _Disconnect()
	\brief Отбить вызов
	*/
	void _Disconnect();

	/** \fn void _Hold()
	\brief Поставить на удержание
	\return true - удачный вызов/false - ошибка вызова
	*/
	void _Hold();

	/** \fn void LogWrite(wchar_t* msg)
	\brief Вывод в лог

	\param[in] msg - указатель на сообщение
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	void _LogWrite(const wchar_t* msg, ...);

	void _IncomingCall(const char* szCingPNum);
	void _DisconnectRemote(const char* szReason);
	void _Connected();
	void _RegisterOk();
	void _RegisterError(int iErr, const char* szErrInfo);
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
	void _Alerting();
	bool _DTMF(wchar_t wcDigit);
	void _Modify();
	void _Microfon(DWORD dwLevel);
	void _Sound(DWORD dwLevel);
private:
	// Дескриптор родительского окна
	HWND m_hParentWnd;
	// LOGIN аккаунта, номер SIP
	std::string m_strLogin;
	// Пароль аккаунта
	std::string m_strPassword;
	// Домен/IP адрес SIP сервера
	std::string m_strServerDomain;
	std::string m_strUserAgent;

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< Событие для завершения потока */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< Событие наличия сообщения в очереди */
	DWORD m_ThreadID = 0;						/**<  ID потока с точки зрения операционной системы */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE потока */

	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);	/**< Функция потока */

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief Функция основного цикла потока
		\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief Функция обработки ошибки при исключении
	\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief Внешний цикл потока
	*/
	void LifeLoopOuter();
	/** \fn virtual bool Tick()
	\brief Обработчик тика
	\return true - ok/false - ошибка
	*/
	bool Tick();
	HANDLE m_hTimer{ INVALID_HANDLE_VALUE };	/**< указатель на таймер */

	DWORD GetTickCountDifference(DWORD nPrevTick);
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief Трассировка стека

	Функция предназначена для получения адреса модуля, вызвавшего исключение путём раскручивания стека.

	\param[out] buffer Буфер куда будет помещён результат
	\param[in] eaddr
	\param[in] FrameAddr
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	template<std::size_t SIZE> wchar_t* StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr);

	/** \fn bool DataInit()
	\brief Вывод в лог

	\param[in] msg - указатель на сообщение
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	bool DataInit();
	/** \fn GetProductAndVersion(std::string& strProductVersion)
	 * Получить версию приложения
	 * 
	 * \param strProductVersion
	 * \return 
	 */
	bool GetProductAndVersion(std::string& strProductVersion);

	/** \fn void SetTimer(int iTout)
	\brief установить таймер в миллисекундах
	\param[in] iTout - величина интервала в миллисекундах
	*/
	void SetTimer(int imsInterval);

	HANDLE m_hLog;				/**< HANDLE Log файла*/
	/** \fn void LogWrite(wchar_t* msg)
	\brief Вывод в лог

	\param[in] msg - указатель на сообщение
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	void LogWrite(const wchar_t* msg, ...);
	/**	\fn void AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem=TRUE, WORD wType=EVENTLOG_ERROR_TYPE, WORD wCategory=0)
	\brief Вывод сообщения в журнал Windows

	Функция предназначена для вывода сообщений в журнал событий Windows. Функция используется в случае невозможности вывода в поток Output

	\param[in] lpszMsg Указатель на сообщение
	\param[in] bSystem Флаг необходимости вывода в системный журнал ОС
	\param[in] wType Тип выводимого сообщения
	\param[in] wCategory Категория сообщения
	\return void
	*/
	void AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem = TRUE, WORD wType = EVENTLOG_ERROR_TYPE, WORD wCategory = 0);

	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< очередь сообщений от PJ*/

	void MessageSheduler();
	void PlayRingTone();

	enum CallState 
	{
		stNUll, stProgress, stAlerting, stRinging, stAnswer, stDisconnect
	};

	CallState m_State{ CallState::stNUll };

	bool m_bTicker{ false };
	void SetNullState();

	wstring m_wstrDTMF;
	//****************************************** PJ
	unique_ptr<Endpoint> m_ep;
	CPJLogWriter* m_log = nullptr;
	unique_ptr<MyAccount> m_acc;
};

