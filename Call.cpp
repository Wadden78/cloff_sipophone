#include "Call.h"
#include "CSIPProcess.h"
#include "Main.h"


MyCall::MyCall(Account& acc, int call_id) : Call(acc, call_id)
{
	m_iAccountId = acc.getId();
	m_Log._LogWrite(L"   Call: acc_id=%d constructor id=%d", m_iAccountId,call_id);
}

MyCall::~MyCall()
{
	m_Log._LogWrite(L"   Call: acc_id=%d destructor.", m_iAccountId);
}

int MyCall::_GetAccountId()
{
	return m_iAccountId;
}

void MyCall::_Microfon(DWORD dwLevel)
{
	try
	{
		AudDevManager& mgr = Endpoint::instance().audDevManager();
		mgr.getCaptureDevMedia().adjustTxLevel(((float)dwLevel)/100);
		m_Log._LogWrite(L"   Call: microfon level set to=%u%%", dwLevel);
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   Call: microfon control error=%S", err.info().c_str());
	}
}

void MyCall::_Sound(DWORD dwLevel)
{
	try
	{
		AudDevManager& mgr = Endpoint::instance().audDevManager();
		mgr.getCaptureDevMedia().adjustRxLevel(((float)dwLevel) / 100);
		m_Log._LogWrite(L"   Call: sound level set to=%u%%", dwLevel);
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   Call: sound control error=%S", err.info().c_str());
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
			case PJSIP_INV_STATE_CONFIRMED:
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
				m_Log._LogWrite(L"   Call: acc_id=%d Disconnect call from local side. State=%d", m_iAccountId, ci.state);
				c_prm.statusCode = PJSIP_SC_BUSY_HERE;
				bRet = true;
				break;
			}
		}
		hangup(c_prm);
		m_Log._LogWrite(L"   Call: acc_id=%d Disconnect call from local side.", m_iAccountId);
	}
	catch(Error& err)
	{
		m_Log._LogWrite(L"   Call: acc_id=%d Disconnect send error=%S", m_iAccountId, err.info().c_str());
	}

	return bRet;
}

 // Notification when call's state has changed.
void MyCall::onCallState(OnCallStateParam& prm)
{
	CallInfo ci = getInfo();
	m_Log._LogWrite(L"   Call: acc_id=%d Call-ID:'%S' State=%S", m_iAccountId, ci.callIdString.c_str(), ci.stateText.c_str());
	switch(ci.state)
	{
		case PJSIP_INV_STATE_DISCONNECTED:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Disconnect call from remote side. Reason=%S(%d)", m_iAccountId, ci.lastReason.c_str(),ci.lastStatusCode);
			/* Schedule/Dispatch call deletion to another thread here */
			string strDump = dump(true, "                                ");
			m_Log._LogWrite(L"   Call: acc_id=%d stat=%S", m_iAccountId, strDump.c_str());
			m_SIPProcess->_DisconnectRemote(ci.accId,ci.lastStatusCode, ci.lastReason.c_str());
			break;
		}
		case PJSIP_INV_STATE_CONFIRMED:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Confirmed state.", m_iAccountId);
			break;
		}
		case PJSIP_INV_STATE_CALLING:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Calling state.", m_iAccountId);
			m_SIPProcess->_Alerting(ci.accId);
			break;
		}
		case PJSIP_INV_STATE_CONNECTING:
		{
			PlaySound(NULL, NULL, 0);
			m_Log._LogWrite(L"   Call: acc_id=%d Call connect.", m_iAccountId);
			m_SIPProcess->_Connected(ci.accId);
			break;
		}
		case PJSIP_INV_STATE_INCOMING:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Incoming Call state. Remote='%S'", m_iAccountId, ci.remoteUri.c_str());
			m_SIPProcess->_IncomingCall(ci.accId, ci.remoteUri.c_str());
			bIncoming = true;
			break;
		}
		case PJSIP_INV_STATE_EARLY:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Early Call state.", m_iAccountId);
			if(!bIncoming) PlaySound(L"KPV", NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			break;
		}
		default:
		{
			m_Log._LogWrite(L"   Call: acc_id=%d Invalid Call state.", m_iAccountId);
			break;
		}
	}
}

// Notification when call's media state has changed.
void MyCall::onCallMediaState(OnCallMediaStateParam& prm)
{
	CallInfo ci = getInfo();
	m_Log._LogWrite(L"   Call: acc_id=%d CallMediaState", m_iAccountId);
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
			mgr.getCaptureDevMedia().adjustTxLevel(((float)dwMicLevel) / 100);
			mgr.getCaptureDevMedia().adjustRxLevel(((float)dwSndLevel) / 100);
		}
	}
}

