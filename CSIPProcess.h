#pragma once

#include <winsock2.h>
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
#include <regex>
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
		enMT_Log,			/**< сообщение в лог файл*/
		enMT_Status,		/**< запрос/ответ состояния аккаунта*/
		enMT_MakeCall,		/**< запрос инициации вызова от вебсокета*/
		enMT_EndCall,		/**< завершение вызова от вебсокета*/
		enMT_AnswerCall,	/**< ответить вызов от вебсокета*/
		enMT_SetMicLevel,	/**< установка уровня микрофона от вебсокета*/
		enMT_SetSoundLevel,	/**< установка уровня динамика от вебсокета*/
		enMT_DTMF,			/**< DTMF от вебсокета*/
		enMT_AccountModify	/**< изменить аккаунт от вебсокета*/
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
		int iErrorCode = 0;		/**< код ошибки*/
		wstring wstrErrorInfo;	/**< расшифровка ошибки*/
	};
	struct SDisconnect : SMsg
	{
		int iCause{0};				/**< причина отбоя */
		wstring wstrInfo;		/**< описание причины отбоя*/
	};
	struct SLog : SMsg
	{
		wstring wstrInfo;		/**< строка для записи в лог файл*/
	};
	struct SStatus : SMsg
	{
		int iStatusCode = -1;	/**< -1 - Регистрация отсутствует / 0 - идёт процесс регистрации / 1 - аккаунт зарегистрирован*/
		wstring wstrInfo;		/**< расшифровка статуса в случае ошибки*/
	};
	struct SLevel : SMsg
	{
		int iValue{0};				/**< Значение уровня микрофона/динамика*/
	};
	struct SDTMF : SMsg
	{
		char cDTMF{'0'};				/**< DTMF цифра набор*/
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

/**
 * .
 */
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

	/** \fn bool _MakeCall(const char* wszNumber)
	\brief Инициировать исходящий вызов
	\param[in] wszNumber - указатель на строку с номером
	\return true - удачный вызов/false - ошибка вызова
	*/
	bool _MakeCall(const char* szNumber);

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

	/**
	 * Функция создаёт сообщение о входящем вызове и отправляет в очередь обработчика.
	 * 
	 * \param szCingPNum - номер вызывающего абонента
	 */
	void _IncomingCall(const char* szCingPNum);
	/**
	 * Функция создаёт сообщение об отбое удалённой стороны и отправляет в очередь обработчика.
	 * 
	 * \param szReason - причина отбоя
	 */
	void _DisconnectRemote(int iCause, const char* szReason);
	/**
	 * Функция создаёт сообщение об ответе удалённой стороны и отправляет в очередь обработчика.
	 * 
	 */
	void _Connected();
	/**
	 * Функция создаёт сообщение об удачной регистрации и отправляет в очередь обработчика.
	 * 
	 */
	void _RegisterOk();
	/**
	 * Функция создаёт сообщение об ошибке регистрации и отправляет в очередь обработчика.
	 * 
	 * \param iErr	- код ошибки
	 * \param szErrInfo	- описание ошибки
	 */
	void _RegisterError(int iErr, const char* szErrInfo);
	/**
	 * Функция отправляет сообщение в очередь обработчика.
	 * 
	 * \param pMsg - ссылка на сообщение
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
	/**
	 * Функция создаёт сообщение об отклике удалённой стороны и отправляет в очередь обработчика.
	 * 
	 */
	void _Alerting();
	/**
	 * Функция отправляет набор DTMF сигнала.
	 *
	 * \param wcDigit - сигнал от 0-9,*,#
	 * \return - true в случае удачи/ false в случае ошибки или отсутствии активного вызова
	 */
	bool _DTMF(wchar_t wcDigit);
	/**
	 * Функция отправляет набор DTMF сигнала.
	 *
	 * \param cDigit - сигнал от 0-9,*,#
	 * \return - true в случае удачи/ false в случае ошибки или отсутствии активного вызова
	 */
	bool _DTMF(char wcDigit);
	/**
	 * Функция изменяет текущие настройки SIP аккаунта.
	 * 
	 */
	void _Modify();
	/**
	 * Функция устанавливает значение громкости микрофона.
	 * 
	 * \param dwLevel - значение громкости в процентах 0 - микрофон выключен
	 */
	void _Microfon(DWORD dwLevel);
	/**
	 * Функция устанавливает значение громкости динамиков.
	 * 
	 * \param dwLevel - значение громкости в процентах 0 - динамик выключен
	 */
	void _Sound(DWORD dwLevel);

	/**
	 * Получить статус аккаунта.
	 * 
	 */
	void _GetStatus();
	/**
	 * Получить строку со статусом аккаунта.
	 * 
	 * \param wstrStatus
	 */
	void _GetStatusString(wstring& wstrStatus);
private:
	HWND m_hParentWnd;		/** Дескриптор родительского окна */
	pj_thread_desc m_TH_Descriptor{ 0 };
	pj_thread_t* m_PJTH{ nullptr };

	std::string m_strLogin;		/** LOGIN аккаунта, номер SIP */
	std::string m_strPassword;	/** Пароль аккаунта */
	
	std::string m_strServerDomain;	/** Домен/IP адрес SIP сервера */
	std::string m_strUserAgent;		/** Название приложения, подставляемое в поле USER-AGENT */

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< Событие для завершения потока */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< Событие наличия сообщения в очереди */
	DWORD m_ThreadID = 0;						/**<  ID потока с точки зрения операционной системы */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE потока */

	/**
	 * Функция потока.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

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

	/** \fn void SetTimer(int iTout)
	\brief установить таймер в миллисекундах
	\param[in] iTout - величина интервала в миллисекундах
	*/
	void SetTimer(int imsInterval);

	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< очередь сообщений от PJ*/

	/**
	 * Обработчик сообщений от PJ.
	 * 
	 */
	void MessageSheduler();
	/**
	 * Проигрыватель вызывного сигнала.
	 * 
	 */
	void PlayRingTone();

	/**
	 * Список состояний вызова.
	 */
	enum CallState 
	{
		stNUll,			/** исходное состояние */
		stProgress,		/** вызов принят в обработку встречной стороной */
		stAlerting,		/** абонент свободен, ожидание ответа от удалённой стороны */
		stRinging,		/** входящий вызов, ожидаем ответа от локальной стороны */
		stAnswer,		/** разговорное состояние */
		stDisconnect	/** состояние завершения отбоя */
	};

	CallState m_State{ CallState::stNUll }; /** состояние вызова */
	bool m_bReg{ false };
	int m_iRegErrorCode{ 0 };
	wstring m_wstrErrorInfo;
	/**
	 * установка исходного состояния.
	 * 
	 */
	void SetNullState();

	string m_strDTMF;	/** строка с цифрами DTMF */
	wstring m_wstrNumA;	/** номер абонента входящего вызова*/

	regex m_reAlias;
	bool GetAlias(const char* szBuf, string& strRes);
	//****************************************** PJ
	unique_ptr<Endpoint> m_ep;		/** PJ Endpoint */
	CPJLogWriter* m_log = nullptr;	/** указатель на контекст лог файла*/
	unique_ptr<MyAccount> m_acc;	/** PJ Account */
};

