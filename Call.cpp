#include "Call.h"
#include "CSIPProcess.h"
#include "Main.h"


MyCall::MyCall(Account& acc, int call_id) : Call(acc, call_id)
{
	m_SIPProcess->_LogWrite(L"   Call: constructor id=%d", call_id);
}

MyCall::~MyCall()
{
	m_SIPProcess->_LogWrite(L"   Call: destructor.");
}

void MyCall::_Microfon(bool bOn)
{
	try
	{
		AudDevManager& mgr = Endpoint::instance().audDevManager();
		mgr.getCaptureDevMedia().adjustTxLevel(bOn ? (float)1.0 : (float)0);
		m_SIPProcess->_LogWrite(L"   Call: microfon %s", bOn ? L"ON" : L"OFF");
	}
	catch(Error& err)
	{
		m_SIPProcess->_LogWrite(L"   Call: microfon control error=%S", err.info().c_str());
	}
}

bool MyCall::_Disconnect()
{
	bool bRet = false;
	CallInfo ci = getInfo();

	CallOpParam c_prm;
	SipHeader shXTS;
	shXTS.hName = "X-NV-Timestamp";

	try
	{
		SYSTEMTIME t;
		GetLocalTime(&t);
		array<char, 256> szData;
		sprintf_s(szData.data(), szData.size(), "\"%04d-%02d-%02d %02d:%02d:%02d.%03d\"", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
		shXTS.hValue = szData.data();
		c_prm.txOption.headers.push_back(shXTS);

		switch(ci.state)
		{
			case PJSIP_INV_STATE_CONNECTING:
			{
				c_prm.statusCode = PJSIP_SC_OK;
				bRet = true;
				break;
			}
			case PJSIP_INV_STATE_INCOMING:
			case PJSIP_INV_STATE_EARLY:
			{
				c_prm.statusCode = PJSIP_SC_FORBIDDEN;
				bRet = true;
				break;
			}
			default:
			{
				c_prm.statusCode = PJSIP_SC_BUSY_HERE;
				bRet = true;
				break;
			}
		}
		hangup(c_prm);
		m_SIPProcess->_LogWrite(L"   Call: Disconnect call from local side.");
	}
	catch(Error& err)
	{
		m_SIPProcess->_LogWrite(L"   Call: Disconnect send error=%S", err.info().c_str());
	}

	return bRet;
}

 // Notification when call's state has changed.
void MyCall::onCallState(OnCallStateParam& prm)
{
	CallInfo ci = getInfo();
	m_SIPProcess->_LogWrite(L"   Call: Call-ID:'%S' State=%S", ci.callIdString.c_str(), ci.stateText.c_str());
	switch(ci.state)
	{
		case PJSIP_INV_STATE_DISCONNECTED:
		{
			PlaySound(L"BUSY", NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			m_SIPProcess->_LogWrite(L"   Call: Disconnect call from remote side. Reason=%S", ci.lastReason.c_str());
			/* Schedule/Dispatch call deletion to another thread here */
			string strDump = dump(true, "                                ");
			m_SIPProcess->_LogWrite(L"   Call: stat=%S", strDump.c_str());
			m_SIPProcess->_DisconnectRemote(ci.lastReason.c_str());
			break;
		}
		case PJSIP_INV_STATE_CONFIRMED:
		{
			m_SIPProcess->_LogWrite(L"   Call: Confirmed state.");
			break;
		}
		case PJSIP_INV_STATE_CALLING:
		{
			m_SIPProcess->_LogWrite(L"   Call: Calling state.");
			m_SIPProcess->_Alerting();
			break;
		}
		case PJSIP_INV_STATE_CONNECTING:
		{
			PlaySound(NULL, NULL, 0);
			m_SIPProcess->_LogWrite(L"   Call: Call connect.");
			m_SIPProcess->_Connected();
			break;
		}
		case PJSIP_INV_STATE_INCOMING:
		{
			m_SIPProcess->_LogWrite(L"   Call: Incoming Call state. Remote='%S'", ci.remoteUri.c_str());
			m_SIPProcess->_IncomingCall(ci.remoteUri.c_str());
			break;
		}
		case PJSIP_INV_STATE_EARLY:
		{
			m_SIPProcess->_LogWrite(L"   Call: Early Call state.");
			PlaySound(L"KPV", NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			break;
		}
		default:
		{
			m_SIPProcess->_LogWrite(L"   Call: Invalid Call state.");
			break;
		}
	}
}

// Notification when call's media state has changed.
void MyCall::onCallMediaState(OnCallMediaStateParam& prm)
{
	CallInfo ci = getInfo();
	m_SIPProcess->_LogWrite(L"   Call: CallMediaState");
	PlaySound(NULL, NULL, 0);
// Iterate all the call medias
	for(unsigned i = 0; i < ci.media.size(); i++)
	{
		if(ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i))
		{
			AudioMedia* aud_med = (AudioMedia*)getMedia(i);

			// Connect the call audio media to sound device
			AudDevManager& mgr = Endpoint::instance().audDevManager();
			aud_med->startTransmit(mgr.getPlaybackDevMedia());
			mgr.getCaptureDevMedia().startTransmit(*aud_med);
		}
	}
}

