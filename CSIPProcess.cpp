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
	m_hLog = CreateFile(L"cloff_sip_phone.log", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(m_hLog == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		std::array<wchar_t, 256> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"   Main: Log file open error %u(%0x)", dw, dw);
		AddToMessageLog(LPTSTR(chMsg.data()));
	}

	if(!TestRandomGenerator()) LogWrite(L"   Main: Random generator = %d", iRandomType);
}
CSIPProcess::~CSIPProcess()
{
	CloseHandle(m_hTimer);
	CloseHandle(m_hEvtQueue);
	CloseHandle(m_hEvtStop);
	if(m_hLog != INVALID_HANDLE_VALUE) CloseHandle(m_hLog);
}

bool CSIPProcess::_Start(void)
{
	LogWrite(L"   Main: Start");
	// Declare and initialize variables.
	if(DataInit())	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	else LogWrite(L"   Main: Error at DataInit()");

	return m_hThread == NULL ? false : true;
}

bool CSIPProcess::_Stop(int time_out)
{
	LogWrite(L"   Main: Stop");
	if(m_hThread != NULL)
	{
		SetEvent(m_hEvtStop);
		if(WaitForSingleObject(m_hThread, time_out) == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread, 0);
		}
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
	AddToMessageLog(szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"%s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	AddToMessageLog(szBuf.data());
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
			AddToMessageLog(chMsg.data());
			if(nCriticalErrorCounter == 0)
			{
				nTick = GetTickCount();
			}
			nCriticalErrorCounter++;
			if(nCriticalErrorCounter > 4)
			{
				nCriticalErrorCounter = 0;
				nTick = GetTickCountDifference(nTick);
				if(nTick < 60000)
				{
					AddToMessageLog(LPTSTR(L"5 СБОЕВ НА ПОТОКЕ В ТЕЧЕНИИ МИНУТЫ !!!!  ПОТОК ОСТАНОВЛЕН"));
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
	if(nTick >= nPrevTick) nTick -= nPrevTick;
	else				nTick += ULONG_MAX - nPrevTick;
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
	if(FrameAddr)
	{
		swprintf_s(&buffer[pos], buffer.size() - pos, L"...");
	}
	return buffer.data();
}

void CSIPProcess::LifeLoop(BOOL bRestart)
{
	HANDLE events[4];
	events[0] = m_hEvtStop;
	events[1] = m_hEvtQueue;
	events[2] = m_hTimer;

	LogWrite(L"   Main: LifeLoop");
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
				LogWrite(L"   Main: Поток останавливается...");
				try
				{
					if(m_acc)
					{
						m_acc->_Disconnect();
						m_acc->_DeleteCall();
						m_acc.release();
					}
					if(m_ep) 
					{
						m_ep.release();
					}
				}
				catch(Error& err)
				{
					LogWrite(L"   Main: Shutdown error: %S", err.info());
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
			if(m_wstrDTMF.length() && m_wstrDTMF[0] == L',')
			{
				m_wstrDTMF.erase(0, 1);
				while(m_wstrDTMF.length())
				{
					if(m_wstrDTMF[0] != ',')
					{
						SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)m_wstrDTMF[0]);
						m_wstrDTMF.erase(0, 1);
					}
					else break;
				}
			}
			SendMessage(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), PBM_STEPIT, 0, 0);
			SetTimer(1000);
			break;
		case CallState::stRinging:
			m_bTicker = !m_bTicker;
			SetTimer(1000);
			break;
		case CallState::stAnswer:
		{
			if(m_wstrDTMF.length() && m_wstrDTMF[0] == L',')
			{
				m_wstrDTMF.erase(0, 1);
				while(m_wstrDTMF.length())
				{
					if(m_wstrDTMF[0] != ',')
					{
						SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)m_wstrDTMF[0]);
						m_wstrDTMF.erase(0, 1);
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

				SetForegroundWindow(m_hParentWnd);
				BringWindowToTop(m_hParentWnd);
				ShowWindow(m_hParentWnd, SW_SHOWNORMAL);

				SetDlgItemText(m_hParentWnd, IDC_BUTTON_DIAL, L"Ответить");
				SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отклонить");
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL),true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT),true);

				FLASHWINFO fInfo{sizeof(FLASHWINFO)};
				fInfo.hwnd = m_hParentWnd;
				fInfo.dwFlags = FLASHW_TRAY | FLASHW_CAPTION;
				fInfo.dwTimeout = 0;
				fInfo.uCount = 1000;
				FlashWindowEx(&fInfo);

				m_bTicker = true;
				SetTimer(100);
				m_State = CallState::stRinging;

				PlayRingTone();
				break;
			}
			case nsMsg::enMsgType::enMT_Progress:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Устанавливается соединение");
				m_State = CallState::stProgress;
				break;
			}
			case nsMsg::enMsgType::enMT_Alerting:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Ожидание ответа");
				m_State = CallState::stAlerting;
				break;
			}
			case nsMsg::enMsgType::enMT_Answer:
			{
				PlaySound(NULL, NULL, 0);
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Разговор");
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_MUTE), true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_SILENCE), true);
				ShowWindow(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), SW_HIDE);

				if(m_wstrDTMF.length() && m_wstrDTMF[0] == L';')
				{
					m_wstrDTMF.erase(0, 1);
					while(m_wstrDTMF.length())
					{
						if(m_wstrDTMF[0] != ',')
						{
							SendMessage(m_hParentWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)m_wstrDTMF[0]);
							m_wstrDTMF.erase(0, 1);
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
				break;
			}
			case nsMsg::enMsgType::enMT_Decline:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов отклонён");
				ShowWindow(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), SW_HIDE);
				//Beep(425, 500);
				//Sleep(500);
				//Beep(425, 500);
				SetNullState();
				break;
			}
			case nsMsg::enMsgType::enMT_Cancel:
			{
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов отменён.");
				SetNullState();
				break;
			}
			case nsMsg::enMsgType::enMT_Disconnect:
			{
				auto Dismsg = static_cast<nsMsg::SDisconnect*>(pmsg.get());
				if(m_acc) m_acc->_DeleteCall();
				wstring wstrInfo;
				wstrInfo = L"Вызов завершён." + Dismsg->wstrInfo;
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, wstrInfo.c_str());
				//Beep(425, 500);
				//Sleep(500);
				//Beep(425, 500);
				SetNullState();
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
				break;
			}
			case nsMsg::enMsgType::enMT_RegSet:
			{
				array<wchar_t, 256> wszCaption;
				swprintf_s(wszCaption.data(), wszCaption.size(), L"%S Зарегистрирован", m_strLogin.c_str());
				SetWindowText(m_hParentWnd, wszCaption.data());
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), true);
				EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), false);
				SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"");
				DefDlgProc(m_hParentWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DIAL, (LPARAM)0);
				break;
			}
			case nsMsg::enMsgType::enMT_RegError:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				vector<wchar_t> wszCaption;
				wszCaption.resize(Errmsg->wstrErrorInfo.length() + 256);
				swprintf_s(wszCaption.data(), wszCaption.size(), L"%S Ошибка регистрации!", m_strLogin.c_str());
				SetWindowText(m_hParentWnd, wszCaption.data());
				swprintf_s(wszCaption.data(), wszCaption.size(), L"ERR=%d(%s)", Errmsg->iErrorCode, Errmsg->wstrErrorInfo.c_str());
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
				break;
			}
			case nsMsg::enMsgType::enMT_Log:
			{
				auto plog = static_cast<nsMsg::SLog*>(pmsg.get());
				LogWrite(L"%s",plog->wstrInfo.c_str());
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

	if(!GetProductAndVersion(m_strUserAgent)) m_strUserAgent = "NV CLOFF PHONE V.0.1";

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
		LogWrite(L"   Main: Startup error: %S",err.info().c_str());
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
		LogWrite(L"   Main: Initialization error: %S", err.info().c_str());
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
		LogWrite(L"   Main: Transport create Ok. ");
	}
	catch(Error& err)
	{
		LogWrite(L"   Main: Transport create error: %S", err.info().c_str());
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
		LogWrite(L"   Main: EndPoint Set codec priority to PCMA");
	}
	catch(Error& err)
	{
		LogWrite(L"   Main: EndPoint Set codec priority error: %S", err.info().c_str());
		bRet = false;
	}
	// Start the library (worker threads etc)
	try
	{
		m_ep->libStart();
		LogWrite(L"   Main: *** PJSUA2 STARTED ***");
	}
	catch(Error& err)
	{
		LogWrite(L"   Main: EndPoint lib start error: %S", err.info().c_str());
		bRet = false;
	}

	if(m_strLogin.length())
	{
		try
		{
			m_acc = make_unique<MyAccount>();
			if(m_acc->_Create(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) LogWrite(L"   Main: Account create ok");
		}
		catch(Error& err)
		{
			LogWrite(L"   Main: EndPoint lib start error: %S", err.info().c_str());
			bRet = false;
		}
	}
	return bRet;
}

bool CSIPProcess::GetProductAndVersion(std::string& strProductVersion)
{
	// get the filename of the executable containing the version resource
	TCHAR moduleName[MAX_PATH + 1];
	GetModuleFileName(NULL, moduleName, MAX_PATH);
	DWORD dummyZero;
	DWORD versionSize = GetFileVersionInfoSize(moduleName, &dummyZero);
	if(versionSize == 0) return false;

	std::vector<BYTE> data; data.resize(versionSize);
	if(GetFileVersionInfo(moduleName, NULL, versionSize, data.data()))
	{
		UINT length;

		struct LANGANDCODEPAGE
		{
			WORD wLanguage;
			WORD wCodePage;
		} *lpTranslate;

		// Read the list of languages and code pages.

		VerQueryValue(data.data(), L"\\VarFileInfo\\Translation",(LPVOID*)&lpTranslate, &length);

// Read the file description for each language and code page.

		std::array<wchar_t, 256> SubBlock;
		LPVOID lpBuffer;
		swprintf_s(SubBlock.data(), SubBlock.size(),L"\\StringFileInfo\\%04x%04x\\ProductName",lpTranslate[0].wLanguage,	lpTranslate[0].wCodePage);
		UINT pnlength;
		if(VerQueryValue(data.data(), SubBlock.data(), (LPVOID*)&lpBuffer, &pnlength))
		{
			std::array<char, 256> szName;
			sprintf_s(szName.data(), szName.size(), "%ls", static_cast<wchar_t*>(lpBuffer));
			strProductVersion = szName.data();
		}
		else strProductVersion = "CLOFF SIP Phone by Omicron";

		VS_FIXEDFILEINFO* pFixInfo;
		VerQueryValue(data.data(), TEXT("\\"), (LPVOID*)&pFixInfo, &length);
		strProductVersion += " " + std::to_string(HIWORD(pFixInfo->dwProductVersionMS)) + "." + 
			std::to_string(LOWORD(pFixInfo->dwProductVersionMS)) + "." +
			std::to_string(HIWORD(pFixInfo->dwProductVersionLS)) +"." +
			std::to_string(LOWORD(pFixInfo->dwProductVersionLS));
	}

	return true;
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

void CSIPProcess::LogWrite(const wchar_t* msg, ...)
{
	std::array<wchar_t, 2048> szBuffer;
	__try
//	try
	{
		std::array<wchar_t, 32> szTime;
		auto res = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, L"HH:mm:ss", szTime.data(), szTime.size());
		std::array<wchar_t, 32> szDate;
		res = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, NULL, L"dd.MM.yyyy", szDate.data(), szDate.size(), NULL);
		swprintf_s(szBuffer.data(), szBuffer.size(), L"%s-%s: ", szTime.data(), szDate.data());

		va_list vargs;
		va_start(vargs, msg);
		_vsnwprintf_s(&szBuffer[wcslen(szBuffer.data())], szBuffer.size() - wcslen(szBuffer.data()), szBuffer.size() - wcslen(szBuffer.data()), msg, vargs);
		va_end(vargs);
		if(szBuffer[wcslen(szBuffer.data()) - 1] == L'\n') szBuffer[wcslen(szBuffer.data()) - 1] = 0;
		swprintf_s(&szBuffer[wcslen(szBuffer.data())], szBuffer.size() - wcslen(szBuffer.data()), L"\r\n");

		if(m_hLog == INVALID_HANDLE_VALUE) AddToMessageLog(LPTSTR(msg), FALSE, EVENTLOG_INFORMATION_TYPE, 0);
		else
		{
			DWORD dwBytesWritten = 0;
			if(!WriteFile(m_hLog, (LPCVOID)szBuffer.data(), wcslen(szBuffer.data()) * sizeof(wchar_t), &dwBytesWritten, NULL))
			{
				DWORD dw = GetLastError();
				std::array<wchar_t, 1024> chMsg;
				swprintf_s(chMsg.data(), chMsg.size(), L"Log file write error %u(%0x). Msg:%s", dw, dw, msg);
				AddToMessageLog(LPTSTR(chMsg.data()));
			}
		}
		auto fileLen = GetFileSize(m_hLog,NULL);
		if(fileLen == INVALID_FILE_SIZE)
		{
			DWORD dw = GetLastError();
			std::array<wchar_t, 1024> chMsg;
			swprintf_s(chMsg.data(), chMsg.size(), L"Log file get file size error %u(%0x). Msg:%s", dw, dw, msg);
			AddToMessageLog(LPTSTR(chMsg.data()));
		}
		else if(fileLen >= 5242880)
		{
			CloseHandle(m_hLog);
			DeleteFile(L"cloff_sip_phone_old.log");
			if(!MoveFile(L"cloff_sip_phone.log", L"cloff_sip_phone_old.log"))
			{
				DWORD dw = GetLastError();
				std::array<wchar_t, 1024> chMsg;
				swprintf_s(chMsg.data(), chMsg.size(), L"Log file move error %u(%0x). Msg:%s", dw, dw, msg);
				AddToMessageLog(LPTSTR(chMsg.data()));
			}
			m_hLog = CreateFile(L"cloff_sip_phone.log", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(m_hLog == INVALID_HANDLE_VALUE)
			{
				DWORD dw = GetLastError();
				std::array<wchar_t, 256> chMsg;
				swprintf_s(chMsg.data(), chMsg.size(), L"   Main: Log file open error %u(%0x)", dw, dw);
				AddToMessageLog(LPTSTR(chMsg.data()));
			}
		}
	}
	__except(ErrorFilter(GetExceptionInformation()), 1)
	{
		std::array<wchar_t, 1024> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"%S", "Last message buffer is empty!");
		AddToMessageLog(chMsg.data());
	}
//catch(...)
	//{
	//	DWORD dwBytesWritten = 0;
	//	std::array<wchar_t, 1024> chMsg;
	//	swprintf_s(chMsg.data(), chMsg.size(), L"Log file msg format error . Msg:%s", msg);
	//	if(!WriteFile(m_hLog, (LPCVOID)chMsg.data(), wcslen(chMsg.data()) * sizeof(wchar_t), &dwBytesWritten, NULL)) AddToMessageLog(LPTSTR(chMsg.data()));
	//}
}

void CSIPProcess::AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem, WORD wType, WORD wCategory)
{
	std::array<wchar_t, 1024> szMsg;
	HANDLE hEventSource;
	LPCWSTR lpszStrings[2];
	DWORD dwErr = 0;
	dwErr = GetLastError();
	hEventSource = RegisterEventSource(NULL, TEXT("CLOFFSIPPHONE"));
	swprintf_s(szMsg.data(), szMsg.size(), TEXT("%s error: %d"), TEXT("CLOFFSIPPHONE"), dwErr);
	lpszStrings[0] = (LPTSTR)szMsg.data();
	lpszStrings[1] = lpszMsg;
	if(hEventSource != NULL)
	{
		if(bSystem)
		{
			ReportEvent(hEventSource, // handle of event source
				EVENTLOG_ERROR_TYPE,  // event type
				0,                    // event category
				0,                    // event ID
				NULL,                 // current user's SID
				2,                    // strings in lpszStrings
				0,                    // no bytes of raw data
				lpszStrings,          // std::array of error strings
				NULL);                // no raw data
		}
		else
		{
			ReportEvent(hEventSource, // handle of event source
				wType,				  // event type
				wCategory,            // event category
				0,                    // event ID
				NULL,                 // current user's SID
				1,                    // strings in lpszStrings
				0,                    // no bytes of raw data
				(LPCWSTR*)&lpszMsg, // std::array of error strings
				NULL);                // no raw data
		}
		(VOID)DeregisterEventSource(hEventSource);
	}
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
		AddToMessageLog(chMsg.data());
		m_mxQueue.unlock();
	}
}

void CSIPProcess::_LogWrite(const wchar_t* msg, ...)
{
	std::array<wchar_t, 2048> szBuffer;
	try
	{
		va_list vargs;
		va_start(vargs, msg);
		_vsnwprintf_s(szBuffer.data(), szBuffer.size(), szBuffer.size(), msg, vargs);
		va_end(vargs);
		if(szBuffer[wcslen(szBuffer.data()) - 1] == L'\n') szBuffer[wcslen(szBuffer.data()) - 1] = 0;
		unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SLog>();
		pMsg->iCode = nsMsg::enMsgType::enMT_Log;
		static_cast<nsMsg::SLog*>(pMsg.get())->wstrInfo = szBuffer.data();
		_PutMessage(pMsg);
	}
	catch(...)
	{
		DWORD dwBytesWritten = 0;
		std::array<wchar_t, 1024> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"Log file msg format error . Msg:%s", msg);
		if(!WriteFile(m_hLog, (LPCVOID)chMsg.data(), wcslen(chMsg.data()) * sizeof(wchar_t), &dwBytesWritten, NULL)) AddToMessageLog(LPTSTR(chMsg.data()));
	}
}

void CSIPProcess::_IncomingCall(const char* szCingPNum)
{
	vector<wchar_t> vNum;
	vNum.resize(strlen(szCingPNum) + 1);
	swprintf_s(vNum.data(), vNum.size(), L"%S", szCingPNum);
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SIncCall>();
	pMsg->iCode = nsMsg::enMsgType::enMT_IncomingCall;
	static_cast<nsMsg::SIncCall*>(pMsg.get())->wstrCallingPNum = vNum.data();
	_PutMessage(pMsg);
}

void CSIPProcess::_DisconnectRemote(const char* szReason)
{
	unique_ptr<nsMsg::SMsg> pMsg = make_unique<nsMsg::SDisconnect>();
	pMsg->iCode = nsMsg::enMsgType::enMT_Disconnect;
	vector<wchar_t> vReason; vReason.resize(strlen(szReason) + 1);
	swprintf_s(vReason.data(), vReason.size(),L"%S",szReason);
	static_cast<nsMsg::SDisconnect*>(pMsg.get())->wstrInfo = vReason.data();
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
	PlaySound(NULL, NULL, 0);
	wstring wstrNum(wszNumber);
	std::array<char, 256> szNum;

	auto strBegin = wstrNum.find_first_not_of(' ');
	if(strBegin == std::string::npos) return false; // no content

	auto strEnd = wstrNum.find_last_not_of(' ');

	if(strBegin) wstrNum.erase(0, strBegin);
	if((strEnd + 1) < wstrNum.length()) wstrNum.erase(strEnd + 1);

	strEnd = wstrNum.find(';');
	if(strEnd != std::string::npos)
	{
		m_wstrDTMF = wstrNum.substr(strEnd);
		wstrNum.erase(strEnd);
	}

	strEnd = wstrNum.find(',');
	if(strEnd != std::string::npos)
	{
		m_wstrDTMF = wstrNum.substr(strEnd);
		wstrNum.erase(strEnd);
	}

	sprintf_s(szNum.data(), szNum.size(), "sip:%ls@%s", wstrNum.c_str(),m_strServerDomain.c_str());
	CSIPProcess::LogWrite(L"   Main: Вызов на '%S'", szNum.data());
	SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отбой");
	SetDlgItemText(m_hParentWnd, IDC_STATIC_REGSTATUS, L"Вызов...");

	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DISCONNECT), true);

	ShowWindow(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), SW_SHOWNORMAL);
	DefDlgProc(m_hParentWnd, (UINT)DM_SETDEFID, (WPARAM)IDC_BUTTON_DISCONNECT, (LPARAM)0);
	SendMessage(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), PBM_SETPOS, 0, 0);

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
	PlaySound(NULL, NULL, 0);
	SetDlgItemText(m_hParentWnd, IDC_BUTTON_DISCONNECT, L"Отбой");
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_DIAL), false);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_MUTE), true);
	EnableWindow(GetDlgItem(m_hParentWnd, IDC_BUTTON_SILENCE), true);
	ShowWindow(GetDlgItem(m_hParentWnd, IDC_PROGRESS_CALL), SW_HIDE);
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
	array<char, 2> szDigit{0};
	sprintf_s(szDigit.data(), szDigit.size(),"%lc",wcDigit);
	return m_acc ? m_acc->_DTMF(szDigit[0], bDTMF_2833) : false;
}


array<const wchar_t*,3> wszRTName{ L"RingToneName1" , L"RingToneName2", L"RingToneName3" };

void CSIPProcess::PlayRingTone()
{
	if(!PlaySound(wszRTName[uiRingToneNumber >= wszRTName.size() ? 0 : uiRingToneNumber], NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP)) LogWrite(L"   Main: RingTone play error=%d",errno);
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
			LogWrite(L"   Main: Transport create Ok. ");
		}
		catch(Error& err)
		{
			LogWrite(L"   Main: Transport create error: %S", err.info().c_str());
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

		if(m_acc) if(bMod) if(m_acc->_Modify(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) LogWrite(L"   Main: Account create ok");
		else
		{
			if(m_strLogin.length())
			{
				try
				{
					m_acc = make_unique<MyAccount>();
					if(m_acc->_Create(m_strLogin.c_str(), m_strPassword.c_str(), m_strServerDomain.c_str())) LogWrite(L"   Main: Account create ok");
				}
				catch(Error& err)
				{
					LogWrite(L"   Main: EndPoint lib start error: %S", err.info().c_str());
				}
			}
		}
	}
	catch(Error& err)
	{
		LogWrite(L"   Main: Account modify error: %S", err.info().c_str());
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
	m_wstrDTMF.clear();

	m_State = CallState::stNUll;
	SetTimer(60000);
}