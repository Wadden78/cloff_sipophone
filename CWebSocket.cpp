#include "CWebSocket.h"
#include <Ws2tcpip.h>
#include <rapidjson/document.h>

CWebSocket::CWebSocket()
{
	m_hEvtStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvtQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	m_hTCPListenSocketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	int iCnt = 0;
	for(auto& cl : m_aClients)
	{
		cl.iNum = iCnt++;
		cl.m_hSocketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	m_Log._LogWrite(L"WS: Constructor");
}

CWebSocket::~CWebSocket()
{
	CloseHandle(m_hTimer);
	CloseHandle(m_hEvtQueue);
	CloseHandle(m_hEvtStop);
	for(auto& cl : m_aClients) CloseHandle(cl.m_hSocketEvent);
	CloseHandle(m_hTCPListenSocketEvent);
	m_Log._LogWrite(L"	WS: Destructor");
}

bool CWebSocket::_Start(void)
{
	if(Open())	m_hThread = (HANDLE)_beginthreadex(NULL, 0, s_ThreadProc, this, 0, (unsigned*)&m_ThreadID);
	return m_hThread == NULL ? false : true;
}

bool CWebSocket::_Stop(int time_out)
{
	m_Log._LogWrite(L"	WS: Stop");
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
void CWebSocket::ErrorFilter(LPEXCEPTION_POINTERS ep)
{
	std::array<wchar_t, 1024> szBuf;
	swprintf_s(szBuf.data(), szBuf.size(), L"	WS: ПЕРЕХВАЧЕНА ОШИБКА ПО АДРЕСУ 0x%p", ep->ExceptionRecord->ExceptionAddress);
	m_Log._LogWrite(szBuf.data());
	swprintf_s(szBuf.data(), szBuf.size(), L"	WS: %s", StackView(szBuf, ep->ExceptionRecord->ExceptionAddress, ep->ContextRecord->Ebp));
	m_Log._LogWrite(szBuf.data());
}

// Функция потока
unsigned __stdcall CWebSocket::s_ThreadProc(LPVOID lpParameter)
{
	((CWebSocket*)lpParameter)->LifeLoopOuter();
	return 0;
}

// Внешний цикл потока
void CWebSocket::LifeLoopOuter()
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
			swprintf_s(chMsg.data(), chMsg.size(), L"	WS: %S", "Last message buffer is empty!");
			m_Log._LogWrite(chMsg.data());
			if(nCriticalErrorCounter == 0) nTick = GetTickCount();
			nCriticalErrorCounter++;
			if(nCriticalErrorCounter > 4)
			{
				nCriticalErrorCounter = 0;
				nTick = GetTickCountDifference(nTick);
				if(nTick < 60000)
				{
					m_Log._LogWrite(L"	WS: 5 СБОЕВ НА ПОТОКЕ В ТЕЧЕНИИ МИНУТЫ !!!!  ПОТОК ОСТАНОВЛЕН");
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

DWORD CWebSocket::GetTickCountDifference(DWORD nPrevTick)
{
	auto nTick = GetTickCount64();
	if(nTick >= nPrevTick)	nTick -= nPrevTick;
	else					nTick += ULONG_MAX - nPrevTick;
	return (DWORD)nTick;
}

//Трассировка стека
template<std::size_t SIZE> wchar_t* CWebSocket::StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr)
{
	int pos = swprintf_s(buffer.data(), buffer.size(), L"	WS: СТЕК: ");
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

void CWebSocket::LifeLoop(BOOL bRestart)
{
	vector<HANDLE> events;
	events.push_back(m_hEvtStop);
	events.push_back(m_hEvtQueue);
	events.push_back(m_hTimer);
	events.push_back(m_hTCPListenSocketEvent);
	int i0ClientEvent = events.size();
	for(auto& cl : m_aClients)	events.push_back(cl.m_hSocketEvent);


	m_Log._LogWrite(L"	WS: LifeLoop");
	SetTimer(100);

	bool bRetry = true;
	while(bRetry)
	{
		DWORD evtno = WaitForMultipleObjects(events.size(), events.data(), FALSE, INFINITE);
		if(evtno == WAIT_TIMEOUT)	continue;
		if(evtno == WAIT_FAILED)
		{
			m_Log._LogWrite(L"	WS: LifeLoop ERROR = %d", GetLastError());
			bRetry = false;
			break;
		}
		evtno -= WAIT_OBJECT_0;
		switch(evtno)
		{
			case 0:	//Stop
			{
				bRetry = false;
				m_Log._LogWrite(L"	WS: Поток останавливается...");
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
			case 3:
			{
				m_Log._LogWrite(L"	WS: TCP client Connect request.");

				SOCKADDR_IN Addr;
				int addrlen = sizeof(Addr);
				// Считываем, кто подключился
				int iDataNum{ 0 };
				if(m_aClients[0].m_hSocket == INVALID_SOCKET) iDataNum = 0;
				else if(m_aClients[1].m_hSocket == INVALID_SOCKET) iDataNum = 1;
				else break;

				m_aClients[iDataNum].m_hSocket = WSAAccept(m_hTCPListenSocket, (SOCKADDR*)&Addr, &addrlen, NULL, 0);
				if(m_aClients[iDataNum].m_hSocket == INVALID_SOCKET)	m_Log._LogWrite(L"	WS: TCP Ошибка выполнения WSAAccept: %i", WSAGetLastError());
				else
				{
					char szBbuffer[256];
					WORD remoteport = HIBYTE(Addr.sin_port) | LOBYTE(Addr.sin_port) << 8;
					sprintf_s(szBbuffer, sizeof(szBbuffer), "%d.%d.%d.%d:%d",
						Addr.sin_addr.S_un.S_un_b.s_b1,
						Addr.sin_addr.S_un.S_un_b.s_b2,
						Addr.sin_addr.S_un.S_un_b.s_b3,
						Addr.sin_addr.S_un.S_un_b.s_b4,
						remoteport);
					m_Log._LogWrite(L"	WS: TCP client num=%d %S : Подключён.", iDataNum, szBbuffer);

					m_aClients[iDataNum].m_bClick2Call = false;
					m_aClients[iDataNum].m_dwNumberOfBytesRecvd = 0;
					m_aClients[iDataNum].m_strMessageBuffer.clear();
					m_aClients[iDataNum].m_strTCPFrame.clear();
					m_aClients[iDataNum].m_uiFrameLen = 0;
					m_aClients[iDataNum].m_uiBytesPackets = 0;
					m_aClients[iDataNum].m_bOpCode = 0;
					m_aClients[iDataNum].m_bFIN = 0;
					m_aClients[iDataNum].m_strMask.clear();
					m_aClients[iDataNum].m_strSSID = to_string(iGetRnd()) + to_string(iGetRnd());
					int value = 1;
					if(::setsockopt(m_aClients[iDataNum].m_hSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value)) != 0) m_Log._LogWrite(L"	WS: Can't set TCP_NODELAY");
					else
					{
						bool bVal = true;
						if(::setsockopt(m_aClients[iDataNum].m_hSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&bVal, sizeof(bVal)) != 0)	m_Log._LogWrite(L"	WS: Can't set SO_KEEPALIVE");
						else
						{
							if(WSAEventSelect(m_aClients[iDataNum].m_hSocket, m_aClients[iDataNum].m_hSocketEvent, FD_CLOSE | FD_READ | FD_WRITE) == SOCKET_ERROR)
							{
								m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения WSAEventSelect: %i.", iDataNum, WSAGetLastError());
								if(m_aClients[iDataNum].m_hSocket != INVALID_SOCKET)
								{
									::closesocket(m_aClients[iDataNum].m_hSocket);
									ResetEvent(m_aClients[iDataNum].m_hSocketEvent);
									m_aClients[iDataNum].m_hSocket = INVALID_SOCKET;
								}
							}
						}
					}
				}
				break;
			}
			default:
			{
				int iClientNum = evtno - i0ClientEvent;
				if(iClientNum >= m_aClients.size()) break;

				if(m_aClients[iClientNum].m_hSocket == INVALID_SOCKET) break;
				WSANETWORKEVENTS NetworkEvents;
				if(WSAEnumNetworkEvents(m_aClients[iClientNum].m_hSocket, events[evtno], &NetworkEvents) == SOCKET_ERROR)
				{
					auto errCode = WSAGetLastError();
					m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения WSAEnumNetworkEvents: %i.", iClientNum, errCode);
				}
				else
				{
					if(NetworkEvents.lNetworkEvents & FD_CLOSE) //Закрытие
					{
						int error = 0;
						if(m_aClients[iClientNum].m_hSocket != INVALID_SOCKET)
						{
							if(::closesocket(m_aClients[iClientNum].m_hSocket) == SOCKET_ERROR) m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения closesocket: %i.", iClientNum, WSAGetLastError());
						}
						ResetEvent(m_aClients[iClientNum].m_hSocketEvent);
						m_aClients[iClientNum].m_hSocket = INVALID_SOCKET;
						SendMessage(hDlgPhoneWnd, WM_USER_SIPWEB_OFF, 0, 0);
						m_Log._LogWrite(L"	WS: client num=%d Отключение клиента.", iClientNum);
					}
					else
					{
						if(NetworkEvents.lNetworkEvents & FD_READ)//Чтение
						{
							if(!NetworkEvents.iErrorCode[FD_READ_BIT])
							{
								if(m_aClients[iClientNum].m_strMessageBuffer.length() > 64000) m_aClients[iClientNum].m_strMessageBuffer.clear();	//предохранитель от переполнения буфера
								m_aClients[iClientNum].m_dwNumberOfBytesRecvd = 0;

								DWORD flags = 0;
								bool bReadEnd = true;
								do
								{
									bReadEnd = true;
									auto iBytesRead = recv(m_aClients[iClientNum].m_hSocket, &m_aClients[iClientNum].m_cRecBuffer[m_aClients[iClientNum].m_dwNumberOfBytesRecvd], (int)sizeof(m_aClients[iClientNum].m_cRecBuffer) - m_aClients[iClientNum].m_dwNumberOfBytesRecvd, 0);
									if(iBytesRead == SOCKET_ERROR)
									{
										int error = WSAGetLastError();
										if(error != WSA_IO_PENDING) m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения recv: %i", iClientNum, error);
										else bReadEnd = false;
									}
									if(iBytesRead == 0)	bReadEnd = false;
									else m_aClients[iClientNum].m_dwNumberOfBytesRecvd += iBytesRead;
								} while(!bReadEnd);

								if(m_aClients[iClientNum].m_dwNumberOfBytesRecvd > 0)
								{
									m_aClients[iClientNum].m_cRecBuffer[m_aClients[iClientNum].m_dwNumberOfBytesRecvd] = 0;
									SetMessageHeaderPointer(m_aClients[iClientNum].m_cRecBuffer.data(), m_aClients[iClientNum].m_vCurrentMsgHeaderPointers, true);

									if(strstr(m_aClients[iClientNum].m_cRecBuffer.data(), "HTTP/1.1")) HandShake(m_aClients[iClientNum]);
									else
									{
										for(DWORD iCnt = 0; iCnt < m_aClients[iClientNum].m_dwNumberOfBytesRecvd;)
										{
											iCnt = WEBSocketDecoder(m_aClients[iClientNum], &m_aClients[iClientNum].m_cRecBuffer[iCnt], m_aClients[iClientNum].m_dwNumberOfBytesRecvd);
										}
									}
								}
							}
							else m_Log._LogWrite(L"	WS: client num=%d Ошибка приёма. Ошибка: %i", iClientNum, NetworkEvents.iErrorCode[FD_READ_BIT]);
						}
						else
						{
							if(NetworkEvents.lNetworkEvents & FD_WRITE)//Запись
							{
								if(NetworkEvents.iErrorCode[FD_WRITE_BIT]) m_Log._LogWrite(L"	WS: client num=%d Ошибка передачи. Ошибка: %i", iClientNum, NetworkEvents.iErrorCode[FD_WRITE_BIT]);
							}
						}
					}
				}
				break;
			}
		}
	}
	Close();
}

void CWebSocket::SetTimer(int imsInterval)
{
	long int m_limsInterval;                    /**< Значение интервала таймера*/
	m_limsInterval = imsInterval;
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = -10000 * (_int64)m_limsInterval;
	CancelWaitableTimer(m_hTimer);
	SetWaitableTimer(m_hTimer, &liDueTime, 0, NULL, NULL, false);
}

bool CWebSocket::Tick()
{
	bool bRet = false;

	return bRet;
}

void CWebSocket::MessageSheduler()
{
	array<char, 2048> abMessageBuffer;
	abMessageBuffer[0] = 0;

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
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"incall\",\r\n\"phone\": \"%ls\"\r\n}", pinc->wstrCallingPNum.c_str());
				break;
			}
			case nsMsg::enMsgType::enMT_Progress:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 4,\r\n\"error\": 0\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_Alerting:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 4,\r\n\"error\": 0\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_Answer:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 6,\r\n\"error\": 0\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_Decline:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 5,\r\n\"error\": 403\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_Cancel:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 7,\r\n\"error\": 480\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_Disconnect:
			{
				auto Dismsg = static_cast<nsMsg::SDisconnect*>(pmsg.get());
				wstring wstrInfo;
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 7,\r\n\"error\": %d\r\n,\"info\": \"%ls\"\r\n}", Dismsg->iCause, Dismsg->wstrInfo.c_str());
				break;
			}
			case nsMsg::enMsgType::enMT_Error:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 100,\r\n\"error\": %d\r\n}", Errmsg->iErrorCode);
				break;
			}
			case nsMsg::enMsgType::enMT_RegSet:
			{
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 2,\r\n\"error\": 0\r\n}");
				break;
			}
			case nsMsg::enMsgType::enMT_RegError:
			{
				auto Errmsg = static_cast<nsMsg::SError*>(pmsg.get());
				sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 3,\r\n\"error\": %d\r\n}", Errmsg->iErrorCode);
				break;
			}
			case nsMsg::enMsgType::enMT_Status:
			{
				auto Statusmsg = static_cast<nsMsg::SStatus*>(pmsg.get());
				if(Statusmsg->iStatusCode == 1) sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 2,\r\n\"error\": 0\r\n}");
				else							sprintf_s(abMessageBuffer.data(), abMessageBuffer.size(), "{\r\n\"command\": \"status\",\r\n\"code\": 9,\r\n\"error\": %d\r\n,\"info\": \"%ls\"\r\n}", Statusmsg->iStatusCode, Statusmsg->wstrInfo.c_str());
				break;
			}
			default:
			{
				break;
			}
		}
	}
	auto tResBufLen = strlen(abMessageBuffer.data());
	if(tResBufLen)
	{
		vector<BYTE> vCodedBuffer;
		vCodedBuffer.resize(tResBufLen + 256);
		int iLen = WEBSocketEncoder(kOpCodeText, (BYTE*)abMessageBuffer.data(), strlen(abMessageBuffer.data()), vCodedBuffer.data(), vCodedBuffer.size());
		if(iLen)
		{
			for(auto& cl : m_aClients) if((cl.m_hSocket != INVALID_SOCKET) && !cl.m_bClick2Call) { MsgSendToRemoteSide(cl, vCodedBuffer.data(), iLen); break; }
		}
	}
}

// Открыть
bool CWebSocket::Open()
{
	bool bRet = false;
	//-----------------------------------------
	// Declare and initialize variables.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != NO_ERROR) m_Log._LogWrite(L"	WS: Error at WSAStartup()");

	// bind
	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(ipWSPort);
	inet_pton(AF_INET, "127.0.0.1", &sockAddr.sin_addr.S_un.S_addr);

	// create socket
	bRet = true;
	m_hTCPListenSocket = WSASocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if(m_hTCPListenSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		m_Log._LogWrite(L"	WS: TCP WSASocket error: %i (0x%X)", error, error);
		bRet = false;
	}

	// select event to socket
	if(bRet && (WSAEventSelect(m_hTCPListenSocket, m_hTCPListenSocketEvent, FD_ACCEPT) == SOCKET_ERROR))
	{
		int error = WSAGetLastError();
		m_Log._LogWrite(L"	WS: TCP WSAEventSelect error: %i (0x%X)", error, error);
		bRet = FALSE;
	}
	if(bRet)
	{
		// bind
		if(::bind(m_hTCPListenSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			m_Log._LogWrite(L"	WS: TCP bind error: %i (0x%X)", error, error);
			bRet = false;
		}
		else
		{
			m_Log._LogWrite(L"	WS: TCP bind OK on port: %i.", ipWSPort);
			// Ставим сокет в готовность к подключению
			bRet = false;
			if(::listen(m_hTCPListenSocket, SOMAXCONN) == SOCKET_ERROR)	m_Log._LogWrite(L"	WS: TCP Ошибка выполнения listen: %i", WSAGetLastError());
			else
			{
				m_Log._LogWrite(L"	WS: TCP listen OK on port: %i.", ipWSPort);
				bRet = true;
			}
		}
	}
	return bRet;
}
void CWebSocket::Close()
{
	for(auto& cl : m_aClients)
	{
		if(cl.m_hSocket != INVALID_SOCKET)
		{
			if(::closesocket(cl.m_hSocket) == SOCKET_ERROR)
			{
				int error = WSAGetLastError();
				m_Log._LogWrite(L"	WS: closesocket error: %i (0x%X)", error, error);
			}
		}
	}

	if(m_hTCPListenSocket != INVALID_SOCKET)
	{
		if(closesocket(m_hTCPListenSocket) == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			m_Log._LogWrite(L"	WS: TCP closesocket error: %i (0x%X)", error, error);
		}
	}
	WSACleanup();
}

void CWebSocket::MsgSendToRemoteSide(SClient& sData, const BYTE* pMsg, size_t stMsgLen)
{
	DWORD NumberOfBytesSent;
	WSABUF buff;
	buff.buf = (char*)pMsg;
	buff.len = stMsgLen;

	int iRes = WSASend(sData.m_hSocket, &buff, 1, &NumberOfBytesSent, 0, NULL, NULL);
	int error = WSAGetLastError();
	switch(iRes)
	{
		case 0: break;
		case SOCKET_ERROR:
		{
			if(error == WSA_IO_PENDING) m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения WSASend: WSA_IO_PENDING %i", sData.iNum, error);
			break;
		}
		default:	m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения WSASend: %i", sData.iNum, iRes);	break;
	}
}

int CWebSocket::WEBSocketEncoder(OpCodeEnum opCode, const BYTE* pbSrcBuffer, size_t stSourceLen, BYTE* pbDstBuffer, size_t stDestSize)
{
//Output(MSG_TEST, "TextOut: %s", pbSrcBuffer);
	int iMsgLen(0);
	if(stDestSize < sizeof(SWEBSocketFrame)) return 0;
	BYTE* pDst = pbDstBuffer + sizeof(SWEBSocketFrame);
	SWEBSocketFrame* pFrame = (SWEBSocketFrame*)pbDstBuffer;
	pFrame->RSV1 = 0;
	pFrame->RSV2 = 0;
	pFrame->RSV3 = 0;
	pFrame->OpCode = opCode;
	pFrame->FIN = 1;
	pFrame->Mask = 0;
	if(stSourceLen < 126) pFrame->BodyLen = stSourceLen;
	else if(stSourceLen < MAXSHORT)
	{
		if(stDestSize < (sizeof(SWEBSocketFrame) + sizeof(UINT16) + sizeof(UINT32) + stSourceLen)) return 0;
		pFrame->BodyLen = 126;
		*((UINT16*)pDst) = (UINT16)htons((u_short)stSourceLen);
		pDst += sizeof(UINT16);
	}
	else
	{
		if(stDestSize < (sizeof(SWEBSocketFrame) + sizeof(UINT64) + sizeof(UINT32) + stSourceLen)) return 0;
		pFrame->BodyLen = 127;
		*((UINT64*)pDst) = htonl(stSourceLen);
		pDst += sizeof(UINT64);
	}
	if(stSourceLen)
	{
//		pFrame->Mask = 1;
		//UINT32 uiMask = rand()*rand();
		//*((UINT32*)pDst) = (UINT32)htonl(uiMask);
		//byte *pMask = pDst;
		//pDst += sizeof(UINT32);
//		for(size_t i = 0; i < stSourceLen; i++, ++pDst) *pDst = pbSrcBuffer[i] ^ pMask[i & 0x03];
		for(size_t i = 0; i < stSourceLen; i++, ++pDst) *pDst = pbSrcBuffer[i];
	}
	return pDst - pbDstBuffer;
}

int CWebSocket::WEBSocketDecoder(SClient& sData, const char* const src, const UINT32 iBytesCount)
{
	int iRet = iBytesCount;

	if(!sData.m_uiFrameLen)
	{
		SWEBSocketFrame* pFrame = (SWEBSocketFrame*)src;
		sData.m_bFIN = pFrame->FIN == 1 ? true : false;
		if(pFrame->OpCode != kOpCodeContinuation) sData.m_bOpCode = pFrame->OpCode;
		sData.m_strMask.clear();
		sData.m_uiFrameLen = pFrame->BodyLen;
		size_t stBefore = sData.m_strTCPFrame.length();
		if(sData.m_uiFrameLen == 126)
		{
			sData.m_uiFrameLen = ntohs(*((UINT16*)((BYTE*)src + sizeof(SWEBSocketFrame))));
			if(pFrame->Mask)
			{
				sData.m_strMask.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT16), 4);
				sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT16) + sizeof(UINT32), iBytesCount - (sizeof(SWEBSocketFrame) + sizeof(UINT16) + sizeof(UINT32)));
			}
			else sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT16), iBytesCount - (sizeof(SWEBSocketFrame) + sizeof(UINT16)));
		}
		else if(sData.m_uiFrameLen == 127)
		{
			sData.m_uiFrameLen = ntohl((u_long)(*((UINT64*)((BYTE*)src + sizeof(SWEBSocketFrame)))));
			if(pFrame->Mask)
			{
				sData.m_strMask.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT64), 4);
				sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT64) + sizeof(UINT32), iBytesCount - (sizeof(SWEBSocketFrame) + sizeof(UINT64) + sizeof(UINT32)));
			}
			else sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT64), iBytesCount - (sizeof(SWEBSocketFrame) + sizeof(UINT64)));
		}
		else
		{
			if(pFrame->Mask)
			{
				sData.m_strMask.append(src + sizeof(SWEBSocketFrame), 4);
				sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame) + sizeof(UINT32), iBytesCount - (sizeof(SWEBSocketFrame) + sizeof(UINT32)));
			}
			else sData.m_strTCPFrame.append(src + sizeof(SWEBSocketFrame), iBytesCount - sizeof(SWEBSocketFrame));
		}
		sData.m_uiBytesPackets = sData.m_strTCPFrame.length() - stBefore;
	}
	else
	{
		sData.m_strTCPFrame.append(src, iBytesCount);
		sData.m_uiBytesPackets += iBytesCount;
	}

	if(sData.m_uiBytesPackets == sData.m_uiFrameLen)	//получен весь фрагментированный на TCP пакеты фрейм
	{
		if(sData.m_strMask.length()) for(size_t i = sData.m_strTCPFrame.length() - (size_t)sData.m_uiFrameLen, msk_c = 0; i < sData.m_strTCPFrame.length(); i++, msk_c++) sData.m_strTCPFrame[i] = ("%c", sData.m_strTCPFrame[i] ^ sData.m_strMask[msk_c & 0x03]);
		if(sData.m_bFIN)	//это последний frame сообщения, переданного через websocket
		{
			switch(sData.m_bOpCode)
			{
				case kOpCodeContinuation: break;
				case kOpCodeText:
				{
#ifdef _DEBUG
					m_Log._LogWrite(L"	WS: client num=%d Receive: '%S'", sData.iNum, sData.m_strTCPFrame.c_str());
#endif // DEBUG
					PacketParser(sData, sData.m_strTCPFrame.c_str());
					break;
				}
				case kOpCodeBinary: break;
				case kOpCodeClose:
				{
					UINT16 iCode = ntohs(*(UINT16*)sData.m_strTCPFrame.c_str());
					m_Log._LogWrite(L"	WS: client num=%d Close: Code=%d", sData.iNum, iCode);
					if(::closesocket(sData.m_hSocket) == SOCKET_ERROR) m_Log._LogWrite(L"	WS: client num=%d Ошибка выполнения closesocket: %i.", sData.iNum, WSAGetLastError());
					ResetEvent(sData.m_hSocketEvent);
					sData.m_hSocket = INVALID_SOCKET;
					m_Log._LogWrite(L"	WS: client num=%d Отключение клиента.", sData.iNum);
					SendMessage(hDlgPhoneWnd, WM_USER_SIPWEB_OFF, 0, 0);
					break;
				}
				case kOpCodePing:
				{
					vector<BYTE> pDst; pDst.resize(iBytesCount + 1);
					memcpy(pDst.data(), src, iBytesCount);
					pDst[iBytesCount] = 0;

					SWEBSocketFrame* pFrame = (SWEBSocketFrame*)pDst.data();
					pFrame->OpCode = kOpCodePong;
					MsgSendToRemoteSide(sData, pDst.data(), iBytesCount);
					break;
				}
				case kOpCodePong: break;
				case kOpCodeControlUnused: break;

			}
			sData.m_bOpCode = -1;
			sData.m_strMask.clear();
			sData.m_strTCPFrame.clear();
			sData.m_bFIN = false;
		}
		sData.m_uiBytesPackets = sData.m_uiFrameLen = 0;
	}

	return iRet;
}

void CWebSocket::HandShake(SClient& sData)
{
	char szBuf[1024];
	if(InHeaderSubStrFind("GET", sData.m_vCurrentMsgHeaderPointers, 0))	//Сообщение протокола WEBSocket
	{
		char* pHeaderBegin = NULL, * pHeaderEnd = NULL;
		std::vector<char*>::size_type pHeader = FindHeaderByName("Connection", "Connection", sData.m_vCurrentMsgHeaderPointers, 0, &pHeaderBegin, &pHeaderEnd);
		if(pHeader < sData.m_vCurrentMsgHeaderPointers.max_size())
		{
			if(InHeaderSubStrFind("Upgrade", sData.m_vCurrentMsgHeaderPointers, pHeader))
			{
				pHeader = FindHeaderByName("Upgrade", "Upgrade", sData.m_vCurrentMsgHeaderPointers, 0, &pHeaderBegin, &pHeaderEnd);
				if(pHeader < sData.m_vCurrentMsgHeaderPointers.max_size())
				{
					if(((pHeaderEnd - pHeaderBegin) == strlen("websocket")) && !_strnicmp(pHeaderBegin, "websocket", strlen("websocket")))
					{
						pHeader = FindHeaderByName("Sec-WebSocket-Protocol", "Sec-WebSocket-Protocol", sData.m_vCurrentMsgHeaderPointers, 0, &pHeaderBegin, &pHeaderEnd);
						std::string strAnswer;
						sData.m_strTCPFrame.clear();
						sData.m_uiFrameLen = 0;
						m_Log._LogWrite(L"	WS: client num=%d 'websocket' HandShake", sData.iNum);
						strAnswer.append("HTTP/1.1 101 Switching Protocols\r\n");
						std::vector<char*>::size_type pExtHeader = FindHeaderByName("Sec-WebSocket-Extensions", "Sec-WebSocket-Extensions", sData.m_vCurrentMsgHeaderPointers, 0, &pHeaderBegin, &pHeaderEnd);
						std::vector<char*>::size_type pKeyHeader = FindHeaderByName("Sec-WebSocket-Key", "Sec-WebSocket-Key", sData.m_vCurrentMsgHeaderPointers, 0, &pHeaderBegin, &pHeaderEnd);
						std::string strKey;
						if(pKeyHeader < sData.m_vCurrentMsgHeaderPointers.max_size())
						{
							CountWebSecureKey((BYTE*)pHeaderBegin, pHeaderEnd - pHeaderBegin, strKey);
						}
						for(std::vector<char*>::size_type stCount = 1; stCount < sData.m_vCurrentMsgHeaderPointers.size(); stCount++)
						{
							if(stCount != pExtHeader)
							{
								if(stCount == pKeyHeader)
								{
									sprintf_s(szBuf, sizeof(szBuf), "Sec-WebSocket-Accept: %s\r\n", strKey.c_str());
									strAnswer.append(szBuf);
								}
								else
								{
									if(stCount == (sData.m_vCurrentMsgHeaderPointers.size() - 1)) strAnswer.append(sData.m_vCurrentMsgHeaderPointers[stCount], strlen(sData.m_vCurrentMsgHeaderPointers[stCount]));
									else strAnswer.append(sData.m_vCurrentMsgHeaderPointers[stCount], sData.m_vCurrentMsgHeaderPointers[stCount + 1] - sData.m_vCurrentMsgHeaderPointers[stCount]);
								}
							}
						}
						MsgSendToRemoteSide(sData, (const BYTE*)strAnswer.c_str(), strAnswer.length());
					}
				}
			}
		}
	}
}

void CWebSocket::PacketParser(SClient& sData, const char* pMessage)
{
	array<char, 512> szAnswer{ 0 };
	rapidjson::Document document;
	document.Parse(pMessage);

	if(!document.HasParseError() && document.IsObject())
	{
		if(document.HasMember("command") && document["command"].IsString())
		{
			if(!strcmp(document["command"].GetString(), "register"))
			{
				m_Log._LogWrite(L"	WS: Запрос регистрации.");
				if(document.HasMember("account") && document["account"].IsString())
				{
					if(document.HasMember("password") && document["password"].IsString())
					{
						auto iRes = SendMessage(hDlgPhoneWnd, WM_USER_SIPWEB_ON, (WPARAM)document["account"].GetString(), (LPARAM)document["password"].GetString());
						if(iRes != 0) sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"status\",\r\n \"errcode\": 100\r\n}");
					}
					else m_Log._LogWrite(L"	WS: Не указан пароль в запросе регистрации!");
				}
				else m_Log._LogWrite(L"	WS: Не указан аккаунт в запросе регистрации!");
			}
			else if(!strcmp(document["command"].GetString(), "call"))
			{
				m_Log._LogWrite(L"	WS: Запрос вызова.");
				if(document.HasMember("phone") && document["phone"].IsString())
				{
					if(hDlgPhoneWnd)
					{
						if(document.HasMember("show") && document["show"].IsInt())
						{
							if(document["show"].GetInt() == 1) RestoreFromTray();
						}
						SendMessage(hDlgPhoneWnd, WM_USER_SIPWEB_MC, sData.m_bClick2Call ? 1 : 0, (LPARAM)document["phone"].GetString());
						sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"call\",\r\n \"errcode\": 0\r\n}");
						if(sData.m_bClick2Call)
						{
							::closesocket(sData.m_hSocket);
							sData.m_hSocket = INVALID_SOCKET;
							sData.m_bClick2Call = false;
							m_Log._LogWrite(L"	WS: clien num=%d Закрытие сокета click2call.", sData.iNum);
						}
					}
				}
				else sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"call\",\r\n \"errcode\": 1\r\n\"errmessage:\":\"phone number is missed\"}");
			}
			else if(!strcmp(document["command"].GetString(), "dtmf"))
			{
				m_Log._LogWrite(L"	WS: Запрос DTMF.");
				if(document.HasMember("button") && document["button"].IsString())
				{
					if(hDlgPhoneWnd)
					{
						array<wchar_t, 8> wszDigit;
						swprintf_s(wszDigit.data(), wszDigit.size(), L"%C", document["button"].GetString()[0]);
						SendMessage(hDlgPhoneWnd, WM_USER_DIGIT, (WPARAM)0, (LPARAM)wszDigit[0]);
						sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"dtmf\",\r\n \"errcode\": 0\r\n}");
					}
				}
			}
			else if(!strcmp(document["command"].GetString(), "answer"))
			{
				m_Log._LogWrite(L"	WS: Запрос ответа.");
				SendMessage(hDlgPhoneWnd, WM_USER_CALL_ANM, (WPARAM)0, (LPARAM)0);
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"answer\",\r\n \"errcode\": 0\r\n}");
			}
			else if(!strcmp(document["command"].GetString(), "hangup"))
			{
				m_Log._LogWrite(L"	WS: Запрос отбоя.");
				SendMessage(hDlgPhoneWnd, WM_USER_CALL_DSC, (WPARAM)0, (LPARAM)0);
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"hangup\",\r\n \"errcode\": 0\r\n}");
			}
			else if(!strcmp(document["command"].GetString(), "status"))
			{
				m_Log._LogWrite(L"	WS: Запрос статуса.");
				auto pMsg = make_unique<nsMsg::SMsg>();
				pMsg->iCode = nsMsg::enMsgType::enMT_Status;
				m_SIPProcess->_PutMessage(pMsg);
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"status\",\r\n \"errcode\": 0\r\n}");
			}
			else if(!strcmp(document["command"].GetString(), "mute"))
			{
				m_Log._LogWrite(L"	WS: Запрос изменения громкости микрофона.");
				if(document.HasMember("level") && document["level"].IsInt())
				{
					dwMicLevel = document["level"].GetInt();
					SendMessage(hDlgPhoneWnd, WM_USER_MIC_LEVEL, (WPARAM)0, (LPARAM)document["level"].GetInt());
					sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"level\",\r\n \"errcode\": 0\r\n}");
				}
			}
			else if(!strcmp(document["command"].GetString(), "volume"))
			{
				m_Log._LogWrite(L"	WS: Запрос изменения громкости динамика.");
				if(document.HasMember("level") && document["level"].IsInt())
				{
					dwSndLevel = document["level"].GetInt();
					SendMessage(hDlgPhoneWnd, WM_USER_SND_LEVEL, (WPARAM)0, (LPARAM)document["level"].GetInt());
					sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"volume\",\r\n \"errcode\": 0\r\n}");
				}
			}
			else if(!strcmp(document["command"].GetString(), "version"))
			{
				m_Log._LogWrite(L"	WS: Запрос получения версии сипфона.");
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"version\",\r\n \"value\": \"%s\"\r\n}", strProductVersion.c_str());
			}
			else if(!strcmp(document["command"].GetString(), "unregister"))
			{
				m_Log._LogWrite(L"	WS: Запрос снятия регистрации.");
				SendMessage(hDlgPhoneWnd, WM_USER_SIPWEB_OFF, 0, 0);
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"command\": \"unregister\",\r\n \"errcode\": 0\r\n}");
			}
		}
		else if(document.HasMember("mode") && document["mode"].IsString())
		{
			m_Log._LogWrite(L"	WS: Запрос установки режима подключения.");
			if(!strcmp(document["mode"].GetString(), "webinterface"))
			{
				m_Log._LogWrite(L"	WS: WEB интерфейс подключен.");
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"mode\": \"webinterface\",\r\n \"errcode\": 0\r\n}");
			}
			else if(!strcmp(document["mode"].GetString(), "click2call"))
			{
				m_Log._LogWrite(L"	WS: 'Click to call' подключен.");
				sData.m_bClick2Call = true;
				sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"mode\": \"click2call\",\r\n \"errcode\": 0\r\n}");
			}
			else sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"mode\": \"%s\",\r\n \"errcode\": 2\r\n\"errmessage:\":\"unrecognized mode name\"}", document["mode"].GetString());
		}
		else sprintf_s(szAnswer.data(), szAnswer.size(), "{\r\n \"mode\": \"\",\r\n \"errcode\": 1\r\n\"errmessage:\":\"mode name is missed\"}");
	}
	if(szAnswer[0])
	{
		vector<BYTE> vCodedBuffer;
		vCodedBuffer.resize(strlen(szAnswer.data()) + 256);
		int iLen = WEBSocketEncoder(kOpCodeText, (BYTE*)szAnswer.data(), strlen(szAnswer.data()), vCodedBuffer.data(), vCodedBuffer.size());
		if(iLen) MsgSendToRemoteSide(sData, vCodedBuffer.data(), iLen);
	}
}

void CWebSocket::SetMessageHeaderPointer(const char* szMsg, std::vector<char*>& vHeaderPointers, bool bWEB)
{
	int iTraceCode = 0;
	int iHeader = 0;

	vHeaderPointers.clear();
	char* pcHead = (char*)szMsg;
	char* pcMsgEnd = (char*)szMsg + strlen(szMsg);
	std::vector<char*>::size_type stCount;
//	__try
	try
	{
		while(pcHead < pcMsgEnd)
		{
			if((*pcHead != ' ') && (*pcHead != '\t')) //заголовок
			{
				vHeaderPointers.push_back(pcHead);
			}
			pcHead = strstr(pcHead, "\r\n");
			if(pcHead) pcHead += strlen("\r\n");
			else break;
		}
		iTraceCode = 1;
		if(!bWEB)
		{
			iTraceCode = 2;
			bool bContent = false;
			for(stCount = 0; stCount < vHeaderPointers.size(); stCount++)
			{
				iTraceCode = 3;
				if(!_strnicmp(vHeaderPointers[stCount], "\r\n", strlen("\r\n"))) bContent = true;
				iHeader = stCount;
				if(!bContent &&
					(!((strlen(vHeaderPointers[stCount]) > strlen("Authorization:")) && (!_strnicmp(vHeaderPointers[stCount], "Authorization:", strlen("Authorization:")) || !_strnicmp(vHeaderPointers[stCount], "Authorization ", strlen("Authorization ")))) &&
						!((strlen(vHeaderPointers[stCount]) > strlen("Allow:")) && (!_strnicmp(vHeaderPointers[stCount], "Allow:", strlen("Allow:")) || !_strnicmp(vHeaderPointers[stCount], "Allow ", strlen("Allow ")))) &&
						!((strlen(vHeaderPointers[stCount]) > strlen("Allow-Events:")) && (!_strnicmp(vHeaderPointers[stCount], "Allow-Events:", strlen("Allow-Events:")) || !_strnicmp(vHeaderPointers[stCount], "Allow-Events ", strlen("Allow-Events ")))) &&
						!((strlen(vHeaderPointers[stCount]) > strlen("Proxy-Authenticate:")) && (!_strnicmp(vHeaderPointers[stCount], "Proxy-Authenticate:", strlen("Proxy-Authenticate:")) || !_strnicmp(vHeaderPointers[stCount], "Proxy-Authenticate ", strlen("Proxy-Authenticate ")))) &&
						!((strlen(vHeaderPointers[stCount]) > strlen("Supported:")) && (!_strnicmp(vHeaderPointers[stCount], "Supported:", strlen("Supported:")) || !_strnicmp(vHeaderPointers[stCount], "Supported ", strlen("Supported ")))) &&
						!((strlen(vHeaderPointers[stCount]) > strlen("WWW-Authenticate:")) && (!_strnicmp(vHeaderPointers[stCount], "WWW-Authenticate:", strlen("WWW-Authenticate:")) || !_strnicmp(vHeaderPointers[stCount], "WWW-Authenticate ", strlen("WWW-Authenticate ")))))
					)
				{//Заголовки Authorisation,WWW-Authenticate и Allow не рассматриваем- там своё формирование
					iTraceCode = 4;
					char* pcHeaderEnd = stCount < (vHeaderPointers.size() - 1) ? vHeaderPointers[stCount + 1] : vHeaderPointers[stCount] + strlen(vHeaderPointers[stCount]);
					bool bQuoted = false;
					bool bBraces = false;
					char* pcPtr = vHeaderPointers[stCount];
					if(*vHeaderPointers[stCount] == ',') pcPtr++;
					iTraceCode = 5;
					for(; pcPtr < pcHeaderEnd; pcPtr++)
					{
						iTraceCode = 6;
						if(*pcPtr == '\"') bQuoted = !bQuoted;
						if(*pcPtr == '<' && !bQuoted) bBraces = true;
						if(*pcPtr == '>' && !bQuoted) bBraces = false;
						if(!bQuoted && !bBraces && *pcPtr == ',')
						{
							if((stCount + 1) < (vHeaderPointers.size())) vHeaderPointers.insert(vHeaderPointers.begin() + stCount + 1, pcPtr);
							else vHeaderPointers.push_back(pcPtr);
							stCount++;
						}
						iTraceCode = 7;
					}
					iTraceCode = 8;
				}
				iTraceCode = 9;
			}
		}
	}
//	__except(Output(MSG_CRITICALERROR,"Alarm = %d Header = %i", iTraceCode, iHeader), 1)
	catch(...)
	{
		m_Log._LogWrite(L"	WS: SetMessageHeaderPointer TraceCode=%i Msg = %s", iTraceCode, szMsg);
	}
}

vector<char*>::size_type CWebSocket::FindHeaderByName(const char* szFullName, const char* szCompactName, std::vector<char*>& vHeaderPointers, std::vector<char*>::size_type stStart, char** psBodyBegin, char** psBodyEnd)
{
	vector<char*>::size_type stRet = vHeaderPointers.max_size();
	if(psBodyBegin)	*psBodyBegin = NULL;
	if(psBodyEnd) *psBodyEnd = NULL;
	if((0 <= stStart) && (stStart < vHeaderPointers.size()))
	{
		for(std::vector<char*>::size_type stCount = stStart; stCount < vHeaderPointers.size(); stCount++)
		{
			std::vector<char*>::size_type stHeaderNum = stCount;
			while(stHeaderNum > 0 && (*vHeaderPointers[stHeaderNum] == ',')) stHeaderNum--;	//отматываем обратно к имени заголовка
			if(((*(vHeaderPointers[stHeaderNum] + 1) == ' ') || (*(vHeaderPointers[stHeaderNum] + 1) == ':')) && !_strnicmp(vHeaderPointers[stHeaderNum], szCompactName, strlen(szCompactName))) stRet = stCount; //нашли короткий заголовок
			if(stRet != stCount)
			{
				if(((vHeaderPointers[stHeaderNum] + strlen(szFullName) < (stHeaderNum == (vHeaderPointers.size() - 1) ? vHeaderPointers[stHeaderNum] + strlen(vHeaderPointers[stHeaderNum]) : vHeaderPointers[stHeaderNum + 1]))) &&
					((*(vHeaderPointers[stHeaderNum] + strlen(szFullName)) == ' ') || (*(vHeaderPointers[stHeaderNum] + strlen(szFullName)) == ':')) &&
					!_strnicmp(vHeaderPointers[stHeaderNum], szFullName, strlen(szFullName)))
					stRet = stCount;  //нашли полный заголовок
			}
			if(stRet == stCount)//проверяем поле на наличие содержимого
			{
				if((vHeaderPointers[stCount][0] != ',') && (vHeaderPointers[stCount][0] != '\r') && (vHeaderPointers[stCount][1] != '\n'))
				{
					char* pCurrentSym;
					for(pCurrentSym = vHeaderPointers[stCount]; (pCurrentSym < vHeaderPointers[stCount + 1]) && (*pCurrentSym != ':'); pCurrentSym++);
					if(*pCurrentSym == ':')
					{
						for(pCurrentSym++; pCurrentSym < vHeaderPointers[stCount + 1]; pCurrentSym++)
						{
							if(pCurrentSym[0] != ' ')
							{
								if((pCurrentSym[0] == '\r') || (pCurrentSym[0] == '\n')) stRet = vHeaderPointers.max_size();
								else
								{
									if(psBodyBegin && psBodyEnd)//требуется извлечение содержимого
									{
										*psBodyBegin = pCurrentSym;
										*psBodyEnd = stRet == (vHeaderPointers.size() - 1) ? *psBodyBegin + strlen(*psBodyBegin) : vHeaderPointers[stRet + 1];
										if(*((*psBodyEnd) - 1) == '\n') (*psBodyEnd)--;
										if(*((*psBodyEnd) - 1) == '\r') (*psBodyEnd)--;
									}
									break;
								}
							}
						}
					}
				}
			}
			if(stRet == stCount) break;
		}
	}
	return stRet;
}

char* CWebSocket::InHeaderSubStrFind(const char* szPattern, std::vector<char*>& vHeaderPointers, std::vector<char*>::size_type stHeaderPointerNum)
{
	char* pcRet = NULL;

	if(stHeaderPointerNum < vHeaderPointers.size())
	{
		char* psStart = vHeaderPointers[stHeaderPointerNum];
		char* psEnd = (stHeaderPointerNum + 1) >= vHeaderPointers.size() ? psStart + strlen(psStart) : vHeaderPointers[stHeaderPointerNum + 1];
		bool bQuote = false;
		if(psStart == nullptr)
		{
			return nullptr;
		}
		for(; psStart < psEnd; psStart++)
		{
			if(*psStart == '\"') bQuote = !bQuote;	//содержимое кавычек не анализируем
			if(!bQuote && !_strnicmp(psStart, szPattern, strlen(szPattern)))
			{
				pcRet = psStart; break;
			}
		}
	}
	return pcRet;
}

void CWebSocket::CountWebSecureKey(const BYTE* const psInKey, const int iKeyLen, std::string& strResult)
{
	strResult.clear();

	vector<BYTE> strKey; strKey.resize(iKeyLen + strlen(c_szGUID));
	BYTE rgbHash[SHA1LEN];
	DWORD cbHash = 0;
	CHAR rgbDigits[] = "0123456789abcdef";
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;

	memcpy(strKey.data(), psInKey, iKeyLen);
	memcpy(&strKey[iKeyLen], c_szGUID, strlen(c_szGUID));

	if(!CryptAcquireContext(&hProv,
		NULL,
		NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT))
	{
		m_Log._LogWrite(L"	WS: CryptAcquireContext failed: %d\n", GetLastError());
	}
	else if(!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
	{
		m_Log._LogWrite(L"	WS: CryptAcquireContext failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
	}
	else if(!CryptHashData(hHash, strKey.data(), iKeyLen + strlen(c_szGUID), 0))
	{
		m_Log._LogWrite(L"	WS: CryptHashData failed: %d\n", GetLastError());
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
	}
	else
	{
		cbHash = SHA1LEN;
		if(CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		{
			strResult = base64_encode(rgbHash, cbHash);
		}
		else m_Log._LogWrite(L"	WS: CryptGetHashParam failed: %d\n", GetLastError());

		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
	}
}

std::string CWebSocket::base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string strRet;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while(in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if(i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i < 4); i++)	strRet += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if(i)
	{
		for(j = i; j < 3; j++) char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for(j = 0; (j < i + 1); j++) strRet += base64_chars[char_array_4[j]];

		while((i++ < 3)) strRet += '=';

	}

	return strRet;
}

std::string CWebSocket::base64_decode(std::string const& encoded_string)
{
	std::string strRet;
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];

	while(in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if(i == 4)
		{
			for(i = 0; i < 4; i++) char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for(i = 0; (i < 3); i++) strRet += char_array_3[i];
			i = 0;
		}
	}

	if(i)
	{
		for(j = i; j < 4; j++) char_array_4[j] = 0;
		for(j = 0; j < 4; j++) char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for(j = 0; (j < i - 1); j++) strRet += char_array_3[j];
	}

	return strRet;
}

void CWebSocket::_PutMessage(unique_ptr<nsMsg::SMsg>& pMsg)
{
	m_mxQueue.lock();
	__try
	{
		m_queue.push(move(pMsg));
		SetEvent(m_hEvtQueue);
	}
	__except(ErrorFilter(GetExceptionInformation()), 1)
	{
		std::array<wchar_t, 1024> chMsg;
		swprintf_s(chMsg.data(), chMsg.size(), L"%s", static_cast<nsMsg::SLog*>(pMsg.get())->wstrInfo.c_str());
		m_Log._LogWrite(L"%s", chMsg.data());
	}
	m_mxQueue.unlock();
}

