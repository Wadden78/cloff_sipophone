#include <winsock2.h>
#include <Windows.h>
#include <Windowsx.h>
#include <conio.h>
#include <string>
#include "Main.h"
#include "History.h"

wchar_t default_key[] = L"3igcZhRdWq01M3G4mTAiv9";

void ReadOldConfig(vector<BYTE>& vData)
{
	wchar_t* key_str = default_key;
	DWORD dwStatus = 0;
	wchar_t info[] = L"Microsoft Enhanced RSA and AES Cryptographic Provider";
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HCRYPTKEY hKey;
	array<wchar_t, 256> wszMessage;

	if(!CryptAcquireContextW(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
	{
		dwStatus = GetLastError();
		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptAcquireContext failed: %x", dwStatus);
		SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
	}
	else
	{
		if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
		{
			dwStatus = GetLastError();
			swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptCreateHash failed: %x", dwStatus);
			SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
		}
		else
		{
			if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenW(key_str), 0))
			{
				DWORD err = GetLastError();
				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptHashData Failed : %#x", err);
				SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
			}
			else
			{
				if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
				{
					dwStatus = GetLastError();
					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptDeriveKey failed: %x", dwStatus);
					SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
				}
				else
				{
					DWORD out_len = vData.size();
					if(!CryptDecrypt(hKey, NULL, TRUE, 0, vData.data(), &out_len))
					{
						dwStatus = GetLastError();
						switch(dwStatus)
						{
							case ERROR_INVALID_HANDLE:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		break;
							case ERROR_INVALID_PARAMETER:	swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	break;
							case NTE_BAD_ALGID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_ALGID");				break;
							case NTE_BAD_DATA:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_DATA");				break;
							case NTE_BAD_FLAGS:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_FLAGS");				break;
							case NTE_BAD_HASH:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_HASH");				break;
							case NTE_BAD_KEY:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_KEY");				break;
							case NTE_BAD_LEN:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_LEN");				break;
							case NTE_BAD_UID:				swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_BAD_UID");				break;
							case NTE_DOUBLE_ENCRYPT:		swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");		break;
							case NTE_FAIL:					swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=NTE_FAIL");					break;
							default:						swprintf_s(wszMessage.data(), wszMessage.size(), L"CryptEncrypt failed err=%d(%x)", dwStatus, dwStatus); break;
						}
						SetDlgItemText(hDlgPhoneWnd, IDC_STATIC_REGSTATUS, wszMessage.data());
					}
					else
					{
						auto bPtr = vData.data();
						RECT sCoord{ 0 };
						sCoord.left = *reinterpret_cast<LONG*>(bPtr);
						bPtr += sizeof(sCoord.left);
						sCoord.right = *reinterpret_cast<LONG*>(bPtr);
						bPtr += sizeof(sCoord.right);
						sCoord.top = *reinterpret_cast<LONG*>(bPtr);
						bPtr += sizeof(sCoord.top);
						sCoord.bottom = *reinterpret_cast<LONG*>(bPtr);
						bPtr += sizeof(sCoord.bottom);
						SetWindowPos(hDlgPhoneWnd, HWND_TOP, sCoord.left, sCoord.top, 344/*sCoord.right - sCoord.left*/, 431/*sCoord.bottom - sCoord.top*/, SWP_SHOWWINDOW);

						auto bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', vData.size()));
						if(bParameterEnd)
						{
							//CompName
							string strCN;
							strCN.append(reinterpret_cast<char*>(bPtr), bParameterEnd - bPtr);
							bPtr = bParameterEnd + sizeof('\n');

							if(!strCN.compare(szCompName.data()))
							{
								bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', vData.size()));
								if(bParameterEnd)
								{
									//Login
									wstrLogin.clear();
									wstrLogin.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
									bPtr = bParameterEnd + sizeof('\n');

									bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', vData.size()));
									if(bParameterEnd)
									{
										//Password
										wstrPassword.clear();
										wstrPassword.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
										bPtr = bParameterEnd + sizeof('\n');

										bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', vData.size()));
										if(bParameterEnd)
										{
											//Server
											wstrServer.clear();
											wstrServer.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
											bPtr = bParameterEnd + sizeof('\n');
											if(DWORD(bPtr - vData.data()) < out_len)
											{
												//Transport
												iTransportType = *reinterpret_cast<int*>(bPtr);
												if(iTransportType != IPPROTO_TCP && iTransportType != IPPROTO_UDP) iTransportType = IPPROTO_TCP;
												bPtr += sizeof(iTransportType);
												if(DWORD(bPtr - vData.data()) < out_len)
												{
													//DTMF
													bDTMF_2833 = *reinterpret_cast<bool*>(bPtr);
													bPtr += sizeof(bDTMF_2833);
													if(DWORD(bPtr - vData.data()) < out_len)
													{
														//Ring
														uiRingToneNumber = *reinterpret_cast<uint16_t*>(bPtr);
														if(uiRingToneNumber >= 3) uiRingToneNumber = 0;
														bPtr += sizeof(uiRingToneNumber);
														//Microfon level
														if(DWORD(bPtr - vData.data()) < out_len)
														{
															dwMicLevel = *reinterpret_cast<DWORD*>(bPtr);
															if(dwMicLevel > ci_LevelMAX) dwMicLevel = ci_LevelMAX;
															bPtr += sizeof(dwMicLevel);
															//Sound level
															if(DWORD(bPtr - vData.data()) < out_len)
															{
																dwSndLevel = *reinterpret_cast<DWORD*>(bPtr);
																if(dwSndLevel > ci_LevelMAX) dwSndLevel = ci_LevelMAX;
																bPtr += sizeof(dwSndLevel);

																wstring wstrCalledPNUM;
																while(DWORD(bPtr - vData.data()) < out_len)
																{
																	auto bParameterEnd = reinterpret_cast<BYTE*>(memchr(bPtr, '\n', vData.size()));
																	if(bParameterEnd)
																	{
																		wstrCalledPNUM.clear();
																		wstrCalledPNUM.append(reinterpret_cast<wchar_t*>(bPtr), (bParameterEnd - bPtr) / sizeof(wchar_t));
																		bPtr = bParameterEnd + sizeof('\n');
																		SendMessage(hComboBox, (UINT)CB_INSERTSTRING, (WPARAM)0, (LPARAM)wstrCalledPNUM.c_str());
																	}
																	else break;
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
					CryptDestroyKey(hKey);
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
}

bool LoadConfig(HWND hWnd)
{
	bool bRet = false;
	vector<char> vStringBuffer;
	vStringBuffer.resize(2048);

	FILE* fInit = nullptr;
	fopen_s(&fInit, "cloff_sip_phone.cfg", "rb");
	if(!fInit)
	{
		if(errno != ENOENT)
		{
			std::array<wchar_t, 256> chMsg;
			swprintf_s(chMsg.data(), chMsg.size(), L"Config file open error %u(%S)", errno, strerror(errno));
			SetDlgItemText(hWnd, IDC_STATIC_REGSTATUS, chMsg.data());
		}
	}
	else
	{
		fseek(fInit, 0, SEEK_SET);
		fread(vStringBuffer.data(), sizeof(char), strlen("[MAIN]"), fInit);
		if(memcmp(vStringBuffer.data(), "[MAIN]", strlen("[MAIN]")) != 0)
		{
			vector<BYTE> vData;
			fseek(fInit, 0, SEEK_END);
			auto stFLen = ftell(fInit);
			vData.resize(stFLen);
			fseek(fInit, 0, SEEK_SET);
			fread(vData.data(), sizeof(char), vData.size(), fInit);
			ReadOldConfig(vData);
			bRet = true;
		}
		fclose(fInit);
	}
	return bRet;
}


#include "Config.h"
char szDefault_key[] = "3igcZhRdWq01M3G4mTAiv9";

CConfigFile::CConfigFile(bool bWrite)
{
	m_bWriteOnly = bWrite;
	switch(m_bWriteOnly)
	{
		case false: m_fCfgFile = fopen("cloff_sip_phone.cfg", "r"); break;
		case true:  m_fCfgFile = fopen("cloff_sip_phone.cfg", "w"); break;
	}
	
	if(m_fCfgFile == NULL) m_Log._LogWrite(L"CFG: ошибка открытия файла конфигурации config.cfg. error=%d", errno);
}

CConfigFile::~CConfigFile(void)
{
	if(m_fCfgFile) fclose(m_fCfgFile);
}

//Получить строковый параметр
size_t CConfigFile::GetStringParameter(const char* szSection, const char* szParameter, std::wstring& wstrResult)
{
	if(m_bWriteOnly || (m_fCfgFile == NULL)) return 0;

	array<char, 256> szFormat;
	wstrResult.clear();

	sprintf_s(szFormat.data(), szFormat.size(), "^\\[%s\\]", szSection);
	regex reSection(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^%s=", szParameter);
	regex reParam(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^\\[\\S*\\]");
	regex reNextSection(szFormat.data(), std::regex_constants::icase);

	fseek(m_fCfgFile, 0, SEEK_SET);
	vector<char> szBuffer(2048);

	bool bInSection = false;
	while(fgets(szBuffer.data(), szBuffer.size(), m_fCfgFile))
	{
		if(bInSection)
		{
			if(regex_search(szBuffer.data(), reNextSection)) break;	//дошли до следующей секции - параметр не нашли
			if(regex_search(szBuffer.data(), reParam))
			{
				char* pLF = strchr(szBuffer.data(), '\n');
				if(pLF) *pLF = 0;
				char* pCR = strchr(szBuffer.data(), '\r');
				if(pCR) *pCR = 0;
				auto pParBegin = strchr(szBuffer.data(), '=') + 1;
				if(pParBegin)
				{
					vector<wchar_t> vCN(strlen(pParBegin) + 1);
					MultiByteToWideChar(1251, MB_PRECOMPOSED, pParBegin, strlen(pParBegin), vCN.data(), vCN.size());
					vCN[vCN.size()-1] = 0;
					wstrResult = vCN.data();
				}
				break;
			}
		}
		if(!bInSection && regex_search(szBuffer.data(), reSection)) bInSection = true; // нашли секцию
	}
	return wstrResult.length();
}

//Получить int параметр
int CConfigFile::GetIntParameter(const char* szSection, const char* szParameter, int iDefault)
{
	if(m_bWriteOnly || (m_fCfgFile == NULL)) return iDefault;

	int iRet = iDefault;
	array<char, 256> szFormat;
	if(m_fCfgFile == NULL) return iRet;

	sprintf_s(szFormat.data(), szFormat.size(), "^\\[%s\\]", szSection);
	regex reSection(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^%s=", szParameter);
	regex reParam(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^\\[\\S*\\]");
	regex reNextSection(szFormat.data(), std::regex_constants::icase);

	fseek(m_fCfgFile, 0, SEEK_SET);
	array<char, 1024> szBuffer;
	bool bInSection = false;
	while(fgets(szBuffer.data(), szBuffer.size(), m_fCfgFile))
	{
		if(bInSection)
		{
			if(regex_search(szBuffer.data(), reNextSection)) break;	//дошли до следующей секции - параметр не нашли
			if(regex_search(szBuffer.data(), reParam))
			{
				iRet = atoi(strchr(szBuffer.data(), '=') + 1);
				break;
			}
		}
		if(!bInSection && regex_search(szBuffer.data(), reSection)) bInSection = true; // нашли секцию
	}
	return iRet;
}

size_t CConfigFile::GetCodedStringParameter(const char* szSection, const char* szParameter, wstring& wstrResult)
{
	if(m_bWriteOnly || (m_fCfgFile == NULL)) return 0;

	array<char, 256> szFormat;
	wstrResult.clear();

	sprintf_s(szFormat.data(), szFormat.size(), "^\\[%s\\]", szSection);
	regex reSection(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^%s=", szParameter);
	regex reParam(szFormat.data(), std::regex_constants::icase);
	sprintf_s(szFormat.data(), szFormat.size(), "^\\[\\S*\\]");
	regex reNextSection(szFormat.data(), std::regex_constants::icase);

	fseek(m_fCfgFile, 0, SEEK_SET);
	vector<char> szBuffer(2048);

	bool bInSection = false;
	while(fgets(szBuffer.data(), szBuffer.size(), m_fCfgFile))
	{
		if(bInSection)
		{
			if(regex_search(szBuffer.data(), reNextSection)) break;	//дошли до следующей секции - параметр не нашли
			if(regex_search(szBuffer.data(), reParam))
			{
				char* pLF = strchr(szBuffer.data(), '\n');
				if(pLF) *pLF = 0;
				char* pCR = strchr(szBuffer.data(), '\r');
				if(pCR) *pCR = 0;
				auto pParBegin = strchr(szBuffer.data(), '=') + 1;
				if(pParBegin) 
				{
					vector<BYTE> vData;
					auto len = strlen(pParBegin);
					BYTE bVar{ 0 };
					for(UINT i = 0; i < len; i+=2)
					{
						unsigned int uiVar{ 0 };
						auto iRes = sscanf(&pParBegin[i], "%02x", &uiVar);
						vData.push_back(uiVar);
					}

					char* key_str = szDefault_key;
					char info[] = "Microsoft Enhanced RSA and AES Cryptographic Provider";
					HCRYPTPROV hProv;
					HCRYPTHASH hHash;
					HCRYPTKEY hKey;

					if(!CryptAcquireContextA(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) m_Log._LogWrite(L"CryptAcquireContext failed: %#x", GetLastError());
					else
					{
						if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))	m_Log._LogWrite(L"CryptCreateHash failed: %#x", GetLastError());
						else
						{
							if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenA(key_str), 0))	m_Log._LogWrite(L"CryptHashData Failed : %#x", GetLastError());
							else
							{
								if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) m_Log._LogWrite(L"CryptDeriveKey failed: %#x", GetLastError());
								else
								{
									DWORD out_len = vData.size();
									if(!CryptDecrypt(hKey, NULL, TRUE, 0, vData.data(), &out_len))
									{
										auto dwStatus = GetLastError();
										switch(dwStatus)
										{
											case ERROR_INVALID_HANDLE:		m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		break;
											case ERROR_INVALID_PARAMETER:	m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	break;
											case NTE_BAD_ALGID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_ALGID");				break;
											case NTE_BAD_DATA:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_DATA");				break;
											case NTE_BAD_FLAGS:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_FLAGS");				break;
											case NTE_BAD_HASH:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_HASH");				break;
											case NTE_BAD_KEY:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_KEY");				break;
											case NTE_BAD_LEN:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_LEN");				break;
											case NTE_BAD_UID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_UID");				break;
											case NTE_DOUBLE_ENCRYPT:		m_Log._LogWrite(L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");			break;
											case NTE_FAIL:					m_Log._LogWrite(L"CryptEncrypt failed err=NTE_FAIL");					break;
											default:						m_Log._LogWrite(L"CryptEncrypt failed err=%d(%x)", dwStatus, dwStatus); break;
										}
									}
									else
									{
										wstrResult.clear();
										wstrResult.append(reinterpret_cast<wchar_t*>(vData.data()), out_len / sizeof(wchar_t));
									}
									CryptDestroyKey(hKey);
								}
							}
							CryptDestroyHash(hHash);
						}
						CryptReleaseContext(hProv, 0);
					}
				}
				break;
			}
		}
		if(!bInSection && regex_search(szBuffer.data(), reSection)) bInSection = true; // нашли секцию
	}
	return wstrResult.length();
}

bool CConfigFile::PutStringParameter(const char* szParameter, const wchar_t* wszValue, const char* szSection)
{
	if(!m_bWriteOnly || (m_fCfgFile == NULL)) return false;

	if(szSection) fprintf(m_fCfgFile, "[%s]\n", szSection);
	fprintf(m_fCfgFile, "%s=%ls\n", szParameter, wszValue);

	return true;
}

bool CConfigFile::PutIntParameter(const char* szParameter, const int iValue, const char* szSection)
{
	if(!m_bWriteOnly || (m_fCfgFile == NULL)) return false;

	if(szSection) fprintf(m_fCfgFile, "[%s]\n", szSection);
	fprintf(m_fCfgFile, "%s=%d\n", szParameter, iValue);

	return true;
}

bool CConfigFile::PutCodedStringParameter(const char* szParameter, const wchar_t* wszValue, const char* szSection)
{
	if(!m_bWriteOnly || (m_fCfgFile == NULL)) return false;

	if(szSection) fprintf(m_fCfgFile, "[%s]\n", szSection);

	size_t stCount = wcslen(wszValue) * sizeof(wchar_t);
	vector<BYTE> szData((stCount / CHUNK_SIZE + 1) * CHUNK_SIZE);
	//CompName
	memcpy_s(szData.data(), szData.size(), wszValue, stCount);

	char* key_str = szDefault_key;
	char info[] = "Microsoft Enhanced RSA and AES Cryptographic Provider";
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HCRYPTKEY hKey;

	if(!CryptAcquireContextA(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) m_Log._LogWrite(L"CryptAcquireContext failed: %#x", GetLastError());
	else
	{
		if(!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))	m_Log._LogWrite(L"CryptCreateHash failed: %#x", GetLastError());
		else
		{
			if(!CryptHashData(hHash, (BYTE*)key_str, lstrlenA(key_str), 0))	m_Log._LogWrite(L"CryptHashData Failed : %#x", GetLastError());
			else
			{
				if(!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey)) m_Log._LogWrite(L"CryptDeriveKey failed: %#x", GetLastError());
				else
				{
					DWORD out_len = stCount;
					if(!CryptEncrypt(hKey, NULL, TRUE, 0, szData.data(), &out_len, szData.size()))
					{
						auto dwStatus = GetLastError();
						switch(dwStatus)
						{
							case ERROR_INVALID_HANDLE:		m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_HANDLE");		 break;
							case ERROR_INVALID_PARAMETER:	m_Log._LogWrite(L"CryptEncrypt failed err=ERROR_INVALID_PARAMETER");	 break;
							case NTE_BAD_ALGID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_ALGID");				 break;
							case NTE_BAD_DATA:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_DATA");				 break;
							case NTE_BAD_FLAGS:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_FLAGS");				 break;
							case NTE_BAD_HASH:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_HASH");				 break;
							case NTE_BAD_KEY:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_KEY");				 break;
							case NTE_BAD_LEN:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_LEN");				 break;
							case NTE_BAD_UID:				m_Log._LogWrite(L"CryptEncrypt failed err=NTE_BAD_UID");				 break;
							case NTE_DOUBLE_ENCRYPT:		m_Log._LogWrite(L"CryptEncrypt failed err=NTE_DOUBLE_ENCRYPT");			 break;
							case NTE_FAIL:					m_Log._LogWrite(L"CryptEncrypt failed err=NTE_FAIL");					 break;
							default:						m_Log._LogWrite(L"CryptEncrypt failed err=%d(%#x)", dwStatus, dwStatus); break;
						}
					}
					else
					{
						fprintf(m_fCfgFile, "%s=", szParameter);
						for(DWORD i = 0; i < out_len; ++i) fprintf(m_fCfgFile, "%02x", szData[i]);
						fprintf(m_fCfgFile, "\n");
					}
					CryptDestroyKey(hKey);
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return true;
}