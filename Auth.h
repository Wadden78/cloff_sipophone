/*!
	/file
	/brief Файл содержит необходимые функции для проведения аутентфикации (MD5 и пр.).

*/
#pragma once

#include <Windows.h>
#include <Wincrypt.h>
#include <string>

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
#define IN
#define OUT

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
#define SHA1LEN  20

struct SAuth
{
	std::string strNonce;		/**< ключ, формируемый сервером и отправляемый клиенту */
	std::string strCNonce;		/**< клиентский ключ, сфорированный клиентом   */
	std::string strUser;		/**< имя пользователя (Login) осуществляющего аутентификацию */
	std::string strRealm;		/**< домен или адрес сервера запросившего аутентификацию */
	std::string strAlg;			/**< наименование алгоритма аутентификации (MD5) */
	std::string strNonceCount;	/**< число, показывающее сколько раз был использован ключ Nonce */
	std::string strQop;			/**<  */
	std::string strURI;			/**< */
	std::string strBody;		/**< */
	std::string strPassword;	/**< пароль пользователя */
	std::string strResponse;	/**< ответ клиента, содержащий хэш полей, участвующих в алгоритме шифрования */
	std::string strOpaque;		/**< */
};


/** \class CAuth
\brief класс содержит методы посчёта контрольных сумм и ключей, прменяемых при аутентификации

*/
class CAuth
{
public:
	CAuth(void);
public:
	virtual ~CAuth(void);
	/** \fn int Init(void)
	\brief инициализация библиотеки
	\return 0 - инициализация прошла удачно/ -1 - Error loading cryptdll.dll:MD5Init/ -2 - Error loading cryptdll.dll:MD5Update/ -3 -Error loading cryptdll.dll:MD5Final/ >0 - Error loading cryptdll.dll
	*/
	int Init(void);
	/** \fn int Authorization(const char* pszMethod, VerifyAuth& va, const char* sNonce, int &iNC)
	\brief проверка параметров авторизации

	Функция используется для авторизации SIP клиентов в процессе регистрации и при запросе соединения

	\param[in] pszMethod строка с именем метода для которго проводится авторизация (REGISTER, INVITE, etc)
	\param[in] va ссылка на структуру с аутентификационными данными
	\param[in] sNonce указатель на строку с ключём
	\param[inout] iNC ссылка на число использований данного ключа
	\return 0 - авторизация прошла удачно/ -1 - неправильный пароль/ 1 - ошибка в параметрах аутентификации
	*/
	int Authorization(const char* pszMethod, SAuth& va, const char* sNonce, int &iNC);
	/** \fn bool CountAuthorization(const char* pszMethod, VerifyAuth& va)
	\brief заполнение параметров авторизации

	Функция используется для заполнения параметров авторизации запрошенных удалённым сервером при регистрации в качестве клиента.
	При этом клиентом выступает наш сервер.

	\param[in] pszMethod строка с именем метода для которго проводится авторизация (REGISTER, INVITE, etc)
	\param[in] va ссылка на структуру с аутентификационными данными
	\return true - удачно/ false - ошибка
	*/
	bool CountAuthorization(const char* pszMethod, SAuth& va);
	/** \fn void CountWebSecureKey(const byte* const psInKey, const int iKeyLen, std::string& strResult)
	\brief расшифровка ключа для WebSecureSocket соединения

	\param[in] psInKey указатель на байтовый массив с ключём
	\param[in] iKeyLen длина ключа
	\param[out] strResult ссылка на строку с результатом
	\return void
	*/
	int CountWebSecureKey(const byte* const psInKey, const int iKeyLen, std::string& strResult);

	/** \fn bool VerifyAuthorization(const char* pszMethod, VerifyAuth* va)
	\brief проверка параметров авторизации без учёта совпадения ключа Nonce

	\param[in] pszMethod строка с именем метода для которго проводится авторизация (REGISTER, INVITE, etc)
	\param[in] va ссылка на структуру с аутентификационными данными
	\return void
	*/
	//		Verify authorization RFC2617
	bool VerifyAuthorization(const char* pszMethod, SAuth* va);

private:
	typedef struct 
	{  
		ULONG i[2];  
		ULONG buf[4];  
		unsigned char in[64];  
		unsigned char digest[16];
	} MD5_CTX;

	typedef void (__stdcall *MD5Init_type)(MD5_CTX* context);
	typedef void (__stdcall *MD5Update_type)(MD5_CTX* context, char* input, int inlen);
	typedef void (__stdcall *MD5Final_type)(MD5_CTX* context);

	MD5Init_type MD5Init;
	MD5Update_type MD5Update;
	MD5Final_type MD5Final;

	char szDP[2]{ ':',0 };

//		calculate H(A1) as per spec
	void DigestCalcHA1(IN const char* pszAlg, IN const char* pszUserName, IN const char* pszRealm, IN const char* pszPassword, IN const char* pszNonce, IN const char* pszCNonce, OUT char* SessionKey);
//		calculate H(A2) as per spec
	void DigestCalcHA2(IN const char* pszMethod, IN const char* pszDigestUri, IN const char* pszQop, IN const char* pszBody, OUT char* HA2Hex);
//		calculate request-digest/response-digest as per HTTP Digest spec
	void DigestCalcResponse(IN const char* HA1, IN const char* pszNonce, IN const char* pszNonceCount, IN const char* pszCNonce, IN const char* pszQop, IN const char* HA2, OUT char* Response);

	void CvtHex(IN HASH Bin, OUT char* Hex);

	std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
	std::string base64_decode(std::string const& encoded_string);

};

/** \class crc32
\brief класс используетсч для подсчёта контрольной суммы методом CRC
*/
class crc32
{
protected:
	static unsigned __int32 table[256];
public:
	unsigned __int32 m_crc32;
	crc32() { m_crc32 = 0; };
	static void _CRC32_Init()
	{
		const unsigned __int32 CRC_POLY = 0xEDB88320;
		unsigned __int32 i, j, r;
		for(i = 0; i < 256; i++)
		{
			for(r = i, j = 8; j; j--)
				r = r & 1 ? (r >> 1) ^ CRC_POLY : r >> 1;
			table[i] = r;
		}
	}
	/** \fn void _ProcessCRC(void* pData, register int nLen)
	\brief функция подсчёта контрольной суммы

	Функция подсчитывает контрольную сумму и записывает в переменную m_crc32

	\param[in] pData указатель на массив данных
	\param[in] nLen длина размер массива данных в байтах
	\return void

	*/
	void _ProcessCRC(void* pData, register int nLen)
	{
		const unsigned __int32 CRC_MASK = 0xD202EF8D;
		register unsigned char* pdata = reinterpret_cast<unsigned char*>(pData);
		register unsigned crc = m_crc32;
		while(nLen--)
		{
			crc = table[static_cast<unsigned char>(crc) ^ *pdata++] ^ crc >> 8;
			crc ^= CRC_MASK;
		}
		m_crc32 = crc;
	}
};