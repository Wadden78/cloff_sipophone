/*****************************************************************//**
 * \file   CSIPWEB.cpp
 * \brief  Поток обработки вызовов SIP инициированных web приложением
 * 
 * \author Wadden
 * \date   December 2020
 *********************************************************************/
#include "CSIPWEB.h"
#include "resource.h"
#include <stdio.h>
#include <conio.h>
#include <wchar.h>
#include <datetimeapi.h>
#include <functional>
#include <chrono>
#include "Main.h"
#include "CWebSocket.h"

CSIPWEB::CSIPWEB(const char* szLogin, const char* szPassword)
{
	m_strLogin = szLogin;
	m_strPassword = szPassword;

	if(!strProductName.length()) m_strUserAgent = "NV CLOFF PHONE V.0.1";
	else m_strUserAgent = strProductName + " V." + strProductVersion;

	std::array<char, 256> szServer{ 0 };
	sprintf_s(szServer.data(), szServer.size(), "%ls", wstrServer.c_str());
	m_strServerDomain = szServer.data();
	if(iTransportType == IPPROTO_TCP) m_strServerDomain += ";transport=tcp";

	m_hEvtStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvtQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	m_reAlias.assign("sips?:(\\S*)@", std::regex_constants::icase);
	m_reAliasW.assign(L"sips?:(\\S*)@", std::regex_constants::icase);

	m_Log._LogWrite(L"   SIPWEB: Constructor");
}

CSIPWEB::~CSIPWEB()
{
	CloseHandle(m_hTimer);
	CloseHandle(m_hEvtQueue);
	CloseHandle(m_hEvtStop);

	m_Log._LogWrite(L"   SIPWEB: Destructor");
}

bool CSIPWEB::_Start(void)
{
	m_Log._LogWrite(L"   SIPWEB: _Start");
	if(DataInit())	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	else m_Log._LogWrite(L"   SIPWEB: Error at DataInit()");

	return m_hThread == NULL ? false : true;
}

bool CSIPWEB::_Stop(int time_out)
{
	m_Log._LogWrite(L"   SIPWEB: Stop");
	if(m_hThread != NULL)
	{
		SetEvent(m_hEvtStop);
		if(WaitForSingleObject(m_hThread, time_out) == WAIT_TIMEOUT) TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return true;
}
// Функция вывода сообщения о сбое
void CSIPWEB::ErrorFilter(LPEXCEPTION_POINTERS ep)
{
	std::array<wchar_t, 1024> szBuf;
	swprintf_s(szBuf.data(), szBuf.size(), L"ПЕРЕХВАЧЕНА ОШИБКА ПО АДРЕСУ 0x%p", ep->ExceptionRecord->ExceptionAddress);
	m_Log._LogWrite(L"%s", szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"%s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	m_Log._LogWrite(L"%s", szBuf.data());
}

// Функция потока
unsigned __stdcall CSIPWEB::s_ThreadProc(LPVOID lpParameter)
{
	((CSIPWEB*)lpParameter)->LifeLoopOuter();
	return 0;
}

// Внешний цикл потока
void CSIPWEB::LifeLoopOuter()
{
	int nCriticalErrorCounter = 0;
	int iExitCode = 0;
	unsigned long nTick;
	BOOL repeat = TRUE;
	std::array<wchar_t, 256> chMsg;
	while(repeat)
	{
		iExitCode = 0;
		__try
		{
			LifeLoop(nCriticalErrorCounter != 0);
			repeat = FALSE;
		}
		__except(ErrorFilter(GetExceptionInformation()), 1)
		{
			swprintf_s(chMsg.data(), chMsg.size(), L"%S", "Last message buffer is empty!");
			m_Log._LogWrite(L"%s", chMsg.data());
			if(nCriticalErrorCounter == 0) nTick = GetTickCount();
			nCriticalErrorCounter++;
			if(nCriticalErrorCounter > 4)
			{
				nCriticalErrorCounter = 0;
				nTick = GetTickCountDifference(nTick);
				if(nTick < 60000)
				{
					m_Log._LogWrite(L"%s", LPTSTR(L"5 СБОЕВ НА ПОТОКЕ В ТЕЧЕНИИ МИНУТЫ !!!!  ПОТОК ОСТАНОВЛЕН"));
					repeat = FALSE;
					break;
				}
			}
			Sleep(1000);
			iExitCode = 1;
		}
	}
	_endthreadex(iExitCode);
}

DWORD CSIPWEB::GetTickCountDifference(DWORD nPrevTick)
{
	DWORD nTick = GetTickCount64();
	if(nTick >= nPrevTick)	nTick -= nPrevTick;
	else					nTick += ULONG_MAX - nPrevTick;
	return nTick;
}

//Трассировка стека
template<std::size_t SIZE> wchar_t* CSIPWEB::StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr)
{
	int pos = swprintf_s(buffer.data(), buffer.size(), L"СТЕК: ");
	pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"0x%08X ", (DWORD)eaddr);
	__try
	{
		for(int i = 0; i < 16 && FrameAddr; i++)
		{
			pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"0x%08X ", *(DWORD*)(FrameAddr + 4));
			FrameAddr = *(DWORD*)(FrameAddr);
		}
	}
	__except(1)
	{
		pos += swprintf_s(&buffer[pos], buffer.size() - pos, L"****");
		FrameAddr = 0;
	}
	if(FrameAddr) swprintf_s(&buffer[pos], buffer.size() - pos, L"...");

	return buffer.data();
}

void CSIPWEB::LifeLoop(BOOL bRestart)
{
	HANDLE events[4];
	events[0] = m_hEvtStop;
	events[1] = m_hEvtQueue;
	events[2] = m_hTimer;

	m_Log._LogWrite(L"   SIPWEB: LifeLoop");
	SetTimer(100);

	bool bRetry = true;
	while(bRetry)
	{
		DWORD evtno = WaitForMultipleObjects(3, events, FALSE, INFINITE);
		evtno -= WAIT_OBJECT_0;
		switch(evtno)
		{
			case 0:	//Stop
			{
				bRetry = false;
				m_Log._LogWrite(L"   SIPWEB: Поток останавливается...");
				try
				{
					if(m_acc)
					{
						m_acc->_Disconnect();
						m_acc->_DeleteCall();
					}
				}
				catch(Error& err)
				{
					m_Log._LogWrite(L"   SIPWEB: Shutdown error: %S", err.info());
				}
				break;
			}
			case 1:
			{
				MessageSheduler();
				break;
			}
			case 2:
			{
				Tick();
				break;
			}
		}
	}
}

bool CSIPWEB::Tick()
{
	bool bRet = false;

	switch(m_State)
	{
		case nsMsg::CallState::stNUll:	break;
		case nsMsg::CallState::stAlerting:
		case nsMsg::CallState::stProgress:
			if(m_strDTMF.length() && m_strDTMF[0] == ',')
			{
				m_strDTMF.erase(0, 1);
				while(m_strDTMF.length())
				{
					if(m_strDTMF[0] != ',') m_strDTMF.erase(0, 1);
					else break;
				}
			}
			SetTimer(1000);
			break;
		case nsMsg::CallState::stRinging:	break;
		case nsMsg::CallState::stAnswer:
		{
			if(m_strDTMF.length() && m_strDTMF[0] == ',')
			{
				m_strDTMF.erase(0, 1);
				while(m_strDTMF.length())
				{
					if(m_strDTMF[0] != ',') m_strDTMF.erase(0, 1);
					else
					{
						SetTimer(1000);
						break;
					}
				}
			}
			else SetTimer(600000);
			break;
		}
	}
	return bRet;
}

void CSIPWEB::MessageSheduler()
{
	auto qsize = m_queue.size();
	while(qsize--)
	{
		unique_ptr<nsMsg::SMsg> pmsg;
		{
			std::lock_guard<std::mutex> _m_lock(m_mxQueue);
			pmsg = move(m_queue.front());
			m_queue.pop();
		}
		switch(pmsg->iCode)
		{
			case nsMsg::enMsgType::enMT_IncomingCall:
			{
				auto pinc = static_cast<nsMsg::SIncCall*>(pmsg.get());
				m_wstrRemoteNumber = pinc->wstrCallingPNum;

				m_CallType = enListType::enLTMiss;
				m_rawtimeCallBegin = time(nullptr);
				m_rawtimeCallAnswer = 0;

				m_State = nsMsg::CallState::stRinging;

				PlayRingTone();

				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Progress:
			{
				m_State = nsMsg::CallState::stProgress;
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Alerting:
			{
				m_State = nsMsg::CallState::stAlerting;
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Answer:
			{
				m_rawtimeCallAnswer = time(nullptr);
				PlaySound(NULL, NULL, 0);

				if(m_strDTMF.length() && m_strDTMF[0] == L';')
				{
					m_strDTMF.erase(0, 1);
					while(m_strDTMF.length())
					{
						if(m_strDTMF[0] != ',')	m_strDTMF.erase(0, 1);
						else
						{
							SetTimer(1000);
							break;
						}
					}
				}
				else SetTimer(600000);

				m_State = nsMsg::CallState::stAnswer;
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Decline:
			{
				Beep(425, 500);
				Sleep(500);
				Beep(425, 500);
				SetNullState();
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Cancel:
			{
				SetNullState();
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Disconnect:
			{
				if(m_acc) m_acc->_DeleteCall();
				SetNullState();
				Beep(425, 500);
				Sleep(500);
				Beep(425, 500);
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Error:
			{
				SetNullState();
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_RegSet:
			{
				m_iRegErrorCode = 0;
				m_wstrErrorInfo.clear();
				m_WebSocketServer->_PutMessage(pmsg);
				m_bReg = true;
				break;
			}
			case nsMsg::enMsgType::enMT_RegError:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				m_bReg = false;
				m_iRegErrorCode = Errmsg->iErrorCode;
				m_wstrErrorInfo = Errmsg->wstrErrorInfo;
				m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Log:
			{
				auto plog = static_cast<nsMsg::SLog*>(pmsg.get());
				m_Log._LogWrite(L"%s", plog->wstrInfo.c_str());
				break;
			}
			case nsMsg::enMsgType::enMT_Status:
			{
				unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SStatus>();
				pMsg->iCode = nsMsg::enMsgType::enMT_Status;
				static_cast<nsMsg::SStatus*>(pMsg.get())->iStatusCode = m_bReg ? 1 : -1;
				if(!m_bReg)
				{
					static_cast<nsMsg::SStatus*>(pMsg.get())->iStatusCode = m_iRegErrorCode;
					static_cast<nsMsg::SStatus*>(pMsg.get())->wstrInfo = m_wstrErrorInfo;
				}
				m_WebSocketServer->_PutMessage(pMsg);
				break;
			}
			case nsMsg::enMsgType::enMT_MakeCall:
			{
				auto msg = static_cast<nsMsg::SIncCall*>(pmsg.get());
				_MakeCall(msg->wstrCallingPNum.c_str());
				break;
			}
			case nsMsg::enMsgType::enMT_EndCall:
			{
				_Disconnect();
				break;
			}
			case nsMsg::enMsgType::enMT_AnswerCall:
			{
				_Answer();
				break;
			}
			case nsMsg::enMsgType::enMT_SetMicLevel:
			{
				auto msg = static_cast<nsMsg::SLevel*>(pmsg.get());
				_Microfon(msg->iValue);
				break;
			}
			case nsMsg::enMsgType::enMT_SetSoundLevel:
			{
				auto msg = static_cast<nsMsg::SLevel*>(pmsg.get());
				_Sound(msg->iValue);
				break;
			}
			case nsMsg::enMsgType::enMT_DTMF:
			{
				auto msg = static_cast<nsMsg::SDTMF*>(pmsg.get());
				_DTMF(msg->cDTMF);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

bool CSIPWEB::DataInit()
{
	bool bRet = true;

	try
	{
		m_ep = make_unique<Endpoint>();
		m_ep->libCreate();
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPWEB: Startup error: %S", err.info().c_str());
		bRet = false;
	}
	// Initialize endpoint
	EpConfig ep_cfg;
	ep_cfg.logConfig.writer = new CPJLogWriter;
	ep_cfg.logConfig.msgLogging = bDebugMode ? 1 : 0;
	ep_cfg.uaConfig.userAgent = m_strUserAgent;
	try
	{
		m_ep->libInit(ep_cfg);
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPWEB: Initialization error: %S", err.info().c_str());
		bRet = false;
	}
	// Create SIP transport. Error handling sample is shown
	TransportConfig tcfg;
	try
	{
		if(iTransportType == IPPROTO_TCP) m_ep->transportCreate(PJSIP_TRANSPORT_TCP, tcfg);
		else
		{
			tcfg.port = 5070;
			tcfg.portRange = 1000;
			m_ep->transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
		}
		m_Log._LogWrite(L"   SIPWEB: Transport create Ok. ");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPWEB: Transport create error: %S", err.info().c_str());
		bRet = false;
	}

	try
	{
		m_ep->codecSetPriority("pcma", 1);
		m_ep->codecSetPriority("pcmu", 0);
		m_ep->codecSetPriority("speex/16000", 0);
		m_ep->codecSetPriority("speex/8000", 0);
		m_ep->codecSetPriority("speex/32000", 0);
		m_ep->codecSetPriority("iLBC/8000", 0);
		m_Log._LogWrite(L"   SIPWEB: EndPoint Set codec priority to PCMA");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPWEB: EndPoint Set codec priority error: %S", err.info().c_str());
		bRet = false;
	}
	// Start the library (worker threads etc)
	try
	{
		m_ep->libStart();
		m_Log._LogWrite(L"   SIPWEB: *** PJSUA2 STARTED ***");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPWEB: EndPoint lib start error: %S", err.info().c_str());
		bRet = false;
	}

	if(m_strLogin.length())
	{
		try
		{
			m_acc = make_unique<MyAccount>();
			if(m_acc->_Create(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) m_Log._LogWrite(L"   SIPWEB: Account create ok");
		}
		catch(Error& err)
		{
			m_Log._LogWrite(L"   SIPWEB: EndPoint lib start error: %S", err.info().c_str());
			bRet = false;
		}
	}
	return bRet;
}

void CSIPWEB::SetTimer(int imsInterval)
{
	long int m_limsInterval;                    /**< Значение интервала таймера*/
	m_limsInterval = imsInterval;
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -10000 * (_int64)m_limsInterval;
	CancelWaitableTimer(m_hTimer);
	SetWaitableTimer(m_hTimer, &liDueTime, 0, NULL, NULL, false);
}

//******************************
void CSIPWEB::_PutMessage(unique_ptr<nsMsg::SMsg>& pMsg)
{
	__try
	{
		m_mxQueue.lock();
		m_queue.push(move(pMsg));
		m_mxQueue.unlock();
		SetEvent(m_hEvtQueue);
	}
	__except(ErrorFilter(GetExceptionInformation()), 1)
	{
		std::array<wchar_t, 1024> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"%s", static_cast<nsMsg::SLog*>(pMsg.get())->wstrInfo.c_str());
		m_Log._LogWrite(L"%s", chMsg.data());
		m_mxQueue.unlock();
	}
}

void CSIPWEB::_IncomingCall(const char* szCingPNum)
{
	string strCingPNum;
	if(!GetAlias(szCingPNum, strCingPNum)) strCingPNum = szCingPNum;
	vector<wchar_t> vNum;
	vNum.resize(strlen(szCingPNum) + 1);
	swprintf_s(vNum.data(), vNum.size(), L"%S", strCingPNum.c_str());
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SIncCall>();
	pMsg->iCode = nsMsg::enMsgType::enMT_IncomingCall;
	static_cast<nsMsg::SIncCall*>(pMsg.get())->wstrCallingPNum = vNum.data();
	_PutMessage(pMsg);
}

void CSIPWEB::_DisconnectRemote(int iCause, const char* szReason)
{
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SDisconnect>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Disconnect;
	vector<wchar_t> vReason; vReason.resize(strlen(szReason) + 1);
	swprintf_s(vReason.data(), vReason.size(), L"%S", szReason);
	static_cast<nsMsg::SDisconnect*>(pMsg.get())->wstrInfo = vReason.data();
	static_cast<nsMsg::SDisconnect*>(pMsg.get())->iCause = iCause;
	_PutMessage(pMsg);
}
void CSIPWEB::_Alerting()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Alerting;
	_PutMessage(pMsg);
}

void CSIPWEB::_Connected()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Answer;
	_PutMessage(pMsg);
}
void CSIPWEB::_RegisterOk()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_RegSet;
	_PutMessage(pMsg);
}
void CSIPWEB::_RegisterError(int iErr, const char* szErrInfo)
{
	vector<wchar_t> vNum;
	vNum.resize(strlen(szErrInfo) + 1);
	swprintf_s(vNum.data(), vNum.size(), L"%S", szErrInfo);
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SError>();
	pMsg->iCode = nsMsg::enMsgType::enMT_RegError;
	static_cast<nsMsg::SError*>(pMsg.get())->iErrorCode = iErr;
	static_cast<nsMsg::SError*>(pMsg.get())->wstrErrorInfo = vNum.data();
	_PutMessage(pMsg);
}
//*************************************
bool CSIPWEB::_MakeCall(const wchar_t* wszNumber)
{
	m_rawtimeCallBegin = time(nullptr);
	m_rawtimeCallAnswer = 0;

	std::vector<char> szNum;
	szNum.resize(wcslen(wszNumber) + 1);
	sprintf_s(szNum.data(), szNum.size(), "%ls", wszNumber);

	return _MakeCall(szNum.data());
}

bool CSIPWEB::_MakeCall(const char* szNumber)
{
	m_CallType = enListType::enLTOut;
	PlaySound(NULL, NULL, 0);
	string strNum(szNumber);
	if(!NumParser(szNumber, strNum)) return false;

	std::array<char, 256> szNum;

	auto strEnd = strNum.find(';');
	if(strEnd != std::string::npos)
	{
		m_strDTMF = strNum.substr(strEnd);
		strNum.erase(strEnd);
	}

	strEnd = strNum.find(',');
	if(strEnd != std::string::npos)
	{
		m_strDTMF = strNum.substr(strEnd);
		strNum.erase(strEnd);
	}

	m_wstrRemoteNumber.resize(strNum.length() + 1);
	swprintf_s(m_wstrRemoteNumber.data(), m_wstrRemoteNumber.size(), L"%S", strNum.c_str());

	sprintf_s(szNum.data(), szNum.size(), "sip:%s@%s", strNum.c_str(), m_strServerDomain.c_str());
	m_Log._LogWrite(L"   SIPWEB: Вызов на '%S'", szNum.data());

	SetTimer(1000);

	bool bCall_res = false;
	if(m_acc)
	{
		bCall_res = m_acc->_MakeCall(szNum.data());
		if(!bCall_res)
		{
			unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SError>();
			pMsg->iCode = nsMsg::enMsgType::enMT_Error;
			static_cast<nsMsg::SError*>(pMsg.get())->iErrorCode = 5;
			m_WebSocketServer->_PutMessage(pMsg);
			SetNullState();
		}
	}

	return bCall_res;
}

void CSIPWEB::_Disconnect()
{
	if(m_acc) m_acc->_Disconnect();
}

bool CSIPWEB::_Answer()
{
	m_rawtimeCallAnswer = time(nullptr);
	m_CallType = enListType::enLTIn;
	PlaySound(NULL, NULL, 0);

	m_State = nsMsg::CallState::stAnswer;
	return m_acc ? m_acc->_Answer() : false;
}

bool CSIPWEB::_DTMF(wchar_t wcDigit)
{
	array<char, 2> szDigit{ 0 };
	sprintf_s(szDigit.data(), szDigit.size(), "%lc", wcDigit);
	return m_acc ? m_acc->_DTMF(szDigit[0], bDTMF_2833) : false;
}
bool CSIPWEB::_DTMF(char cDigit)
{
	return m_acc ? m_acc->_DTMF(cDigit, bDTMF_2833) : false;
}


extern array<const wchar_t*, 3> wszRTName;

void CSIPWEB::PlayRingTone()
{
	if(!PlaySound(wszRTName[uiRingToneNumber >= wszRTName.size() ? 0 : uiRingToneNumber], NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP)) m_Log._LogWrite(L"   SIPWEB: RingTone play error=%d", errno);
}

void CSIPWEB::_Microfon(DWORD dwLevel)
{
	if(m_acc) m_acc->_Microfon(dwLevel);
}
void CSIPWEB::_Sound(DWORD dwLevel)
{
	if(m_acc) m_acc->_Sound(dwLevel);
}

void CSIPWEB::SetNullState()
{
	if(m_State == nsMsg::CallState::stNUll) return;

	m_strDTMF.clear();

	m_State = nsMsg::CallState::stNUll;
	if(m_CallType != enListType::enLTMiss) SetTimer(10000);
	m_CallType = enListType::enLTAll;
	m_wstrRemoteNumber.clear();
}
void CSIPWEB::_GetStatus()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Status;
	_PutMessage(pMsg);
}

bool CSIPWEB::GetAlias(const char* szBuf, string& strRes)
{
	bool bRet = false;
	std::cmatch reMatch;

	bRet = std::regex_search(szBuf, reMatch, m_reAlias, std::regex_constants::format_first_only);
	if(bRet) strRes = reMatch[1].str();
	return bRet;
}

bool CSIPWEB::GetAliasW(wstring& szBuf, wstring& strRes)
{
	bool bRet = false;
	std::wsmatch reMatch;

	bRet = std::regex_search(szBuf, reMatch, m_reAliasW, std::regex_constants::format_first_only);
	if(bRet) strRes = reMatch[1].str();
	return bRet;
}

void CSIPWEB::_GetStatusString(wstring& wstrStatus)
{
	wstrStatus = wstrLogin;
	if(!m_bReg)	wstrStatus += L" Ошибка регистрации! ERR=" + to_wstring(m_iRegErrorCode) + L"(" + m_wstrErrorInfo + L")";
	else		wstrStatus += L" Зарегистрирован";
}

size_t CSIPWEB::NumParser(const char* pszIn, string& strOut)
{
	strOut.clear();
	bool bInString = false;
	for(size_t i = 0; i < strlen(pszIn); i++)
	{
		if(pszIn[i] == '@') break;
		if(pszIn[i] == '\"' || pszIn[i] == '\'') bInString = !bInString;
		if(!bInString && (isdigit(pszIn[i]) || (pszIn[i] == ',') || (pszIn[i] == ';'))) strOut += pszIn[i];
	}

	return strOut.length();
}