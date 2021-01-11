/*****************************************************************//**
 * \file   id_list.h
 * \brief  Список идентификаторов елементов 
 * 
 * \author Wadden
 * \date   October 2020
 *********************************************************************/
#pragma once

enum class enListType { enLTIn, enLTOut, enLTMiss, enLTAll };

/**
 * \brief History Wnd.
 */
#define ID_HISTBUTTON_ALL	100		///< кнопка истории всех вызовов
#define ID_HISTBUTTON_OUT	101		///< кнопка истории исходящих вызовов
#define ID_HISTBUTTON_IN	102		///< кнопка истории входящих вызовов
#define ID_HISTBUTTON_MISS	103		///< кнопка истории пропущенных вызовов
#define ID_HISTLISTV_ALL	110		///< список всех вызовов
#define ID_HISTLISTV_OUT	111		///< список исходящих вызовов
#define ID_HISTLISTV_IN		112		///< список входящих вызовов
#define ID_HISTLISTV_MISS	113		///< список пропущенных вызовов
#define ID_HISTLISTV_PROGB	114		///< progress bar

/**
 * \brief Phone book Wnd.
 */
#define ID_PHONEBOOKLIST_ALL     150	///< список вызовов
#define ID_PHONEBOOKLIST_LISTBEG 160	///< начало списка номеров
#define ID_PHONEBOOKLIST_LISTEND 199	///< коней списка номеров

 /**
 * \brief Contact card Wnd.
 */
#define ID_CCSTATICTEXT_CONTACT	200		///< заголовок поля контакта
#define ID_CCSTATICTEXT_NAME	201		///< флаг включения имени в псевдоним контакта
#define ID_CCSTATICTEXT_ORG		202		///< флаг включения имени организации в псевдоним контакта
#define ID_CCSTATICTEXT_POS		203		///< флаг включения должности в псевдоним контакта
#define ID_CCSTATICTEXT_LOCNUM	204		///< заголовок поля местного номера
#define ID_CCSTATICTEXT_NUMLIST	205		///< заголовок поля списка телефонных номеров контакта
#define ID_CCSTATICTEXT_HSTLIST	206		///< заголовок поля списка вызовов контакта
#define ID_CCEDITCTRL_NAME		217		///< поле редактирования имени контакта
#define ID_CCEDITCTRL_ORG		218		///< поле редактирования организации контакта
#define ID_CCEDITCTRL_POS		219		///< поле редактирования должности контакта
#define ID_CCEDITCTRL_LOCNUM	220		///< поле редактирования местного номера контакта
#define ID_CCLISTVIEW_NUMLIST	221		///< список телефонных номеров контакта
#define ID_CCLISTVIEW_HSTLIST	222		///< список вызовов контакта
#define ID_CCBUTTONINS_NUMLIST	223		///< кнопка добавления телефонного номера контакта
#define ID_CCBUTTONDEL_NUMLIST	224		///< кнопка удаления телефонного номера контакта
#define ID_CCBUTTONED_NUMLIST	225		///< кнопка редактирования телефонного номера контакта
#define ID_CCBUTTON_OK			226		///< кнопка выхода с сохранением
#define ID_CCBUTTON_CANCEL		227		///< кнопка выхода без сохранения

