#include "CPJLogWriter.h"
#include "CSIPProcess.h"
#include "Main.h"

void CPJLogWriter::write(const LogEntry& entry)
{
	m_SIPProcess->_LogWrite(L"    PJ: L%d %S->%S", entry.level, entry.threadName.c_str(), entry.msg.c_str());
}