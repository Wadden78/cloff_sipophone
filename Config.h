/** \file ConfigFile.h
/brief Файл содержит класс для работы с текстовым файлом конфигурации
*/
#pragma once

#include <stdio.h>
#include <codecvt>
#include "main.h"

/** \class CConfigFile
\brief Класс содержит свойства и методы для работы с текстовым файлом конфигурации
*/
class CConfigFile
{
public:
	/** \fn CConfigFile(void)
	\brief Конструктор
	*/
	CConfigFile(bool bWrite);
	/** \fn virtual ~CConfigFile(void)
	\brief Деструктор
	*/
	virtual ~CConfigFile(void);

	/** \fn int GetStringParameter(const wchar_t* szSection, const wchar_t* szParameter, wstring& strResult)
	\brief Получить строку из файла конфигурации
	\param[in] szSection - указатель на имя секции
	\param[in] szParameter - указатель на имя параметра
	\param[out] strResult - ссылка на строку, куда будет записан результат
	\return длина htpekmnbhe.otq строки
	*/
	size_t GetStringParameter(const char* szSection, const char* szParameter, wstring& wstrResult);
	/** \fn int _GetRegistryState(wchar_t* szBuffer, size_t stBufferSize)
	\brief Получить расшифровку состояние регистрации в буфер
	\param[out] szBuffer - указатель на буфер, куда будет помещён результат
	\param[in] stBufferSize - длина буфера результата
	\return длина строки в буфере
	*/
	int GetIntParameter(const char* szSection, const char* szParameter, int iDefault);
	/** \fn int _GetRegistryState(wchar_t* szBuffer, size_t stBufferSize)
	\brief Получить расшифровку состояние регистрации в буфер
	\param[out] szBuffer - указатель на буфер, куда будет помещён результат
	\param[in] stBufferSize - длина буфера результата
	\return длина строки в буфере
	*/
	size_t GetCodedStringParameter(const char* szSection, const char* szParameter, wstring& wstrResult);
	bool _CheckConfig();
private:
	FILE* m_fCfgFile{ nullptr }; /**< Указатель на дескриптор открытого файла конфигурации*/
	bool m_bWriteOnly{ false }; /**< файл открыт только для записи*/

};
