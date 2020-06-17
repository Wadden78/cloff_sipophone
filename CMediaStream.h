#pragma once

#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>
#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjsua-lib/pjsua.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>

typedef unsigned short in_port_t;

class CMediaStream
{
public:
	CMediaStream();
	~CMediaStream();
	bool _Init();
	void _Test();
private:
	in_port_t m_LocalRTPPort = 2048;

	pj_constants_ m_pj_inited = pj_constants_::PJ_FALSE;
};

