/*****************************************************************//**
 * \file   CSIPWEB.h
 * \brief  Поток обработки вызовов SIP инициированных web приложением
 * 
 * \author Wadden
 * \date   December 2020
 *********************************************************************/
#pragma once
#include "CSIPProcess.h"

class CSIPWEB
{
public:
	CSIPWEB(const char* szLogin, const char* szPassword);
	~CSIPWEB();

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
	/**
	 * \brief вернуть состояние вызова.
	 *
	 * \return
	 */
	nsMsg::CallState _GetCallState() { return m_State; }
private:
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

	nsMsg::CallState m_State{ nsMsg::CallState::stNUll }; /** состояние вызова */
	bool m_bReg{ false };
	int m_iRegErrorCode{ 0 };
	wstring m_wstrErrorInfo;
	/**
	 * установка исходного состояния.
	 *
	 */
	void SetNullState();

	string m_strDTMF;			/** строка с цифрами DTMF */
	wstring m_wstrRemoteNumber;	/** номер удалённого абонента*/
	enListType m_CallType{ enListType::enLTAll };	//тип вызова

	regex m_reAlias;
	wregex m_reAliasW;
	bool GetAlias(const char* szBuf, string& strRes);
	bool GetAliasW(wstring& szBuf, wstring& strRes);

	time_t m_rawtimeCallBegin{ 0 }; /** время начала вызова*/
	time_t m_rawtimeCallAnswer{ 0 }; /** время ответа*/
	size_t NumParser(const char* pszIn, string& strOut);
	//****************************************** PJ
	unique_ptr<Endpoint> m_ep;		/** PJ Endpoint */
	CPJLogWriter* m_log = nullptr;	/** указатель на контекст лог файла*/
	unique_ptr<MyAccount> m_acc;	/** PJ Account */
};

