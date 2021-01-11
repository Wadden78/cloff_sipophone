/*****************************************************************//**
 * \file   id_list.h
 * \brief  ������ ��������������� ��������� 
 * 
 * \author Wadden
 * \date   October 2020
 *********************************************************************/
#pragma once

enum class enListType { enLTIn, enLTOut, enLTMiss, enLTAll };

/**
 * \brief History Wnd.
 */
#define ID_HISTBUTTON_ALL	100		///< ������ ������� ���� �������
#define ID_HISTBUTTON_OUT	101		///< ������ ������� ��������� �������
#define ID_HISTBUTTON_IN	102		///< ������ ������� �������� �������
#define ID_HISTBUTTON_MISS	103		///< ������ ������� ����������� �������
#define ID_HISTLISTV_ALL	110		///< ������ ���� �������
#define ID_HISTLISTV_OUT	111		///< ������ ��������� �������
#define ID_HISTLISTV_IN		112		///< ������ �������� �������
#define ID_HISTLISTV_MISS	113		///< ������ ����������� �������
#define ID_HISTLISTV_PROGB	114		///< progress bar

/**
 * \brief Phone book Wnd.
 */
#define ID_PHONEBOOKLIST_ALL     150	///< ������ �������
#define ID_PHONEBOOKLIST_LISTBEG 160	///< ������ ������ �������
#define ID_PHONEBOOKLIST_LISTEND 199	///< ����� ������ �������

 /**
 * \brief Contact card Wnd.
 */
#define ID_CCSTATICTEXT_CONTACT	200		///< ��������� ���� ��������
#define ID_CCSTATICTEXT_NAME	201		///< ���� ��������� ����� � ��������� ��������
#define ID_CCSTATICTEXT_ORG		202		///< ���� ��������� ����� ����������� � ��������� ��������
#define ID_CCSTATICTEXT_POS		203		///< ���� ��������� ��������� � ��������� ��������
#define ID_CCSTATICTEXT_LOCNUM	204		///< ��������� ���� �������� ������
#define ID_CCSTATICTEXT_NUMLIST	205		///< ��������� ���� ������ ���������� ������� ��������
#define ID_CCSTATICTEXT_HSTLIST	206		///< ��������� ���� ������ ������� ��������
#define ID_CCEDITCTRL_NAME		217		///< ���� �������������� ����� ��������
#define ID_CCEDITCTRL_ORG		218		///< ���� �������������� ����������� ��������
#define ID_CCEDITCTRL_POS		219		///< ���� �������������� ��������� ��������
#define ID_CCEDITCTRL_LOCNUM	220		///< ���� �������������� �������� ������ ��������
#define ID_CCLISTVIEW_NUMLIST	221		///< ������ ���������� ������� ��������
#define ID_CCLISTVIEW_HSTLIST	222		///< ������ ������� ��������
#define ID_CCBUTTONINS_NUMLIST	223		///< ������ ���������� ����������� ������ ��������
#define ID_CCBUTTONDEL_NUMLIST	224		///< ������ �������� ����������� ������ ��������
#define ID_CCBUTTONED_NUMLIST	225		///< ������ �������������� ����������� ������ ��������
#define ID_CCBUTTON_OK			226		///< ������ ������ � �����������
#define ID_CCBUTTON_CANCEL		227		///< ������ ������ ��� ����������

