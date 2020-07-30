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
		enMT_Log,			/**< ��������� � ��� ����*/
		enMT_Status,		/**< ������/����� ��������� ��������*/
		enMT_MakeCall,		/**< ������ ��������� ������ �� ���������*/
		enMT_EndCall,		/**< ���������� ������ �� ���������*/
		enMT_AnswerCall,	/**< �������� ����� �� ���������*/
		enMT_SetMicLevel,	/**< ��������� ������ ��������� �� ���������*/
		enMT_SetSoundLevel,	/**< ��������� ������ �������� �� ���������*/
		enMT_DTMF,			/**< DTMF �� ���������*/
		enMT_AccountModify	/**< �������� ������� �� ���������*/
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
		int iErrorCode = 0;		/**< ��� ������*/
		wstring wstrErrorInfo;	/**< ����������� ������*/
	};
	struct SDisconnect : SMsg
	{
		int iCause{0};				/**< ������� ����� */
		wstring wstrInfo;		/**< �������� ������� �����*/
	};
	struct SLog : SMsg
	{
		wstring wstrInfo;		/**< ������ ��� ������ � ��� ����*/
	};
	struct SStatus : SMsg
	{
		int iStatusCode = -1;	/**< -1 - ����������� ����������� / 0 - ��� ������� ����������� / 1 - ������� ���������������*/
		wstring wstrInfo;		/**< ����������� ������� � ������ ������*/
	};
	struct SLevel : SMsg
	{
		int iValue{0};				/**< �������� ������ ���������/��������*/
	};
	struct SDTMF : SMsg
	{
		char cDTMF{'0'};				/**< DTMF ����� �����*/
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

/**
 * .
 */
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

	/** \fn bool _MakeCall(const char* wszNumber)
	\brief ������������ ��������� �����
	\param[in] wszNumber - ��������� �� ������ � �������
	\return true - ������� �����/false - ������ ������
	*/
	bool _MakeCall(const char* szNumber);

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

	/**
	 * ������� ������ ��������� � �������� ������ � ���������� � ������� �����������.
	 * 
	 * \param szCingPNum - ����� ����������� ��������
	 */
	void _IncomingCall(const char* szCingPNum);
	/**
	 * ������� ������ ��������� �� ����� �������� ������� � ���������� � ������� �����������.
	 * 
	 * \param szReason - ������� �����
	 */
	void _DisconnectRemote(int iCause, const char* szReason);
	/**
	 * ������� ������ ��������� �� ������ �������� ������� � ���������� � ������� �����������.
	 * 
	 */
	void _Connected();
	/**
	 * ������� ������ ��������� �� ������� ����������� � ���������� � ������� �����������.
	 * 
	 */
	void _RegisterOk();
	/**
	 * ������� ������ ��������� �� ������ ����������� � ���������� � ������� �����������.
	 * 
	 * \param iErr	- ��� ������
	 * \param szErrInfo	- �������� ������
	 */
	void _RegisterError(int iErr, const char* szErrInfo);
	/**
	 * ������� ���������� ��������� � ������� �����������.
	 * 
	 * \param pMsg - ������ �� ���������
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
	/**
	 * ������� ������ ��������� �� ������� �������� ������� � ���������� � ������� �����������.
	 * 
	 */
	void _Alerting();
	/**
	 * ������� ���������� ����� DTMF �������.
	 *
	 * \param wcDigit - ������ �� 0-9,*,#
	 * \return - true � ������ �����/ false � ������ ������ ��� ���������� ��������� ������
	 */
	bool _DTMF(wchar_t wcDigit);
	/**
	 * ������� ���������� ����� DTMF �������.
	 *
	 * \param cDigit - ������ �� 0-9,*,#
	 * \return - true � ������ �����/ false � ������ ������ ��� ���������� ��������� ������
	 */
	bool _DTMF(char wcDigit);
	/**
	 * ������� �������� ������� ��������� SIP ��������.
	 * 
	 */
	void _Modify();
	/**
	 * ������� ������������� �������� ��������� ���������.
	 * 
	 * \param dwLevel - �������� ��������� � ��������� 0 - �������� ��������
	 */
	void _Microfon(DWORD dwLevel);
	/**
	 * ������� ������������� �������� ��������� ���������.
	 * 
	 * \param dwLevel - �������� ��������� � ��������� 0 - ������� ��������
	 */
	void _Sound(DWORD dwLevel);

	/**
	 * �������� ������ ��������.
	 * 
	 */
	void _GetStatus();
	/**
	 * �������� ������ �� �������� ��������.
	 * 
	 * \param wstrStatus
	 */
	void _GetStatusString(wstring& wstrStatus);
private:
	HWND m_hParentWnd;		/** ���������� ������������� ���� */
	pj_thread_desc m_TH_Descriptor{ 0 };
	pj_thread_t* m_PJTH{ nullptr };

	std::string m_strLogin;		/** LOGIN ��������, ����� SIP */
	std::string m_strPassword;	/** ������ �������� */
	
	std::string m_strServerDomain;	/** �����/IP ����� SIP ������� */
	std::string m_strUserAgent;		/** �������� ����������, ������������� � ���� USER-AGENT */

	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< ������� ��� ���������� ������ */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< ������� ������� ��������� � ������� */
	DWORD m_ThreadID = 0;						/**<  ID ������ � ����� ������ ������������ ������� */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE ������ */

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

	/** \fn void SetTimer(int iTout)
	\brief ���������� ������ � �������������
	\param[in] iTout - �������� ��������� � �������������
	*/
	void SetTimer(int imsInterval);

	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< ������� ��������� �� PJ*/

	/**
	 * ���������� ��������� �� PJ.
	 * 
	 */
	void MessageSheduler();
	/**
	 * ������������� ��������� �������.
	 * 
	 */
	void PlayRingTone();

	/**
	 * ������ ��������� ������.
	 */
	enum CallState 
	{
		stNUll,			/** �������� ��������� */
		stProgress,		/** ����� ������ � ��������� ��������� �������� */
		stAlerting,		/** ������� ��������, �������� ������ �� �������� ������� */
		stRinging,		/** �������� �����, ������� ������ �� ��������� ������� */
		stAnswer,		/** ����������� ��������� */
		stDisconnect	/** ��������� ���������� ����� */
	};

	CallState m_State{ CallState::stNUll }; /** ��������� ������ */
	bool m_bReg{ false };
	int m_iRegErrorCode{ 0 };
	wstring m_wstrErrorInfo;
	/**
	 * ��������� ��������� ���������.
	 * 
	 */
	void SetNullState();

	string m_strDTMF;	/** ������ � ������� DTMF */
	wstring m_wstrNumA;	/** ����� �������� ��������� ������*/

	regex m_reAlias;
	bool GetAlias(const char* szBuf, string& strRes);
	//****************************************** PJ
	unique_ptr<Endpoint> m_ep;		/** PJ Endpoint */
	CPJLogWriter* m_log = nullptr;	/** ��������� �� �������� ��� �����*/
	unique_ptr<MyAccount> m_acc;	/** PJ Account */
};

