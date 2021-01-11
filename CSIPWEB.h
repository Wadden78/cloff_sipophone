/*****************************************************************//**
 * \file   CSIPWEB.h
 * \brief  ����� ��������� ������� SIP �������������� web �����������
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
	/**
	 * \brief ������� ��������� ������.
	 *
	 * \return
	 */
	nsMsg::CallState _GetCallState() { return m_State; }
private:
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

	nsMsg::CallState m_State{ nsMsg::CallState::stNUll }; /** ��������� ������ */
	bool m_bReg{ false };
	int m_iRegErrorCode{ 0 };
	wstring m_wstrErrorInfo;
	/**
	 * ��������� ��������� ���������.
	 *
	 */
	void SetNullState();

	string m_strDTMF;			/** ������ � ������� DTMF */
	wstring m_wstrRemoteNumber;	/** ����� ��������� ��������*/
	enListType m_CallType{ enListType::enLTAll };	//��� ������

	regex m_reAlias;
	wregex m_reAliasW;
	bool GetAlias(const char* szBuf, string& strRes);
	bool GetAliasW(wstring& szBuf, wstring& strRes);

	time_t m_rawtimeCallBegin{ 0 }; /** ����� ������ ������*/
	time_t m_rawtimeCallAnswer{ 0 }; /** ����� ������*/
	size_t NumParser(const char* pszIn, string& strOut);
	//****************************************** PJ
	unique_ptr<Endpoint> m_ep;		/** PJ Endpoint */
	CPJLogWriter* m_log = nullptr;	/** ��������� �� �������� ��� �����*/
	unique_ptr<MyAccount> m_acc;	/** PJ Account */
};

