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

using namespace pj;
using namespace std;

class MyCall : public Call
{
public:
	MyCall(Account& acc, int call_id = PJSUA_INVALID_ID);
	virtual ~MyCall();

	void _Microfon(DWORD dwLevel);
	void _Sound(DWORD dwLevel);
	bool _Disconnect();

	// Notification when call's state has changed.
	virtual void onCallState(OnCallStateParam& prm) final;
	// Notification when call's media state has changed.
	virtual void onCallMediaState(OnCallMediaStateParam& prm) final;
private:
};

