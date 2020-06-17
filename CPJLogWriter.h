#pragma once
#include <Windows.h>
#include <tchar.h>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <pjsua2.hpp>

using namespace pj;
using namespace std;

class CPJLogWriter :  public LogWriter
{
public:
	CPJLogWriter() = default;
	virtual ~CPJLogWriter() = default;

	virtual void write(const LogEntry& entry);
};

