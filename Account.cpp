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

bool MyAccount::_Create(const char* szNewLogin, const char* szPassword, const char* szServer)
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

			array<char, MAX_COMPUTERNAME_LENGTH + 1> szCompName;
			DWORD dwLen = MAX_COMPUTERNAME_LENGTH;
			SipHeader shXTS;
			if(GetComputerNameA((LPSTR)szCompName.data(), &dwLen))
			{
				shXTS.hName = "X-NV";
				shXTS.hValue = "\"CN="; shXTS.hValue += szCompName.data(); shXTS.hValue += "\"";
				acfg.regConfig.headers.push_back(shXTS);
			}
		}
		create(acfg);
		m_SIPProcess->_LogWrite(L"Account: Account create Ok. Uri='%S' RegURI=%S", acfg.idUri.c_str(), acfg.regConfig.registrarUri.c_str());
		bRet = true;
	}
	catch(Error& err)
	{
		m_SIPProcess->_LogWrite(L"Account: Account create error: %S", err.info());
	}

	return bRet;
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
		m_SIPProcess->_LogWrite(L"Account: Account changed Ok. Uri='%S' RegURI=%S", acfg.idUri.c_str(), acfg.regConfig.registrarUri.c_str());
		m_bReg = false;
		bRet = true;
	}
	catch(Error& err)
	{
		m_SIPProcess->_LogWrite(L"Account: Account change error: %S", err.info());
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
			m_SIPProcess->_LogWrite(L"Account: Register state: %S. Code=%d. Expires=%d", ai.regIsActive ? "*** Register:" : "*** Unregister:", prm.code, prm.expiration);
			m_SIPProcess->_RegisterOk();
			m_bReg = true;
		}
	}
	else 
	{
		m_SIPProcess->_LogWrite(L"Account: Register state: %S. ERROR Code=%d. Reason=%S", ai.regIsActive ? "*** Register:" : "*** Unregister:", prm.code, prm.reason.c_str());
		m_SIPProcess->_RegisterError(prm.code, prm.reason.c_str());
		m_bReg = false;
	}
}
void MyAccount::onIncomingCall(OnIncomingCallParam& prm)
{
	m_SIPProcess->_LogWrite(L"Account: Incoming call: %S", prm.rdata.info.c_str());

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
	if(m_call) bRet = m_call->_Disconnect();
	else bRet = false;

	return bRet;
}

bool MyAccount::_DeleteCall()
{
	bool bRet = true;
	if(m_call)
	{
		m_SIPProcess->_LogWrite(L"Account: Delete call.");
		m_call.reset(nullptr);
	}
	else bRet = false;

	return bRet;
}

bool MyAccount::_MakeCall(const char* szNumber)
{
	bool bRet = true;

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
	}
	catch(Error& err)
	{
		m_SIPProcess->_LogWrite(L"Account: Make call error: %S", err.info().c_str());
	}
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
			m_SIPProcess->_LogWrite(L"Account: DTMF send: %c", cDigit);
			bRet = true;
		}
		catch(Error& err)
		{
			m_SIPProcess->_LogWrite(L"Account: DTMF send error: %S", err.info().c_str());
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