#include "Account.h"
#include "CSIPProcess.h"
#include "Main.h"

MyAccount::MyAccount() 
{
}
MyAccount::~MyAccount()
{
	shutdown();
}

int MyAccount::_Create(const char* szNewLogin, const char* szPassword, const char* szServer, bool bDefault)
{
	int iRet = -1;

	// Configure an AccountConfig
	AccountConfig acfg;
	// Create the account
	try
	{
		if(strlen(szNewLogin) && strlen(szServer))
		{
			acfg.idUri = "sip:";
			acfg.idUri += szNewLogin;
			acfg.idUri += "@";
			acfg.idUri += szServer;
			acfg.regConfig.registrarUri = "sip:";
			acfg.regConfig.registrarUri += szServer;
			acfg.regConfig.delayBeforeRefreshSec = 10;
			acfg.regConfig.firstRetryIntervalSec = 60;
			acfg.regConfig.retryIntervalSec = 60;
			acfg.regConfig.timeoutSec = 600;

			AuthCredInfo cred("digest", "*", szNewLogin, 0, szPassword);
			acfg.sipConfig.authCreds.push_back(cred);

			SipHeader shXTS;
			shXTS.hName = "X-NV";
			shXTS.hValue = "\"CN="; shXTS.hValue += szCompName.data(); shXTS.hValue += "\"";
			acfg.regConfig.headers.push_back(shXTS);
		}
		create(acfg,bDefault);
		iRet = getId();
		m_iId = iRet;
		m_Log._LogWrite(L"Account: Account id=%d create Ok. Uri='%S' RegURI=%S", iRet, acfg.idUri.c_str(), acfg.regConfig.registrarUri.c_str());
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"Account: Account create error: %S", err.info());
	}

	return iRet;
}

bool MyAccount::_Modify(const char* szNewLogin, const char* szPassword, const char* szServer)
{
	bool bRet = false;

	// Configure an AccountConfig
	AccountConfig acfg;
	// Create the account
	try
	{
		if(strlen(szNewLogin) && strlen(szServer))
		{
			acfg.idUri = "sip:";
			acfg.idUri += szNewLogin;
			acfg.idUri += "@";
			acfg.idUri += szServer;
			acfg.regConfig.registrarUri = "sip:";
			acfg.regConfig.registrarUri += szServer;

			AuthCredInfo cred("digest", "*", szNewLogin, 0, szPassword);
			acfg.sipConfig.authCreds.push_back(cred);
		}
		modify(acfg);
		m_Log._LogWrite(L"Account: Account changed Ok. Uri='%S' RegURI=%S", acfg.idUri.c_str(), acfg.regConfig.registrarUri.c_str());
		m_bReg = false;
		bRet = true;
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"Account: Account change error: %S", err.info());
	}

	return bRet;
}

void MyAccount::onRegState(OnRegStateParam& prm)
{
	AccountInfo ai = getInfo();
	if(ai.regIsActive && prm.code == 200) 
	{
		if(!m_bReg)
		{
			m_Log._LogWrite(L"Account: id=%d Register state: %S. Code=%d. Expires=%d", m_iId,ai.regIsActive ? "'Register'" : "'Unregister'", prm.code, prm.expiration);
			m_SIPProcess->_RegisterOk(ai.id);
			m_bReg = true;
		}
	}
	else 
	{
		auto pMyData = strstr(prm.rdata.wholeMsg.c_str(), "X-NV:");
		if(pMyData)
		{
			auto pMyDataEnd = strstr(pMyData,"\r\n");
			if(pMyDataEnd)
			{
				string strX_NV(prm.reason.c_str());
				strX_NV.append(pMyData + strlen("X-NV: "), pMyDataEnd - (pMyData + strlen("X-NV: ")));
				m_Log._LogWrite(L"Account: id=%d Register state: %S. ERROR Code=%d. Reason=%S.", m_iId, ai.regIsActive ? "'Register'": "'Unregister'", prm.code, strX_NV.c_str());
				m_SIPProcess->_RegisterError(ai.id, prm.code, strX_NV.c_str());
			}
			else pMyData = nullptr;
		}
		if(!pMyData)
		{
			m_Log._LogWrite(L"Account: id=%d Register state: %S. ERROR Code=%d. Reason=%S", m_iId, ai.regIsActive ? "'Register'" : "'Unregister'", prm.code, prm.reason.c_str());
			m_SIPProcess->_RegisterError(ai.id, prm.code, prm.reason.c_str());
		}
		m_bReg = false;
	}
}
void MyAccount::onIncomingCall(OnIncomingCallParam& prm)
{
	m_Log._LogWrite(L"Account: id=%d Incoming call: %S", m_iId,prm.rdata.info.c_str());

	if(!m_call) 
	{
		m_call = make_unique<MyCall>(*this, prm.callId);

		CallOpParam c_prm;

		c_prm.statusCode = PJSIP_SC_RINGING;
		SipHeader shXTS;
		shXTS.hName = "X-NV-Timestamp";

		SYSTEMTIME t;
		GetLocalTime(&t);
		array<char, 256> szData;
		sprintf_s(szData.data(), szData.size(), "\"%04d-%02d-%02d %02d:%02d:%02d.%03d\"", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
		shXTS.hValue = szData.data();
		c_prm.txOption.headers.push_back(shXTS);

		m_call->answer(c_prm);
	}
	else
	{
		auto newcall = make_unique<MyCall>(*this, prm.callId);

		CallOpParam c_prm;
		c_prm.statusCode = PJSIP_SC_BUSY_HERE;
		m_call->hangup(c_prm);
	}

}

bool MyAccount::_Answer()
{
	bool bRet = true;
	CallOpParam c_prm;
	c_prm.statusCode = PJSIP_SC_OK;

	SipHeader shXTS;
	shXTS.hName = "X-NV-Timestamp";

	SYSTEMTIME t;
	GetLocalTime(&t);
	array<char, 256> szData;
	sprintf_s(szData.data(), szData.size(), "\"%04d-%02d-%02d %02d:%02d:%02d.%03d\"", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	shXTS.hValue = szData.data();
	c_prm.txOption.headers.push_back(shXTS);

	if(m_call) m_call->answer(c_prm);
	else bRet = false;

	return bRet;
}

bool MyAccount::_Disconnect()
{
	bool bRet = true;
	if(m_call)	bRet = m_call->_Disconnect();
	else		bRet = false;

	return bRet;
}

bool MyAccount::_DeleteCall()
{
	bool bRet = true;
	if(m_call)
	{
		m_Log._LogWrite(L"Account: id=%d Delete call.", m_iId);
		m_call.reset(nullptr);
	}
	else bRet = false;

	return bRet;
}

bool MyAccount::_MakeCall(const char* szNumber)
{
	bool bRet = false;

	if(!m_call)
	{
		m_call = make_unique<MyCall>(*this);
		CallOpParam prm(true); // Use default call settings
		try
		{
			SipHeader shXTS;
			shXTS.hName = "X-NV-Timestamp";

			SYSTEMTIME t;
			GetLocalTime(&t);
			array<char, 256> szData;
			sprintf_s(szData.data(), szData.size(), "\"%04d-%02d-%02d %02d:%02d:%02d.%03d\"", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
			shXTS.hValue = szData.data();
			prm.txOption.headers.push_back(shXTS);

			m_call->makeCall(szNumber, prm);
			bRet = true;
		}
		catch(Error& err)
		{
			m_Log._LogWrite(L"Account: id=%d Make call error: %S", m_iId, err.info().c_str());
			PlaySound(L"BUSY", NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			/* Schedule/Dispatch call deletion to another thread here */
			string strErr;
			auto iBegin = err.info().find("error:");
			if(iBegin)
			{
				auto iEnd = err.info().find("[");
				if(iEnd != string::npos) strErr.append(&err.info()[iBegin],iEnd - iBegin);
				else strErr.append(&err.info()[iBegin]);
			}
			else strErr = err.info();
			m_SIPProcess->_DisconnectRemote(m_call->_GetAccountId(), 0, strErr.c_str());
			m_call.reset(nullptr);
		}
		catch(...)
		{
			m_Log._LogWrite(L"Account: id=%d Make call error!", m_iId);
			PlaySound(L"BUSY", NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			/* Schedule/Dispatch call deletion to another thread here */
//			m_SIPProcess->_DisconnectRemote(0, "Ошибка инициализации вызова!");
			m_call.reset(nullptr);
		}
	}
	else bRet = false;
	return bRet;
}

bool MyAccount::_DTMF(const char cDigit, bool bRFC_2833)
{
	bool bRet = false;
	if(m_call)
	{
		try
		{
			CallSendDtmfParam prm;
			prm.digits = cDigit;
			if(bRFC_2833) prm.method = PJSUA_DTMF_METHOD_RFC2833;
			else prm.method = PJSUA_DTMF_METHOD_SIP_INFO;
			prm.duration = PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT;
			m_call->sendDtmf(prm);
			m_Log._LogWrite(L"Account: id=%d DTMF send: %c", m_iId, cDigit);
			bRet = true;
		}
		catch(Error& err)
		{
			m_Log._LogWrite(L"Account: id=%d DTMF send error: %S", m_iId, err.info().c_str());
		}
	}

	return bRet;
}

void MyAccount::_Microfon(DWORD dwLevel)
{
	if(m_call) m_call->_Microfon(dwLevel);
}
void MyAccount::_Sound(DWORD dwLevel)
{
	if(m_call) m_call->_Sound(dwLevel);
}