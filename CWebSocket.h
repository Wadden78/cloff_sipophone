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
\brief проверка символов на вхождение в список допустимых

Функция предназначена для проверки символов на допустимость использования при
шифровании по алгоритму BASE64

\param[in] c проверяемый символ
\return true в случае вхождения в диапазон/ false в случае недопустимости
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
	 * Запуск потока.
	 *
	 * \return true - поток запущен/ false - ошибка запуска потока
	 */
	bool _Start(void);
	/**
	 *  Останов потока.
	 *
	 * \param time_out - таймаут ожидания завершения потока
	 * \return true - поток остановлен нормально/ false - поток остановлен принудительно
	 */
	bool _Stop(int time_out);
	/**
	 * Функция отправляет сообщение в очередь обработчика.
	 *
	 * \param pMsg - ссылка на сообщение
	 */
	void _PutMessage(unique_ptr<nsMsg::SMsg>& pMsg);
private:
	HANDLE m_hEvtStop{ INVALID_HANDLE_VALUE };	/**< Событие для завершения потока */
	HANDLE m_hEvtQueue{ INVALID_HANDLE_VALUE };	/**< Событие наличия сообщения в очереди */
	DWORD m_ThreadID = 0;						/**<  ID потока с точки зрения операционной системы */
	HANDLE m_hThread{ INVALID_HANDLE_VALUE };	/**<  HANDLE потока */
	mutex m_mxQueue;
	queue<unique_ptr<nsMsg::SMsg>> m_queue;	/**< очередь сообщений от PJ*/

	/**
	 * Функция потока.
	 */
	static unsigned __stdcall s_ThreadProc(LPVOID lpParameter);

	/** \fn virtual void LifeLoop(BOOL bRestart)
		\brief Функция основного цикла потока
		\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void LifeLoop(BOOL bRestart);

	/** \fn void ErrorFilter(LPEXCEPTION_POINTERS ep)
	\brief Функция обработки ошибки при исключении
	\param[in] bRestart флаг, указывающий основному циклу, что он был перезапущен после обработки исключения
	*/
	void ErrorFilter(LPEXCEPTION_POINTERS ep);

	/** \fn virtual void LifeLoopOuter()
	\brief Внешний цикл потока
	*/
	void LifeLoopOuter();
	/** \fn virtual bool Tick()
	\brief Обработчик тика
	\return true - ok/false - ошибка
	*/
	bool Tick();
	HANDLE m_hTimer{ INVALID_HANDLE_VALUE };	/**< указатель на таймер */

	DWORD GetTickCountDifference(DWORD nPrevTick);
	/** \fn char* StackView(char* buffer,size_t stBufferSize, void* eaddr, DWORD FrameAddr)
	\brief Трассировка стека

	Функция предназначена для получения адреса модуля, вызвавшего исключение путём раскручивания стека.

	\param[out] buffer Буфер куда будет помещён результат
	\param[in] eaddr
	\param[in] FrameAddr
	\return указатель на результирующий буфер в случае удачного завершения и NULL в случае неудачи
	*/
	template<std::size_t SIZE> wchar_t* StackView(std::array<wchar_t, SIZE>& buffer, void* eaddr, DWORD FrameAddr);

	/** \fn void SetTimer(int iTout)
	\brief установить таймер в миллисекундах
	\param[in] iTout - величина интервала в миллисекундах
	*/
	void SetTimer(int imsInterval);
	/**
	 * Обработчик сообщений от PJ.
	 *
	 */
	void MessageSheduler();

	/**
	 * Инициализация сервера.
	 *
	 * \return true - сервер в работе / false - ошибка инициализации
	 */
	bool Open();
	/**
	 * закрытие работы сервера.
	 *
	 */
	void Close();

	SOCKET m_hTCPListenSocket{ INVALID_SOCKET };	/** TCP Socket */
	HANDLE m_hTCPListenSocketEvent;					/** Event on TCP socket */

	struct SClient
	{
		int iNum;										/** порядковый номер*/
		SOCKET m_hSocket{ INVALID_SOCKET };				/** Socket */
		HANDLE m_hSocketEvent{ INVALID_HANDLE_VALUE };	/** Event on socket */
		array<char, 1048576> m_cRecBuffer{ 0 };			/** буфер приёмника */
		DWORD m_dwNumberOfBytesRecvd{ 0 };				/** число принятых из сокета байт */
		UINT64 m_uiBytesPackets{ 0 };					/** число байт реально принятых для одного фрагментированного кадра */
		string m_strMessageBuffer;						/** Накопитель принятых байт для формирования сообщения */
		vector<char*> m_vCurrentMsgHeaderPointers;

		string m_strTCPFrame;							/** WEBsocket frame, собираемый из нескольких пакетов */
		UINT64 m_uiFrameLen{ 0 };						/** длина принятого WEBsocket frame */
		BYTE m_bOpCode{ 0 };							/** код фрейма */
		BYTE m_bFIN{ 0 };								/** признак продолжения */
		string m_strMask;								/** маска текущего пакета */
		string m_strSSID;								/** номер сессии WEBRTC */

		string m_strIP;									/** адрес:порт клиента */
		SOCKADDR_IN m_Addr{ 0 };						/** */

		bool m_bClick2Call{ false };
	};
	std::array<SClient,2> m_aClients;					/** список подключений*/
	/**
	 * /brief PacketParser разборка полученных пакетов, анализ содержимого на наличие сообщений.
	 *
	 * \param pMessage
	 */
	void PacketParser(SClient& sData, const char* pMessage);

	//WEB RTC-------------------------
#pragma pack(1)
	struct SWEBSocketFrame
	{
		UINT16 OpCode : 4;	//опкод
		UINT16 RSV3 : 1;	//RSV3
		UINT16 RSV2 : 1;	//RSV2
		UINT16 RSV1 : 1;	//RSV1
		UINT16 FIN : 1;		//FIN
		UINT16 BodyLen : 7;	//длина тела
		UINT16 Mask : 1;	//маска
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
	 * непосредственная отправка сообщения в сокет.
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