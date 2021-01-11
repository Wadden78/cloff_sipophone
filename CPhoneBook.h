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

const int ci_PhBHeight = ci_MainHeight;	/**< Высота окна по умолчанию*/
const int ci_PhBWidth = ci_MainWidth;	/** Ширина окна по умолчанию*/
const int ci_PhBNumHeaderWidth = 190;	/** Ширина колонки с номером абонента*/
const int ci_PhBHeaderNumber = 4;		/** Число колонок без иконки*/

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
 * \brief Записная книжка.
 */
class CPhoneBook
{
public:
	CPhoneBook();
	~CPhoneBook();
	/**
	 * \brief Показать окно.
	 *
	 */
	void _Show() 
	{ 
		SetForegroundWindow(m_hPhBWnd);
		BringWindowToTop(m_hPhBWnd);
		ShowWindow(m_hPhBWnd, SW_SHOWNORMAL);
	}
	/**
	 * \brief Скрыть окно.
	 *
	 */
	void _Hide() { ShowWindow(m_hPhBWnd, SW_HIDE); }
	/**
	 * Добавить вызов в список.
	 *
	 * \param wstrCallNum	- номер контакта
	 * \param wstrName		- имя контакта
	 */
	void _AddToList(const wchar_t* wstrName, const wchar_t* wstrCompany, const wchar_t* wstrPosition, const wchar_t* wstrNum);
	/**
	 * Сохранить параметры в файл конфигурации.
	 *
	 * \param pCfg - ссылка на объект работы с файлом конфигурации
	 */
	void _SaveParameters();
	/**
	 * Получить параметры из файла конфигурации.
	 *
	 */
	void _LoadData();
	/**
	 * \brief Очистить список.
	 * 
	 * \param bClearAll - true - удалить все записи/false - удалить одну запись
	 */
	void _ListViewClear(bool bClearAll = false);
	/**
	 * \brief Создать новую запись.
	 * 
	 */
	void _CreateNewRecord();
	/**
	 * \brief Отработка события WM_PAINT.
	 * 
	 */
	void _Paint();
	/**
	 * \brief Редактирование карточки клиента.
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

	HWND m_hPhBWnd{ 0 };			/**< дескриптор основного окна записной книжки*/
	HWND m_hListWnd{ 0 };			/**< дескриптор списка*/
	HIMAGELIST m_hImageList{ 0 };	/**< дескриптор списка изображений*/

	static LRESULT CALLBACK PhBWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void JSONParser(rapidjson::GenericObject<false, rapidjson::Value>& phb_obj, bool bUTF8=false);

	struct SContact
	{
		int32_t iId{ 0 };				///< идентификатор контакта
		wstring wstrName;				///< Имя 
		wstring wstrCompany;			///< Организация
		wstring wstrPosition;			///< Должность
		wstring wstrLocalNum;			///< Местный номер
		vector<wstring> vwstrNumber;	///< Список телефонных номеров
	};
	vector<shared_ptr<SContact>> m_vContactList;

	int HTTPParser(const char* pszText);

	map<int, wstring> m_mInMenuNumberList;
};

