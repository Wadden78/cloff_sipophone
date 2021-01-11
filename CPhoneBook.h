#pragma once
#include <winsock2.h>
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
#include <map>
#include <mutex>
#include <queue>
#include <mmsystem.h>
#include "Config.h"
#include "Main.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

using namespace std;

const int ci_PhBHeight = ci_MainHeight;	/**< ������ ���� �� ���������*/
const int ci_PhBWidth = ci_MainWidth;	/** ������ ���� �� ���������*/
const int ci_PhBNumHeaderWidth = 190;	/** ������ ������� � ������� ��������*/
const int ci_PhBHeaderNumber = 4;		/** ����� ������� ��� ������*/

#define PHBOOK_OBJ_NAME			"phone_book"
#define PHB_NAM_HD_WIDTH_NAME	"name_header_w"
#define PHB_NUM_HD_WIDTH_NAME	"default_number_header_w"
#define PHB_ORG_HD_WIDTH_NAME	"company_header_w"
#define PHB_POS_HD_WIDTH_NAME	"position_header_w"
#define PHB_DB_OBJ_NAME			"data"
#define PHB_CONTACT_ARRAY_NAME	"arr"
#define PHB_NUM_ARRAY_NAME		"arr_phone"
#define PHB_ID_FLD_NAME			"id"
#define PHB_NAME_FLD_NAME		"name"
#define PHB_LOCNUM_FLD_NAME		"ext"
#define PHB_ORG_FLD_NAME		"company"
#define PHB_POS_FLD_NAME		"position"

/**
 * \brief �������� ������.
 */
class CPhoneBook
{
public:
	CPhoneBook();
	~CPhoneBook();
	/**
	 * \brief �������� ����.
	 *
	 */
	void _Show() 
	{ 
		SetForegroundWindow(m_hPhBWnd);
		BringWindowToTop(m_hPhBWnd);
		ShowWindow(m_hPhBWnd, SW_SHOWNORMAL);
	}
	/**
	 * \brief ������ ����.
	 *
	 */
	void _Hide() { ShowWindow(m_hPhBWnd, SW_HIDE); }
	/**
	 * �������� ����� � ������.
	 *
	 * \param wstrCallNum	- ����� ��������
	 * \param wstrName		- ��� ��������
	 */
	void _AddToList(const wchar_t* wstrName, const wchar_t* wstrCompany, const wchar_t* wstrPosition, const wchar_t* wstrNum);
	/**
	 * ��������� ��������� � ���� ������������.
	 *
	 * \param pCfg - ������ �� ������ ������ � ������ ������������
	 */
	void _SaveParameters();
	/**
	 * �������� ��������� �� ����� ������������.
	 *
	 */
	void _LoadData();
	/**
	 * \brief �������� ������.
	 * 
	 * \param bClearAll - true - ������� ��� ������/false - ������� ���� ������
	 */
	void _ListViewClear(bool bClearAll = false);
	/**
	 * \brief ������� ����� ������.
	 * 
	 */
	void _CreateNewRecord();
	/**
	 * \brief ��������� ������� WM_PAINT.
	 * 
	 */
	void _Paint();
	/**
	 * \brief �������������� �������� �������.
	 * 
	 * \param iRecNum
	 */
	void _EditRecord(const int iRecNum);
	void _CallContactFromList(int iStringID);
	void _CallContactByDefault(LPARAM lParam);
	/**
	 * .
	 */
	HMENU _CreatePopUpMenu(LPARAM lParam);
private:

	HWND m_hPhBWnd{ 0 };			/**< ���������� ��������� ���� �������� ������*/
	HWND m_hListWnd{ 0 };			/**< ���������� ������*/
	HIMAGELIST m_hImageList{ 0 };	/**< ���������� ������ �����������*/

	static LRESULT CALLBACK PhBWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void JSONParser(rapidjson::GenericObject<false, rapidjson::Value>& phb_obj, bool bUTF8=false);

	struct SContact
	{
		int32_t iId{ 0 };				///< ������������� ��������
		wstring wstrName;				///< ��� 
		wstring wstrCompany;			///< �����������
		wstring wstrPosition;			///< ���������
		wstring wstrLocalNum;			///< ������� �����
		vector<wstring> vwstrNumber;	///< ������ ���������� �������
	};
	vector<shared_ptr<SContact>> m_vContactList;

	int HTTPParser(const char* pszText);

	map<int, wstring> m_mInMenuNumberList;
};

