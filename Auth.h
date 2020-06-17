/*!
	/file
	/brief ���� �������� ����������� ������� ��� ���������� ������������� (MD5 � ��.).

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
#define SHA1LEN  20

struct SAuth
{
	std::string strNonce;		/**< ����, ����������� �������� � ������������ ������� */
	std::string strCNonce;		/**< ���������� ����, ������������� ��������   */
	std::string strUser;		/**< ��� ������������ (Login) ��������������� �������������� */
	std::string strRealm;		/**< ����� ��� ����� ������� ������������ �������������� */
	std::string strAlg;			/**< ������������ ��������� �������������� (MD5) */
	std::string strNonceCount;	/**< �����, ������������ ������� ��� ��� ����������� ���� Nonce */
	std::string strQop;			/**<  */
	std::string strURI;			/**< */
	std::string strBody;		/**< */
	std::string strPassword;	/**< ������ ������������ */
	std::string strResponse;	/**< ����� �������, ���������� ��� �����, ����������� � ��������� ���������� */
	std::string strOpaque;		/**< */
};


/** \class CAuth
\brief ����� �������� ������ ������� ����������� ���� � ������, ���������� ��� ��������������

*/
class CAuth
{
public:
	CAuth(void);
public:
	virtual ~CAuth(void);
	/** \fn int Init(void)
	\brief ������������� ����������
	\return 0 - ������������� ������ ������/ -1 - Error loading cryptdll.dll:MD5Init/ -2 - Error loading cryptdll.dll:MD5Update/ -3 -Error loading cryptdll.dll:MD5Final/ >0 - Error loading cryptdll.dll
	*/
	int Init(void);
	/** \fn int Authorization(const char* pszMethod, VerifyAuth& va, const char* sNonce, int &iNC)
	\brief �������� ���������� �����������

	������� ������������ ��� ����������� SIP �������� � �������� ����������� � ��� ������� ����������

	\param[in] pszMethod ������ � ������ ������ ��� ������� ���������� ����������� (REGISTER, INVITE, etc)
	\param[in] va ������ �� ��������� � ������������������� �������
	\param[in] sNonce ��������� �� ������ � ������
	\param[inout] iNC ������ �� ����� ������������� ������� �����
	\return 0 - ����������� ������ ������/ -1 - ������������ ������/ 1 - ������ � ���������� ��������������
	*/
	int Authorization(const char* pszMethod, SAuth& va, const char* sNonce, int &iNC);
	/** \fn bool CountAuthorization(const char* pszMethod, VerifyAuth& va)
	\brief ���������� ���������� �����������

	������� ������������ ��� ���������� ���������� ����������� ����������� �������� �������� ��� ����������� � �������� �������.
	��� ���� �������� ��������� ��� ������.

	\param[in] pszMethod ������ � ������ ������ ��� ������� ���������� ����������� (REGISTER, INVITE, etc)
	\param[in] va ������ �� ��������� � ������������������� �������
	\return true - ������/ false - ������
	*/
	bool CountAuthorization(const char* pszMethod, SAuth& va);
	/** \fn void CountWebSecureKey(const byte* const psInKey, const int iKeyLen, std::string& strResult)
	\brief ����������� ����� ��� WebSecureSocket ����������

	\param[in] psInKey ��������� �� �������� ������ � ������
	\param[in] iKeyLen ����� �����
	\param[out] strResult ������ �� ������ � �����������
	\return void
	*/
	int CountWebSecureKey(const byte* const psInKey, const int iKeyLen, std::string& strResult);

	/** \fn bool VerifyAuthorization(const char* pszMethod, VerifyAuth* va)
	\brief �������� ���������� ����������� ��� ����� ���������� ����� Nonce

	\param[in] pszMethod ������ � ������ ������ ��� ������� ���������� ����������� (REGISTER, INVITE, etc)
	\param[in] va ������ �� ��������� � ������������������� �������
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
\brief ����� ������������ ��� �������� ����������� ����� ������� CRC
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
	\brief ������� �������� ����������� �����

	������� ������������ ����������� ����� � ���������� � ���������� m_crc32

	\param[in] pData ��������� �� ������ ������
	\param[in] nLen ����� ������ ������� ������ � ������
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