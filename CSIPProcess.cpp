#include "CSIPProcess.h"
#include "resource.h"
#include <stdio.h>
#include <conio.h>
#include <wchar.h>
#include <datetimeapi.h>
#include <random>
#include <functional>
#include <chrono>
#include "Main.h"
#include "CWebSocket.h"
#include "History.h"
int iRandomType = 0;
std::random_device rd_Dev;

bool TestRandomGenerator()
{
	bool bRet = true;
	std::array<int32_t, 256> iValue;
	for(auto& cnt : iValue) cnt = iGetRnd();
	for(size_t pattern =0; pattern < iValue.size(); ++pattern)
		for(size_t current = pattern + 1; current < iValue.size(); ++current)
			if(iValue[pattern] == iValue[current]) bRet = false;
	if(!bRet)
	{  
		bRet = true;
		++iRandomType;
		for(auto& cnt : iValue) cnt = iGetRnd();
		for(size_t pattern = 0; pattern < iValue.size(); ++pattern)
			for(size_t current = pattern + 1; current < iValue.size(); ++current)
				if(iValue[pattern] == iValue[current]) bRet = false;
		if(!bRet)
		{
			bRet = true;
			++iRandomType;
			for(auto& cnt : iValue) cnt = iGetRnd();
			for(size_t pattern = 0; pattern < iValue.size(); ++pattern)
				for(size_t current = pattern + 1; current < iValue.size(); ++current)
					if(iValue[pattern] == iValue[current]) bRet = false;
			if(!bRet) ++iRandomType;
		}
	}
	return bRet;
}

int iGetRnd(const int iMin, const int iMax)
{
	int iRet = rand();
	switch(iRandomType)
	{
		case 0:
		{
			std::mt19937 gen(rd_Dev());
			std::uniform_int_distribution<> dis(iMin, iMax);
			iRet = dis(gen);
			break;
		}
		case 1:
		{
			std::default_random_engine gen(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
			std::uniform_int_distribution<> dis(iMin, iMax);
			iRet = dis(gen);
			break;
		}
		case 2:
		{
			int iInterval = iMax - iMin;
			int16_t shRnd[2];
			int32_t* iRndRes = (int32_t*)shRnd;
			for(int i = 0; i < 50; i++)
			{
				shRnd[0] = rand();
				shRnd[1] = rand();
				*iRndRes = iMin + *iRndRes % iInterval;
			}
			iRet = *iRndRes;
			break;
		}
	}
	return iRet;
}

CSIPProcess::CSIPProcess(HWND hParentWnd)
{
	m_hParentWnd = hParentWnd;
	m_hEvtStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvtQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	if(!TestRandomGenerator()) m_Log._LogWrite(L"   SIPP: Random generator = %d", iRandomType);
	m_reAlias.assign("sips?:(\\S*)@", std::regex_constants::icase);
	m_reAliasW.assign(L"sips?:(\\S*)@", std::regex_constants::icase);
}
CSIPProcess::~CSIPProcess()
{
	CloseHandle(m_hTimer);
	CloseHandle(m_hEvtQueue);
	CloseHandle(m_hEvtStop);
}

bool CSIPProcess::_Start(void)
{
	m_Log._LogWrite(L"   SIPP: _Start");
	if(DataInit())	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	else m_Log._LogWrite(L"   SIPP: Error at DataInit()");

	return m_hThread == NULL ? false : true;
}

bool CSIPProcess::_Stop(int time_out)
{
	m_Log._LogWrite(L"   SIPP: Stop");
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
void CSIPProcess::ErrorFilter(LPEXCEPTION_POINTERS ep)
{
	std::array<wchar_t, 1024> szBuf;
	swprintf_s(szBuf.data(), szBuf.size(), L"ПЕРЕХВАЧЕНА ОШИБКА ПО АДРЕСУ 0x%p", ep->ExceptionRecord->ExceptionAddress);
	m_Log._LogWrite(L"%s",szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"%s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	m_Log._LogWrite(L"%s", szBuf.data());
}

// Функция потока
unsigned __stdcall CSIPProcess::s_ThreadProc(LPVOID lpParameter)
{
	((CSIPProcess*)lpParameter)->LifeLoopOuter();
	return 0;
}

// Внешний цикл потока
void CSIPProcess::LifeLoopOuter()
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

DWORD CSIPProcess::GetTickCountDifference(DWORD nPrevTick)
{
	DWORD nTick = GetTickCount();
	if(nTick >= nPrevTick)	nTick -= nPrevTick;
	else					nTick += ULONG_MAX - nPrevTick;
	return nTick;
}

//Трассировка стека
template<std::size_t SIZE> wchar_t* CSIPProcess::StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr)
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

void CSIPProcess::LifeLoop(BOOL bRestart)
{
	HANDLE events[4];
	events[0] = m_hEvtStop;
	events[1] = m_hEvtQueue;
	events[2] = m_hTimer;

	if(bMinimizeOnStart) SendMessage(hDlgPhoneWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	m_Log._LogWrite(L"   SIPP: LifeLoop");
	SetTimer(100);

	SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"");

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
				m_Log._LogWrite(L"   SIPP: Поток останавливается...");
				try
				{
					if(m_acc)
					{
						m_acc->_Disconnect();
						m_acc->_DeleteCall();
						m_acc.release();
					}
					m_ep.release();
				}
				catch(Error& err)
				{
					m_Log._LogWrite(L"   SIPP: Shutdown error: %S", err.info());
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

bool CSIPProcess::Tick()
{
	bool bRet = false;

	switch(m_State)
	{
		case CallState::stAlerting:
		case CallState::stProgress:
			if(m_strDTMF.length() && m_strDTMF[0] == ',')
			{
				m_strDTMF.erase(0, 1);
				while(m_strDTMF.length())
				{
					if(m_strDTMF[0] != ',')
					{
						array<wchar_t,8> wszDigit;
						swprintf_s(wszDigit.data(), wszDigit.size(),L"%C", m_strDTMF[0]);
						SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)wszDigit[0]);
						m_strDTMF.erase(0, 1);
					}
					else break;
				}
			}
			SetTimer(1000);
			break;
		case CallState::stRinging:	break;
		case CallState::stAnswer:
		{
			if(m_strDTMF.length() && m_strDTMF[0] == ',')
			{
				m_strDTMF.erase(0, 1);
				while(m_strDTMF.length())
				{
					if(m_strDTMF[0] != ',')
					{
						array<wchar_t, 8> wszDigit;
						swprintf_s(wszDigit.data(), wszDigit.size(), L"%C", m_strDTMF[0]);
						SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)wszDigit[0]);
						m_strDTMF.erase(0, 1);
					}
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

void CSIPProcess::MessageSheduler()
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
				wstring wszCaption;
				wszCaption = L"Вызов от " + pinc->wstrCallingPNum;
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wszCaption.c_str());
				m_wstrNumA = pinc->wstrCallingPNum;

				SetForegroundWindow(m_hParentWnd);
				if(!bWebMode) BringWindowToTop(m_hParentWnd);
				if(!bWebMode) ShowWindow(m_hParentWnd, SW_SHOWNORMAL);

				SetDlgItemText(m_hParentWnd, IDC_BUTTON_DIAL, L"Ответить");
				SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отклонить");
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), true);

				if(!bWebMode)
				{
					FLASHWINFO fInfo{ sizeof(FLASHWINFO) };
					fInfo.hwnd = m_hParentWnd;
					fInfo.dwFlags = FLASHW_TRAY | FLASHW_CAPTION;
					fInfo.dwTimeout = 0;
					fInfo.uCount = 1000;
					FlashWindowEx(&fInfo);
				}
				m_State = CallState::stRinging;

				PlayRingTone();

				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Progress:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Устанавливается соединение");
				m_State = CallState::stProgress;
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Alerting:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Ожидание ответа");
				m_State = CallState::stAlerting;
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Answer:
			{
				PlaySound(NULL, NULL, 0);
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Разговор");
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_MUTE), true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_SILENCE), true);

				if(m_strDTMF.length() && m_strDTMF[0] == L';')
				{
					m_strDTMF.erase(0, 1);
					while(m_strDTMF.length())
					{
						if(m_strDTMF[0] != ',')
						{
							array<wchar_t, 8> wszDigit;
							swprintf_s(wszDigit.data(), wszDigit.size(), L"%C", m_strDTMF[0]);
							SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)wszDigit[0]);
							m_strDTMF.erase(0, 1);
						}
						else
						{
							SetTimer(1000);  
							break;
						}
					}
				}
				else SetTimer(600000);

				m_State = CallState::stAnswer;
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Decline:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов отклонён");
				Beep(425, 500);
				Sleep(500);
				Beep(425, 500);
				SetNullState();
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Cancel:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов отменён.");
				SetNullState();
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Disconnect:
			{
				auto Dismsg = static_cast<nsMsg::SDisconnect*>(pmsg.get());
				if(m_acc) m_acc->_DeleteCall();
				wstring wstrInfo;
				switch(m_State)
				{
					case CallState::stRinging:	
						wstrInfo = L"Пропущенный вызов от " + m_wstrNumA +L".";	
						if(m_History) m_History->_AddToCallList(m_wstrNumA.c_str(), m_rawtimeCallBegin, enListType::enLTMiss);
						break;
					case CallState::stAnswer:	wstrInfo = L"Вызов завершён.";	break;
					default:					wstrInfo = L"Вызов завершён." + to_wstring(Dismsg->iCause) + L":" + Dismsg->wstrInfo; break;
				}
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wstrInfo.c_str());
				SetNullState();
				Beep(425, 500);
				Sleep(500);
				Beep(425, 500);
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_Error:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				vector<wchar_t> wszCaption;
				wszCaption.resize(Errmsg->wstrErrorInfo.length() + 256);
				swprintf_s(wszCaption.data(), wszCaption.size(), L"Ошибка соединения.ERR=%d(%s)", Errmsg->iErrorCode, Errmsg->wstrErrorInfo.c_str());
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wszCaption.data());
				SetNullState();
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				break;
			}
			case nsMsg::enMsgType::enMT_RegSet:
			{
				array<wchar_t, 256> wszCaption;
				swprintf_s(wszCaption.data(), wszCaption.size(), L"%S Зарегистрирован", m_strLogin.c_str());
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), false);
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wszCaption.data());
				DefDlgProc(m_hParentWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);
				m_iRegErrorCode = 0;
				m_wstrErrorInfo.clear();
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
				m_bReg = true;
				break;
			}
			case nsMsg::enMsgType::enMT_RegError:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				vector<wchar_t> wszCaption;
				wszCaption.resize(Errmsg->wstrErrorInfo.length() + 256);
				//swprintf_s(wszCaption.data(), wszCaption.size(), L"%S Ошибка регистрации!", m_strLogin.c_str());
				//SetWindowText(m_hParentWnd, wszCaption.data());
				swprintf_s(wszCaption.data(), wszCaption.size(), L"%S Ошибка регистрации! ERR=%d(%s)", m_strLogin.c_str(), Errmsg->iErrorCode, Errmsg->wstrErrorInfo.c_str());
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wszCaption.data());

				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), false);

				FLASHWINFO fInfo{ sizeof(FLASHWINFO) };
				fInfo.hwnd = m_hParentWnd;
				fInfo.dwFlags = FLASHW_STOP;
				fInfo.dwTimeout = 0;
				fInfo.uCount = 0;
				FlashWindowEx(&fInfo);

				DefDlgProc(m_hParentWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_CONFIG, (LPARAM)0);
				m_bReg = false;
				m_iRegErrorCode = Errmsg->iErrorCode;
				m_wstrErrorInfo = Errmsg->wstrErrorInfo;
				if(m_WebSocketServer) m_WebSocketServer->_PutMessage(pmsg);
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
			case nsMsg::enMsgType::enMT_AccountModify:
			{
				_Modify();
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

bool CSIPProcess::DataInit()
{
	bool bRet = true;

	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), false);

	if(!strProductName.length()) m_strUserAgent = "NV CLOFF PHONE V.0.1";
	else m_strUserAgent = strProductName + " V." + strProductVersion;

	std::array<char, 256> szServer{ 0 };
	sprintf_s(szServer.data(), szServer.size(), "%ls", wstrServer.c_str());
	m_strServerDomain = szServer.data();
	if(iTransportType == IPPROTO_TCP) m_strServerDomain += ";transport=tcp";

	std::array<char, 256> szLogin{ 0 };
	sprintf_s(szLogin.data(), szLogin.size(), "%ls", wstrLogin.c_str());
	m_strLogin = szLogin.data();

	std::array<char, 256> szPassword{ 0 };
	sprintf_s(szPassword.data(), szPassword.size(), "%ls", wstrPassword.c_str());
	m_strPassword = szPassword.data();

	try
	{
		m_ep = make_unique<Endpoint>();
		m_ep->libCreate();
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPP: Startup error: %S",err.info().c_str());
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
		m_Log._LogWrite(L"   SIPP: Initialization error: %S", err.info().c_str());
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
		m_Log._LogWrite(L"   SIPP: Transport create Ok. ");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPP: Transport create error: %S", err.info().c_str());
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
		m_Log._LogWrite(L"   SIPP: EndPoint Set codec priority to PCMA");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPP: EndPoint Set codec priority error: %S", err.info().c_str());
		bRet = false;
	}
	// Start the library (worker threads etc)
	try
	{
		m_ep->libStart();
		m_Log._LogWrite(L"   SIPP: *** PJSUA2 STARTED ***");
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPP: EndPoint lib start error: %S", err.info().c_str());
		bRet = false;
	}

	if(m_strLogin.length())
	{
		try
		{
			m_acc = make_unique<MyAccount>();
			if(m_acc->_Create(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) m_Log._LogWrite(L"   SIPP: Account create ok");
		}
		catch(Error& err)
		{
			m_Log._LogWrite(L"   SIPP: EndPoint lib start error: %S", err.info().c_str());
			bRet = false;
		}
	}
	return bRet;
}

void CSIPProcess::SetTimer(int imsInterval)
{
	long int m_limsInterval;                    /**< Значение интервала таймера*/
	m_limsInterval = imsInterval;
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -10000 * (_int64)m_limsInterval;
	CancelWaitableTimer(m_hTimer);
	SetWaitableTimer(m_hTimer, &liDueTime, 0, NULL, NULL, false);
}

//******************************
void CSIPProcess::_PutMessage(unique_ptr<nsMsg::SMsg>& pMsg)
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
		m_Log._LogWrite(L"%s",chMsg.data());
		m_mxQueue.unlock();
	}
}

void CSIPProcess::_IncomingCall(const char* szCingPNum)
{
	time(&m_rawtimeCallBegin);
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

void CSIPProcess::_DisconnectRemote(int iCause, const char* szReason)
{
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SDisconnect>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Disconnect;
	vector<wchar_t> vReason; vReason.resize(strlen(szReason) + 1);
	swprintf_s(vReason.data(), vReason.size(),L"%S",szReason);
	static_cast<nsMsg::SDisconnect*>(pMsg.get())->wstrInfo = vReason.data();
	static_cast<nsMsg::SDisconnect*>(pMsg.get())->iCause = iCause;
	_PutMessage(pMsg);
}
void CSIPProcess::_Alerting()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Alerting;
	_PutMessage(pMsg);
}

void CSIPProcess::_Connected()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Answer;
	_PutMessage(pMsg);
}
void CSIPProcess::_RegisterOk()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_RegSet;
	_PutMessage(pMsg);
}
void CSIPProcess::_RegisterError(int iErr, const char* szErrInfo)
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
bool CSIPProcess::_MakeCall(const wchar_t* wszNumber)
{
	time(&m_rawtimeCallBegin);
	if(m_History) m_History->_AddToCallList(wszNumber, m_rawtimeCallBegin, enListType::enLTOut);

	std::vector<char> szNum;
	szNum.resize(wcslen(wszNumber) + 1);
	sprintf_s(szNum.data(), szNum.size(), "%ls", wszNumber);

	return _MakeCall(szNum.data());
}

bool CSIPProcess::_MakeCall(const char* szNumber)
{
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

	sprintf_s(szNum.data(), szNum.size(), "sip:%s@%s", strNum.c_str(), m_strServerDomain.c_str());
	m_Log._LogWrite(L"   SIPP: Вызов на '%S'", szNum.data());
	SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отбой");
	SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов...");

	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), true);

	DefDlgProc(m_hParentWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DISCONNECT, (LPARAM)0);

	SetTimer(1000);

	return m_acc ? m_acc->_MakeCall(szNum.data()) : false;
}

void CSIPProcess::_Disconnect()
{
	if(m_acc) m_acc->_Disconnect();
	SetNullState();
}

bool CSIPProcess::_Answer()
{
	if(m_History) m_History->_AddToCallList(m_wstrNumA.c_str(), m_rawtimeCallBegin, enListType::enLTIn);
	PlaySound(NULL, NULL, 0);
	SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отбой");
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_MUTE), true);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_SILENCE), true);
	ShowWindow(GetDlgItem(m_hParentWnd, IDC_STATIC_AN1), SW_HIDE);

	FLASHWINFO fInfo{ sizeof(FLASHWINFO) };
	fInfo.hwnd = m_hParentWnd;
	fInfo.dwFlags = FLASHW_STOP;
	fInfo.dwTimeout = 0;
	fInfo.uCount = 0;
	FlashWindowEx(&fInfo);

	m_State = CallState::stAnswer;
	return m_acc ? m_acc->_Answer(): false;
}

bool CSIPProcess::_DTMF(wchar_t wcDigit)
{
	array<char, 2> szDigit{ 0 };
	sprintf_s(szDigit.data(), szDigit.size(), "%lc", wcDigit);
	return m_acc ? m_acc->_DTMF(szDigit[0], bDTMF_2833) : false;
}
bool CSIPProcess::_DTMF(char cDigit)
{
	return m_acc ? m_acc->_DTMF(cDigit, bDTMF_2833) : false;
}


array<const wchar_t*,3> wszRTName{ L"RingToneName1" , L"RingToneName2", L"RingToneName3" };

void CSIPProcess::PlayRingTone()
{
//	m_Log._LogWrite(L"   SIPP: PlayRingTone = %d", uiRingToneNumber);
	if(!PlaySound(wszRTName[uiRingToneNumber >= wszRTName.size() ? 0 : uiRingToneNumber], NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP)) m_Log._LogWrite(L"   SIPP: RingTone play error=%d",errno);
}

void CSIPProcess::_Modify()
{
	auto epInfo = m_ep->transportGetInfo(0);
	if((epInfo.type == PJSIP_TRANSPORT_TCP && iTransportType == IPPROTO_UDP) || (epInfo.type == PJSIP_TRANSPORT_UDP && iTransportType == IPPROTO_TCP))
	{
		// Create SIP transport. Error handling sample is shown
		m_ep->transportClose(0);
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
			m_Log._LogWrite(L"   SIPP: Transport create Ok. ");
		}
		catch(Error& err)
		{
			m_Log._LogWrite(L"   SIPP: Transport create error: %S", err.info().c_str());
		}
	}
	try
	{
		bool bMod = false;
		std::array<char, 256> szServer{ 0 };
		if(iTransportType == IPPROTO_TCP) sprintf_s(szServer.data(), szServer.size(), "%ls;transport=tcp", wstrServer.c_str());
		else sprintf_s(szServer.data(), szServer.size(), "%ls", wstrServer.c_str());
		if(m_strServerDomain.compare(szServer.data()) != 0) bMod = true;
		m_strServerDomain = szServer.data();

		std::array<char, 256> szLogin{ 0 };
		sprintf_s(szLogin.data(), szLogin.size(), "%ls", wstrLogin.c_str());
		if(m_strLogin.compare(szLogin.data()) != 0) bMod = true;
		m_strLogin = szLogin.data();

		std::array<char, 256> szPassword{ 0 };
		sprintf_s(szPassword.data(), szPassword.size(), "%ls", wstrPassword.c_str());
		if(m_strPassword.compare(szPassword.data()) != 0) bMod = true;
		m_strPassword = szPassword.data();

		if(m_acc) if(bMod) if(m_acc->_Modify(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) m_Log._LogWrite(L"   SIPP: Account create ok");
		else
		{
			if(m_strLogin.length())
			{
				try
				{
					m_acc = make_unique<MyAccount>();
					if(m_acc->_Create(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) m_Log._LogWrite(L"   SIPP: Account create ok");
				}
				catch(Error& err)
				{
					m_Log._LogWrite(L"   SIPP: EndPoint lib start error: %S", err.info().c_str());
				}
			}
		}
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   SIPP: Account modify error: %S", err.info().c_str());
	}

}
void CSIPProcess::_Microfon(DWORD dwLevel)
{
	if(m_acc) m_acc->_Microfon(dwLevel);
}
void CSIPProcess::_Sound(DWORD dwLevel)
{
	if(m_acc) m_acc->_Sound(dwLevel);
}

void CSIPProcess::SetNullState()
{
	SendMessage(m_hParentWnd,WM_USER_REGISTER_DS,(WPARAM)0,(LPARAM)0);
	m_strDTMF.clear();

	m_State = CallState::stNUll;
	SetTimer(60000);
}
void CSIPProcess::_GetStatus()
{
	auto pMsg = make_unique<nsMsg::SMsg>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Status;
	_PutMessage(pMsg);
}

bool CSIPProcess::GetAlias(const char* szBuf, string& strRes)
{
	bool bRet = false;
	std::cmatch reMatch;

	bRet = std::regex_search(szBuf, reMatch, m_reAlias, std::regex_constants::format_first_only);
	if(bRet) strRes = reMatch[1].str();
	return bRet;
}

bool CSIPProcess::GetAliasW(wstring& szBuf, wstring& strRes)
{
	bool bRet = false;
	std::wsmatch reMatch;

	bRet = std::regex_search(szBuf, reMatch, m_reAliasW, std::regex_constants::format_first_only);
	if(bRet) strRes = reMatch[1].str();
	return bRet;
}

void CSIPProcess::_GetStatusString(wstring& wstrStatus)
{
	wstrStatus = wstrLogin;
	if(!m_bReg)	wstrStatus += L" Ошибка регистрации! ERR=" + to_wstring(m_iRegErrorCode) + L"(" + m_wstrErrorInfo +L")";
	else		wstrStatus += L" Зарегистрирован";
}

size_t CSIPProcess::NumParser(const char* pszIn, string& strOut)
{
	strOut.clear();
	bool bInString = false;
	for(size_t i = 0; i < strlen(pszIn); i++)
	{
		if(pszIn[i] == '\"' || pszIn[i] == '\'') bInString = !bInString;
		if(!bInString && (isdigit(pszIn[i]) || (pszIn[i] == ',') || (pszIn[i] == ';'))) strOut += pszIn[i];
	}

	return strOut.length();
}