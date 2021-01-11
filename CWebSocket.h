#pragma once

#include <winsock2.h>

#include "main.h"
#include "CSIPProcess.h"
#include <time.h>
#include <sys/timeb.h>
#include <tchar.h>
#include <process.h>
#include <delayimp.h>
#include <string>
#include <map>
#include <vector>
#include <list>
#include "CSIPWEB.h"

#define SHA1LEN  20
static const char* c_szGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

/**	\fn static inline bool is_base64(unsigned char c)
\brief �������� �������� �� ��������� � ������ ����������

������� ������������� ��� �������� �������� �� ������������ ������������� ���
���������� �� ��������� BASE64

\param[in] c ����������� ������
\return true � ������ ��������� � ��������/ false � ������ ��������������
*/
static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

class CWebSocket
{
public:
	CWebSocket();
	~CWebSocket();
	/**
	 * ������ ������.
	 *
	 * \return true - ����� �������/ false - ������ ������� ������
	 */
	bool _Start(void);
	/**
	 *  ������� ������.
	 *
	 * \param time_out - ������� �������� ���������� ������
	 * \return true - ����� ���������� ���������/ false - ����� ���������� �������������
	 */
	bool _Stop(int time_out);
	/**
	 * ������� ���������� ��������� � ������� �����������.
	 *
	 * \param pMsg - ������ �� ���������
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
private:
	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< ������� ��� ���������� ������ */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< ������� ������� ��������� � ������� */
	DWORD m_ThreadID = 0;						/**<  ID ������ � ����� ������ ������������ ������� */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE ������ */
	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< ������� ��������� �� PJ*/

	/**
	 * ������� ������.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief ������� ��������� ����� ������
		\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief ������� ��������� ������ ��� ����������
	\param[in] bRestart ����, ����������� ��������� �����, ��� �� ��� ����������� ����� ��������� ����������
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief ������� ���� ������
	*/
	void LifeLoopOuter();
	/** \fn virtual bool Tick()
	\brief ���������� ����
	\return true - ok/false - ������
	*/
	bool Tick();
	HANDLE m_hTimer{ INVALID_HANDLE_VALUE };	/**< ��������� �� ������ */

	DWORD GetTickCountDifference(DWORD nPrevTick);
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief ����������� �����

	������� ������������� ��� ��������� ������ ������, ���������� ���������� ���� ������������� �����.

	\param[out] buffer ����� ���� ����� ������� ���������
	\param[in] eaddr
	\param[in] FrameAddr
	\return ��������� �� �������������� ����� � ������ �������� ���������� � NULL � ������ �������
	*/
	template<std::size_t SIZE> wchar_t* StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr);

	/** \fn void SetTimer(int iTout)
	\brief ���������� ������ � �������������
	\param[in] iTout - �������� ��������� � �������������
	*/
	void SetTimer(int imsInterval);
	/**
	 * ���������� ��������� �� PJ.
	 *
	 */
	void MessageSheduler();

	/**
	 * ������������� �������.
	 *
	 * \return true - ������ � ������ / false - ������ �������������
	 */
	bool Open();
	/**
	 * �������� ������ �������.
	 *
	 */
	void Close();

	SOCKET m_hTCPListenSocket{ INVALID_SOCKET };	/** TCP Socket */
	HANDLE m_hTCPListenSocketEvent;					/** Event on TCP socket */

	struct SClient
	{
		int iNum;										/** ���������� �����*/
		SOCKET m_hSocket{ INVALID_SOCKET };				/** Socket */
		HANDLE m_hSocketEvent{ INVALID_HANDLE_VALUE };	/** Event on socket */
		array<char, 1048576> m_cRecBuffer{ 0 };			/** ����� �������� */
		DWORD m_dwNumberOfBytesRecvd{ 0 };				/** ����� �������� �� ������ ���� */
		UINT64 m_uiBytesPackets{ 0 };					/** ����� ���� ������� �������� ��� ������ ������������������ ����� */
		string m_strMessageBuffer;						/** ���������� �������� ���� ��� ������������ ��������� */
		vector<char*> m_vCurrentMsgHeaderPointers;

		string m_strTCPFrame;							/** WEBsocket frame, ���������� �� ���������� ������� */
		UINT64 m_uiFrameLen{ 0 };						/** ����� ��������� WEBsocket frame */
		BYTE m_bOpCode{ 0 };							/** ��� ������ */
		BYTE m_bFIN{ 0 };								/** ������� ����������� */
		string m_strMask;								/** ����� �������� ������ */
		string m_strSSID;								/** ����� ������ WEBRTC */

		string m_strIP;									/** �����:���� ������� */
		SOCKADDR_IN m_Addr{ 0 };						/** */

		bool m_bClick2Call{ false };
	};
	std::array<SClient,2> m_aClients;					/** ������ �����������*/
	/**
	 * /brief PacketParser �������� ���������� �������, ������ ����������� �� ������� ���������.
	 *
	 * \param pMessage
	 */
	void PacketParser(SClient& sData, const char* pMessage);

	//WEB RTC-------------------------
#pragma pack(1)
	struct SWEBSocketFrame
	{
		UINT16 OpCode : 4;	//�����
		UINT16 RSV3 : 1;	//RSV3
		UINT16 RSV2 : 1;	//RSV2
		UINT16 RSV1 : 1;	//RSV1
		UINT16 FIN : 1;		//FIN
		UINT16 BodyLen : 7;	//����� ����
		UINT16 Mask : 1;	//�����
	};
	enum OpCodeEnum
	{
		kOpCodeContinuation = 0x0,
		kOpCodeText = 0x1,
		kOpCodeBinary = 0x2,
		kOpCodeDataUnused = 0x3,
		kOpCodeClose = 0x8,
		kOpCodePing = 0x9,
		kOpCodePong = 0xA,
		kOpCodeControlUnused = 0xB,
	};
#pragma pack()

	void HandShake(SClient& sData);
	/**
	 * ���������������� �������� ��������� � �����.
	 */
	void MsgSendToRemoteSide(SClient& sData, const BYTE* pMsg, size_t stMsgLen);
	int WEBSocketEncoder(OpCodeEnum opCode, const BYTE* pbSrcBuffer, size_t stSorceLen, BYTE* pbDstBuffer, size_t stDestSize);
	int WEBSocketDecoder(SClient& sData, const char* const src, const UINT32 iBytesCount);

	void SetMessageHeaderPointer(const char* szMsg, std::vector<char*>& vHeaderPointers, bool bWEB);
	std::vector<char*>::size_type FindHeaderByName(const char* szFullName, const char* szCompactName, std::vector<char*>& vHeaderPointers, std::vector<char*>::size_type stStart, char** psBodyBegin, char** psBodyEnd);
	char* InHeaderSubStrFind(const char* szPattern, std::vector<char*>& vHeaderPointers, std::vector<char*>::size_type stHeaderPointerNum);
	void CountWebSecureKey(const BYTE* const psInKey, const int iKeyLen, std::string& strResult);
	string base64_decode(std::string const& encoded_string);
	string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
};

/*

*/