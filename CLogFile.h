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
	 * ������ ������.
	 *
	 * \return true - ����� �������/ false - ������ ������� ������
	 */
	bool _Start(void);
	/**
	 *  ������� ������.
	 *
	 * \param time_out - ������� �������� ���������� ������
	 * \return true - ����� ���������� ���������/ false - ����� ���������� �������������
	 */
	bool _Stop(int time_out);
	/** \fn void LogWrite(wchar_t* msg)
	\brief ����� � ���

	\param[in] msg - ��������� �� ���������
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
	*/
	void _LogWrite(const wchar_t* msg, ...);
private:

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< ������� ��� ���������� ������ */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< ������� ������� ��������� � ������� */
	DWORD m_ThreadID = 0;						/**<  ID ������ � ����� ������ ������������ ������� */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE ������ */
	mutex m_mxQueue;
	queue<wstring> m_queue;	/**< ������� ��������� �� PJ*/

	HANDLE m_hLog{ INVALID_HANDLE_VALUE };				/**< HANDLE Log �����*/

	/**
	 * ������� ������.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief ������� ��������� ����� ������
		\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief ������� ��������� ������ ��� ����������
	\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief ������� ���� ������
	*/
	void LifeLoopOuter();
	DWORD GetTickCountDifference(DWORD nPrevTick);
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief ����������� �����

	������� ������������� ��� ��������� ������ ������, ���������� ���������� ���� ������������� �����.

	\param[out] buffer ����� ���� ����� ������� ���������
	\param[in] eaddr
	\param[in] FrameAddr
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
	*/
	template<std::size_t SIZE> wchar_t* StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr);
	/**	\fn void AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem=TRUE, WORD wType=EVENTLOG_ERROR_TYPE, WORD wCategory=0)
	\brief ����� ��������� � ������ Windows

	������� ������������� ��� ������ ��������� � ������ ������� Windows. ������� ������������ � ������ ������������� ������ � ����� Output

	\param[in] lpszMsg ��������� �� ���������
	\param[in] bSystem ���� ������������� ������ � ��������� ������ ��
	\param[in] wType ��� ���������� ���������
	\param[in] wCategory ��������� ���������
	\return void
	*/
	void AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem = TRUE, WORD wType = EVENTLOG_ERROR_TYPE, WORD wCategory = 0);
	/**
	 * �������� ��������� � �������.
	 * 
	 * \param wszMsg - ��������� �� NTS ������
	 */
	void PutMessage(const wchar_t* wszMsg);
};

