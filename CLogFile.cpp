#include "CLogFile.h"

CLogFile::CLogFile()
{
	m_hEvtStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvtQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hLog = CreateFile(L"cloff_sip_phone.log", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(m_hLog == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		std::array<wchar_t, 256> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"   Main: Log file open error %u(%0x)", dw, dw);
		AddToMessageLog(LPTSTR(chMsg.data()));
	}
	else SetFilePointer(m_hLog, 0, 0, FILE_END);

}

CLogFile::~CLogFile()
{
	CloseHandle(m_hEvtQueue);
	CloseHandle(m_hEvtStop);
	if(m_hLog != INVALID_HANDLE_VALUE) CloseHandle(m_hLog);
}

bool CLogFile::_Start(void)
{
	if(m_hLog != INVALID_HANDLE_VALUE)	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	return m_hThread == NULL ? false : true;
}

bool CLogFile::_Stop(int time_out)
{
	if(m_hThread != NULL)
	{
		SetEvent(m_hEvtStop);
		if(WaitForSingleObject(m_hThread, time_out) == WAIT_TIMEOUT) TerminateThread(m_hThread, 0);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return true;
}
// Ôóíêöèÿ âûâîäà ñîîáùåíèÿ î ñáîå
void CLogFile::ErrorFilter(LPEXCEPTION_POINTERS ep)
{
	std::array<wchar_t, 1024> szBuf;
	swprintf_s(szBuf.data(), szBuf.size(), L"	WS: ÏÅÐÅÕÂÀ×ÅÍÀ ÎØÈÁÊÀ ÏÎ ÀÄÐÅÑÓ 0x%p", ep->ExceptionRecord->ExceptionAddress);
	AddToMessageLog(szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"	WS: %s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	AddToMessageLog(szBuf.data());
}

// Ôóíêöèÿ ïîòîêà
unsigned __stdcall CLogFile::s_ThreadProc(LPVOID lpParameter)
{
	((CLogFile*)lpParameter)->LifeLoopOuter();
	return 0;
}

// Âíåøíèé öèêë ïîòîêà
void CLogFile::LifeLoopOuter()
{
	int nCriticalErrorCounter = 0;
	int iExitCode = 0;
	unsigned long nTick;
	BOOL repeat = TRUE;
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
			if(nCriticalErrorCounter == 0) nTick = GetTickCount();
			nCriticalErrorCounter++;
			if(nCriticalErrorCounter > 4)
			{
				nCriticalErrorCounter = 0;
				nTick = GetTickCountDifference(nTick);
				if(nTick < 60000)
				{
					std::array<wchar_t, 1024> szBuf;
					swprintf_s(szBuf.data(), szBuf.size(), L"	WS: 5 ÑÁÎÅÂ ÍÀ ÏÎÒÎÊÅ Â ÒÅ×ÅÍÈÈ ÌÈÍÓÒÛ !!!!  ÏÎÒÎÊ ÎÑÒÀÍÎÂËÅÍ");
					AddToMessageLog(szBuf.data());
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

DWORD CLogFile::GetTickCountDifference(DWORD nPrevTick)
{
	DWORD nTick = GetTickCount();
	if(nTick >= nPrevTick)	nTick -= nPrevTick;
	else					nTick += ULONG_MAX - nPrevTick;
	return nTick;
}

//Òðàññèðîâêà ñòåêà
template<std::size_t SIZE> wchar_t* CLogFile::StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr)
{
	int pos = swprintf_s(buffer.data(), buffer.size(), L"	WS: ÑÒÅÊ: ");
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

void CLogFile::LifeLoop(BOOL bRestart)
{
	vector<HANDLE> events;
	events.push_back(m_hEvtStop);
	events.push_back(m_hEvtQueue);

	bool bRetry = true;
	while(bRetry)
	{
		DWORD evtno = WaitForMultipleObjects(events.size(), events.data(), FALSE, INFINITE);
		evtno -= WAIT_OBJECT_0;

		switch(evtno)
		{
			case 0:	//Stop
			{
				bRetry = false;
				break;
			}
			case 1:
			{
				auto qsize = m_queue.size();
				while(qsize--)
				{
					wstring pmsg;
					{
						std::lock_guard<std::mutex> _m_lock(m_mxQueue);
						pmsg = m_queue.front();
						m_queue.pop();
					}
					if(m_hLog == INVALID_HANDLE_VALUE) AddToMessageLog(LPTSTR(pmsg.c_str()), FALSE, EVENTLOG_INFORMATION_TYPE, 0);
					else
					{
						DWORD dwBytesWritten = 0;
						if(!WriteFile(m_hLog, (LPCVOID)pmsg.c_str(), pmsg.length()*sizeof(wchar_t), &dwBytesWritten, NULL))
						{
							DWORD dw = GetLastError();
							std::array<wchar_t, 1024> chMsg;
							swprintf_s(chMsg.data(), chMsg.size(), L"Log file write error %u(%0x). Msg:%s", dw, dw, pmsg.c_str());
							AddToMessageLog(LPTSTR(chMsg.data()));
						}
					}
					auto fileLen = GetFileSize(m_hLog, NULL);
					if(fileLen == INVALID_FILE_SIZE)
					{
						DWORD dw = GetLastError();
						std::array<wchar_t, 1024> chMsg;
						swprintf_s(chMsg.data(), chMsg.size(), L"Log file get file size error %u(%0x). Msg:%s", dw, dw, pmsg.c_str());
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
							swprintf_s(chMsg.data(), chMsg.size(), L"Log file move error %u(%0x). Msg:%s", dw, dw, pmsg.c_str());
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
				break;
			}
		}
	}
}

void CLogFile::AddToMessageLog(LPTSTR lpszMsg, BOOL bSystem, WORD wType, WORD wCategory)
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

void CLogFile::PutMessage(const wchar_t* wszMsg)
{
	std::lock_guard<std::mutex> _m_lock(m_mxQueue);
	m_queue.push(wszMsg);
	SetEvent(m_hEvtQueue);
}
void CLogFile::_LogWrite(const wchar_t* msg, ...)
{
	std::array<wchar_t, 2048> szBuffer;
	std::array<wchar_t, 32> szTime;
	std::array<wchar_t, 32> szDate;
	va_list vargs;
	__try
	{
		int res = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, L"HH:mm:ss", szTime.data(), szTime.size());
		res = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, NULL, L"dd.MM.yyyy", szDate.data(), szDate.size(), NULL);
		swprintf_s(szBuffer.data(), szBuffer.size(), L"%s-%s: ", szTime.data(), szDate.data());

		va_start(vargs, msg);
		_vsnwprintf_s(&szBuffer[wcslen(szBuffer.data())], szBuffer.size() - wcslen(szBuffer.data()), szBuffer.size() - wcslen(szBuffer.data()), msg, vargs);
		va_end(vargs);
		if(szBuffer[wcslen(szBuffer.data()) - 1] == L'\n') szBuffer[wcslen(szBuffer.data()) - 1] = 0;
		swprintf_s(&szBuffer[wcslen(szBuffer.data())], szBuffer.size() - wcslen(szBuffer.data()), L"\r\n");

		PutMessage(szBuffer.data());
	}
	__except(ErrorFilter(GetExceptionInformation()), 1)
	{
		std::array<wchar_t, 1024> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"%S", "Last message buffer is empty!");
		AddToMessageLog(chMsg.data());
	}
}
