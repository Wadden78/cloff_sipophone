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

using namespace std;

class CLogFile
{
public:
	CLogFile();
	virtual ~CLogFile();
	/**
	 * Запуск потока.
	 *
	 * \return true - поток запущен/ false - ошибка запуска потока
	 */
	bool _Start(void);
	/**
	 *  Останов потока.
	 *
	 * \param time_out - таймаут ожидания завершения потока
	 * \return true - поток остановлен нормально/ false - поток остановлен принудительно
	 */
	bool _Stop(int time_out);
	/** \fn void LogWrite(wchar_t* msg)
	\brief Вывод в лог

	\param[in] msg - указатель на сообщение
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	void _LogWrite(const wchar_t* msg, ...);
private:

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< Событие для завершения потока */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< Событие наличия сообщения в очереди */
	DWORD m_ThreadID = 0;						/**<  ID потока с точки зрения операционной системы */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE потока */
	mutex m_mxQueue;
	queue<wstring> m_queue;	/**< очередь сообщений от PJ*/

	HANDLE m_hLog{ INVALID_HANDLE_VALUE };				/**< HANDLE Log файла*/

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
	/**
	 * Положить сообщение в очередь.
	 * 
	 * \param wszMsg - указатель на NTS строку
	 */
	void PutMessage(const wchar_t* wszMsg);
};

