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
		enMT_IncomingCall,	/**< ��������� � �������� ������*/
		enMT_Progress,		/**< ��������� � �������� ���������� ������ � ��������� ��������� ��������*/
		enMT_Alerting,		/**< ��������� � ���, ��� �������� ������� ��������*/
		enMT_Answer,		/**< ��������� �� ������ ��������� ��������*/
		enMT_Decline,		/**< ��������� �� ���������� ������ ��������� ��������*/
		enMT_Cancel,		/**< ��������� � ������ ��������� ������ �������� ��������*/
		enMT_Disconnect,	/**< ��������� ����� ������ �������� ��������*/
		enMT_Error,			/**< ��������� �� ������ ��������� ����������*/
		enMT_RegSet,		/**< ��������� � �������� ����������� ��������*/
		enMT_RegError,		/**< ��������� �� ������ ��� ����������� ��������*/
		enMT_Log			/**< ��������� � ��� ����*/
	};
	struct SMsg
	{
		enMsgType iCode;	/**< ��� ���������*/
	};
	struct SIncCall : SMsg
	{
		wstring wstrCallingPNum;	/**< ��� ���������*/
	};
	struct SProgress : SMsg 
	{
		bool bSignal;	/**< ���� ������������� ��������� ������������� ������� ��� ���������� SDP � ���������*/
	};
	struct SError : SMsg
	{
		int iErrorCode = 0;			/**< ��� ������*/
		wstring wstrErrorInfo;	/**< ����������� ������*/
	};
	struct SDisconnect : SMsg
	{
		wstring wstrInfo;		/**< ������ ��� ������ � ��� ����*/
	};
	struct SLog : SMsg
	{
		wstring wstrInfo;		/**< ������ ��� ������ � ��� ����*/
	};
}

/** \fn bool TestRandomGenerator()
\brief ���� ���
\return true - ��� ������ ���������/false - ��� �� ��������
*/
bool TestRandomGenerator();
/** \fn iGetRnd(const int iMin=0,const int iMax=MAXINT)
\brief �������� ��������� ����� �� ���������� ���������

������� ������������� ��� ��������� ���������� ����� �� ���������� ���������. �������� ����������� ������� rand().

\param[in] iMin ����������� �������� ���������
\param[in] iMax ������������ �������� ���������
\return ����� ����� �� ���������
*/
int iGetRnd(const int iMin = 0, const int iMax = MAXINT);

class CSIPProcess
{
public:
	CSIPProcess(HWND hParentWnd);
	~CSIPProcess();

	/** \fn bool _Start(void)
	\brief ������ ������
	\return true - ������� ������/false - ����� �� �������
	*/
	bool _Start(void);
	/** \fn bool _Stop(int time_out)
	\brief ������� ������
	\param[in] time_out ������� �� ���������� ������
	\return true - ������� �������/false - �������������� �������
	*/
	bool _Stop(int time_out);

	/** \fn bool _MakeCall(const wchar_t* wszNumber)
	\brief ������������ ��������� �����
	\param[in] wszNumber - ��������� �� ������ � �������
	\return true - ������� �����/false - ������ ������
	*/
	bool _MakeCall(const wchar_t* wszNumber);

	/** \fn bool _Answer()
	\brief �������� �� �������� �����
	\return true - ������� �����/false - ������ ������
	*/
	bool _Answer();

	/** \fn void _Disconnect()
	\brief ������ �����
	*/
	void _Disconnect();

	/** \fn void _Hold()
	\brief ��������� �� ���������
	\return true - ������� �����/false - ������ ������
	*/
	void _Hold();

	/** \fn void LogWrite(wchar_t* msg)
	\brief ����� � ���

	\param[in] msg - ��������� �� ���������
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
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
	// ���������� ������������� ����
	HWND m_hParentWnd;
	// LOGIN ��������, ����� SIP
	std::string m_strLogin;
	// ������ ��������
	std::string m_strPassword;
	// �����/IP ����� SIP �������
	std::string m_strServerDomain;
	std::string m_strUserAgent;

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< ������� ��� ���������� ������ */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< ������� ������� ��������� � ������� */
	DWORD m_ThreadID = 0;						/**<  ID ������ � ����� ������ ������������ ������� */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE ������ */

	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);	/**< ������� ������ */

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
	/** \fn virtual bool Tick()
	\brief ���������� ����
	\return true - ok/false - ������
	*/
	bool Tick();
	HANDLE m_hTimer{ INVALID_HANDLE_VALUE };	/**< ��������� �� ������ */

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

	/** \fn bool DataInit()
	\brief ����� � ���

	\param[in] msg - ��������� �� ���������
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
	*/
	bool DataInit();
	/** \fn GetProductAndVersion(std::string& strProductVersion)
	 * �������� ������ ����������
	 * 
	 * \param strProductVersion
	 * \return 
	 */
	bool GetProductAndVersion(std::string& strProductVersion);

	/** \fn void SetTimer(int iTout)
	\brief ���������� ������ � �������������
	\param[in] iTout - �������� ��������� � �������������
	*/
	void SetTimer(int imsInterval);

	HANDLE m_hLog;				/**< HANDLE Log �����*/
	/** \fn void LogWrite(wchar_t* msg)
	\brief ����� � ���

	\param[in] msg - ��������� �� ���������
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
	*/
	void LogWrite(const wchar_t* msg, ...);
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

	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< ������� ��������� �� PJ*/

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

