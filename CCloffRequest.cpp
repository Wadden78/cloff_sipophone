#include "CCloffRequest.h"
#include <ws2tcpip.h>


CCloffRequest::CCloffRequest()
{
}
CCloffRequest::~CCloffRequest()
{

}

int CCloffRequest::RequestDataFromDataBase(const char* szRequest, string& strAnswer)
{
	bool bRet{ false };
	string strBuffer;
	vector<char> cRecBuffer; cRecBuffer.resize(0xffff);
	WSABUF buff;
	buff.buf = cRecBuffer.data();
	buff.len = cRecBuffer.size();

	auto m_hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if(m_hSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		m_Log._LogWrite(L"   CLOFFRequest: server socket error=%d", error);
	}
	else
	{
		auto m_hSocketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(WSAEventSelect(m_hSocket, m_hSocketEvent, FD_CLOSE | FD_READ | FD_WRITE | FD_CONNECT) == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			m_Log._LogWrite(L"   CLOFFRequest: WSAEventSelect error=%d(0x%X)", error, error);
		}
		else
		{
			sockaddr_in service;
			service.sin_family = AF_INET;
			service.sin_port = htons(80);

			ADDRINFOW hints{ NULL };
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			PADDRINFOW pAddrRes{ nullptr };

			auto gaRet = GetAddrInfoW(wstrWEBServer.c_str(), L"http", &hints, &pAddrRes);
			if(gaRet == 0)
			{
				for(auto ptr = pAddrRes; ptr != nullptr; ptr = ptr->ai_next)
				{
					if(ptr->ai_family == AF_INET && ptr->ai_socktype == SOCK_STREAM && ptr->ai_protocol == IPPROTO_TCP)
					{
						memcpy(&service, ptr->ai_addr, sizeof(service));
						break;
					}
				}
				service.sin_family = AF_INET;
				service.sin_port = htons(80);

				bool bConnectOk{ false };
				if(WSAConnect(m_hSocket, (LPSOCKADDR)&service, sizeof(SOCKADDR), NULL, NULL, NULL, NULL) == -1)
				{
					int error = WSAGetLastError();
					if(error != 10035) m_Log._LogWrite(L"   CLOFFRequest: connect error=%d", error);
					else
					{
						switch(WaitForSingleObject(m_hSocketEvent, 5000))
						{
							case WAIT_ABANDONED:	m_Log._LogWrite(L"   CLOFFRequest: WAIT_ABANDONED error=%d", WSAGetLastError()); break;
							case WAIT_OBJECT_0:
							{
								WSANETWORKEVENTS NetworkEvents;
								if(WSAEnumNetworkEvents(m_hSocket, m_hSocketEvent, &NetworkEvents) == SOCKET_ERROR) m_Log._LogWrite(L"   CLOFFRequest: WSAEnumNetworkEvents: error=%d", WSAGetLastError());
								else
								{
									m_Log._LogWrite(L"   CLOFFRequest: Net event = 0x%x", NetworkEvents.lNetworkEvents);
									if(NetworkEvents.lNetworkEvents & FD_CONNECT)
									{
										if(!NetworkEvents.iErrorCode[FD_CONNECT_BIT])
										{
											bConnectOk = true;
										}
										else m_Log._LogWrite(L"   CLOFFRequest: NetworkEvents.iErrorCode[FD_CONNECT_BIT] error=%i", NetworkEvents.iErrorCode[FD_CONNECT_BIT]);
									}
									if(NetworkEvents.lNetworkEvents & FD_CLOSE)
									{
										if(NetworkEvents.iErrorCode[FD_CLOSE_BIT])	m_Log._LogWrite(L"   CLOFFRequest: Ошибка connect. Ошибка: %i", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
										else m_Log._LogWrite(L"   CLOFFRequest: Close connect !");
									}
								}
								break;
							}
							case WAIT_TIMEOUT:		break;
							case WAIT_FAILED:		break;
						}

					}
				}
				if(bConnectOk)
				{
					dword dwSend{ 0 };
					WSABUF sndBuf{ 0 };
					sndBuf.len = strlen(szRequest);
					sndBuf.buf = const_cast<char*>(szRequest);
//m_Log._LogWrite(L"   CLOFFRequest: WSASend\n %S", sndBuf.buf);
					if(WSASend(m_hSocket, &sndBuf, 1, (LPDWORD)&dwSend, 0, nullptr, nullptr) != 0)	m_Log._LogWrite(L"   CLOFFRequest: WSASend error=%d", WSAGetLastError());
					else
					{
						bool bRecvEnd{ false };
						while(!bRecvEnd)
						{
							switch(WaitForSingleObject(m_hSocketEvent, 25000))
							{
								case WAIT_OBJECT_0:
								{
									WSANETWORKEVENTS NetworkEvents;
									if(WSAEnumNetworkEvents(m_hSocket, m_hSocketEvent, &NetworkEvents) == SOCKET_ERROR) m_Log._LogWrite(L"   CLOFFRequest: WSAEnumNetworkEvents: error=%d", WSAGetLastError());
									else
									{
										m_Log._LogWrite(L"   CLOFFRequest: Net event = 0x%x", NetworkEvents.lNetworkEvents);
										if(NetworkEvents.lNetworkEvents & FD_READ)//Чтение
										{
											if(!NetworkEvents.iErrorCode[FD_READ_BIT])
											{
												DWORD NumberOfBytesRecvd = 0;
												DWORD flags = 0;
												bool bDone{ false };
												while(!bDone)
												{
													NumberOfBytesRecvd = 0;
													auto recRes = WSARecv(m_hSocket, &buff, 1, &NumberOfBytesRecvd, &flags, NULL, NULL);
													if(recRes == SOCKET_ERROR)
													{
														auto error = WSAGetLastError();
														switch(error)
														{
															case WSAEWOULDBLOCK: m_Log._LogWrite(L"   CLOFFRequest: WSARecv WSAEWOULDBLOCK");  bDone = true; break; //ждём остатка
															case WSA_IO_PENDING: m_Log._LogWrite(L"   CLOFFRequest: WSARecv WSA_IO_PENDING");  break;
															default:		     m_Log._LogWrite(L"   CLOFFRequest: WSARecv error=%i", error); bDone = true; bRecvEnd = true; break;
														}
													}
													else
													{
														if(NumberOfBytesRecvd == 0)
														{
															m_Log._LogWrite(L"   CLOFFRequest: WSARecv NumberOfBytesRecvd=0");
															bDone = true;
														}
														else
														{
															strBuffer.append(cRecBuffer.data(), NumberOfBytesRecvd);
															switch(HTTPParser(strBuffer.c_str(), strAnswer))
															{
																case -1: m_Log._LogWrite(L"   CLOFFRequest: WSARecv Parser -1"); bDone = bRecvEnd = true;			break;	//ошибка при разборе - сообщение битое
																case  0: m_Log._LogWrite(L"   CLOFFRequest: WSARecv Parser  0"); bDone = bRecvEnd = bRet = true;	break;	//сообщение получено - завершаем работу
																case  1: m_Log._LogWrite(L"   CLOFFRequest: WSARecv Parser  1");									break;
															}
														}
													}
												}
											}
											else m_Log._LogWrite(L"   CLOFFRequest: NetworkEvents.iErrorCode[FD_READ_BIT] error=%i", NetworkEvents.iErrorCode[FD_READ_BIT]);
										}
										else
										{
											if(NetworkEvents.lNetworkEvents & FD_WRITE)//Запись
											{
												if(NetworkEvents.iErrorCode[FD_WRITE_BIT])	m_Log._LogWrite(L"   CLOFFRequest: NetworkEvents.iErrorCode[FD_WRITE_BIT] error=%i", NetworkEvents.iErrorCode[FD_WRITE_BIT]);
											}
										}
										if(NetworkEvents.lNetworkEvents & FD_CLOSE)//Connect
										{
											if(NetworkEvents.iErrorCode[FD_CLOSE_BIT])	m_Log._LogWrite(L"   CLOFFRequest: Ошибка connect. Ошибка: %i", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
											else m_Log._LogWrite(L"   CLOFFRequest: Close connect !\n");
										}
									}
									break;
								}
								case WAIT_TIMEOUT:	m_Log._LogWrite(L"   CLOFFRequest: WAIT_TIMEOUT");  bRecvEnd = true;	break;
								default: m_Log._LogWrite(L"   CLOFFRequest: error=%d", WSAGetLastError()); bRecvEnd = true; break;
							}
						}
					}
				}
			}
			else m_Log._LogWrite(L"   CLOFFRequest: GetAddrInfoW error=%s!", gai_strerror(gaRet));
			FreeAddrInfoW(pAddrRes);
			CloseHandle(m_hSocketEvent);
		}
	}
	if(m_hSocket != INVALID_SOCKET) closesocket(m_hSocket);

	return bRet;
}

int CCloffRequest::HTTPParser(const char* pszText,string& strJSON_Result)
{
	int iRet = -1;
	regex reAnswer("^http\\/1.1\\s200\\sOK", std::regex_constants::icase);
	regex reContentLength("^Content-Length\\s*:\\s*(\\d*)", std::regex_constants::icase);
	regex reChunk("^Transfer-Encoding\\s*:\\s*chunked", std::regex_constants::icase);

	if(regex_search(pszText, reAnswer))
	{
		auto pContent = strstr(pszText, "\r\n\r\n");
		if(pContent)
		{
			pContent += strlen("\r\n\r\n");

			cmatch reMatch;
			if(regex_search(pszText, reMatch, reContentLength, std::regex_constants::format_first_only))
			{
				auto iContentLength = atoi(reMatch[1].str().c_str());
				if((iContentLength > 0) && (strlen(pContent) == iContentLength))
				{
					strJSON_Result = pContent;
					iRet = 0;
				}
				else iRet = 1;
			}
			else if(regex_search(pszText, reChunk))
			{
				auto pNextChunk = pContent;
				auto pTextEnd = pContent + strlen(pContent);
				while(iRet == -1)
				{
					if(pNextChunk >= pTextEnd) iRet = 1;
					else
					{
						auto pTerminate = strstr(pNextChunk, "\r\n");
						if(pTerminate)
						{
							auto iChunkSize = strtol(pNextChunk, nullptr, 16);
							pNextChunk = pTerminate + strlen("\r\n");
							if(iChunkSize > 0)	pNextChunk += iChunkSize;
							else if(iChunkSize == 0)
							{
								pContent = strstr(pContent, "\r\n");
								if(pContent)
								{
									pContent += strlen("\r\n");
									strJSON_Result.append(pContent, pTerminate);
									iRet = 0;
								}
								else break;
							}	//достигли конца
						}
						else iRet = 1;
					}
				}
			}

		}
	}
	else m_Log._LogWrite(L"   PhoneBook: Answer\n%S", pszText);
	return iRet;
}

