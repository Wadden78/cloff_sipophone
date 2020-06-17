#pragma once

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include <tchar.h>
#include <process.h>
#include <delayimp.h>
#include <stdlib.H>
#include <signal.h>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <mutex>
#include <pjsua2.hpp>
#include "Call.h"

using namespace pj;
using namespace std;

// Subclass to extend the Account and get notifications etc.
class MyAccount : public Account
{
public:
	MyAccount();
	virtual ~MyAccount();
	bool _Create(const char* szNewLogin, const char* szPassword, const char* szServer);
	bool _Modify(const char* szNewLogin, const char* szPassword, const char* szServer);
	bool _Answer();
	bool _Disconnect();
	bool _DeleteCall();
	bool _MakeCall(const char* szNumber);
	bool _DTMF(const char cDigit, bool bRFC_2833);
	void _Microfon(bool bOn);

	virtual void onRegState(OnRegStateParam& prm) final;
	virtual void onIncomingCall(OnIncomingCallParam& prm) final;
	//virtual void onInstantMessage(OnIncoming& prm) final;
	//onInstantMessageStatus()
private:
	std::unique_ptr<MyCall> m_call;
	bool m_bReg = false;
};
