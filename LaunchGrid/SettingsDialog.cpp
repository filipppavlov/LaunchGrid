#include "stdafx.h"
#include "SettingsDialog.h"
#include "resource.h"
#include "Settings.h"

using namespace simplejson;

namespace
{
	Value _settings;


	BOOL CALLBACK GeneralProc(HWND hwndDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) {
		auto dlgItem = [=](int control) { return GetDlgItem(hwndDlg, control); };
		switch (message)
		{
		case WM_INITDIALOG:
		{
			bool bottomWindow = _settings[L"general"][L"bottomWindow"].asNumber() != 0;
			Button_SetCheck(dlgItem(bottomWindow ? IDC_BOTTOM_WINDOW : IDC_TOP_WINDOW), BST_CHECKED);
			Button_Enable(dlgItem(IDC_AUTO_START), !bottomWindow ? FALSE : TRUE);
			Button_Enable(dlgItem(IDC_DO_NOT_CLOSE), !bottomWindow ? TRUE : FALSE);
			if (_settings[L"general"][L"autoStart"])
			{
				Button_SetCheck(dlgItem(IDC_AUTO_START), BST_CHECKED);
			}
			if (_settings[L"general"][L"doNotClose"])
			{
				Button_SetCheck(dlgItem(IDC_DO_NOT_CLOSE), BST_CHECKED);
			}
			bool darkTheme = _settings[L"general"][L"theme"].asNumber() != 0;
			Button_SetCheck(dlgItem(darkTheme ? IDC_DARK_THEME : IDC_LIGHT_THEME), TRUE);
			Button_SetCheck(dlgItem(IDC_LOG), _settings[L"general"][L"log"].asNumber() != 0);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_TOP_WINDOW:
			case IDC_BOTTOM_WINDOW:
			{
				auto checked = IsDlgButtonChecked(hwndDlg, IDC_TOP_WINDOW);
				Button_Enable(dlgItem(IDC_AUTO_START), checked ? FALSE : TRUE);
				Button_Enable(dlgItem(IDC_DO_NOT_CLOSE), checked ? TRUE : FALSE);
				_settings.setDefault(L"general", Value::object())[L"bottomWindow"] = Value::number(checked ? 0.0 : 1.0);
				break;
			}
			case IDC_AUTO_START:
				_settings.setDefault(L"general", Value::object())[L"autoStart"] = Value::number(IsDlgButtonChecked(hwndDlg, IDC_AUTO_START) ? 1.0 : 0.0);
				break;
			case IDC_DO_NOT_CLOSE:
				_settings.setDefault(L"general", Value::object())[L"doNotClose"] = Value::number(IsDlgButtonChecked(hwndDlg, IDC_DO_NOT_CLOSE) ? 1.0 : 0.0);
				break;
			case IDC_LIGHT_THEME:
			case IDC_DARK_THEME:
				_settings.setDefault(L"general", Value::object())[L"theme"] = Value::number(IsDlgButtonChecked(hwndDlg, IDC_DARK_THEME) ? 1.0 : 0.0);
				break;
			case IDC_LOG:
				_settings.setDefault(L"general", Value::object())[L"log"] = Value::number(IsDlgButtonChecked(hwndDlg, IDC_LOG) ? 1.0 : 0.0);
				break;
			default:
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}


	BOOL CALLBACK ContentProc(HWND hwndDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) 
	{
		auto dlgItem = [=](int control) { return GetDlgItem(hwndDlg, control); };
		switch (message)
		{
		case WM_INITDIALOG:
		{
			SetWindowLong(dlgItem(IDC_FLAG_ADD), GWL_STYLE, GetWindowStyle(dlgItem(IDC_FLAG_ADD)) | BS_SPLITBUTTON);

			BUTTON_SPLITINFO splitInfo;
			ZeroMemory(&splitInfo, sizeof(splitInfo));
			splitInfo.mask = BCSIF_STYLE;
			splitInfo.uSplitStyle = BCSS_NOSPLIT;
			Button_SetSplitInfo(dlgItem(IDC_FLAG_ADD), &splitInfo);

			ComboBox_AddString(dlgItem(IDC_FLAG_POSITION), L"Row");
			ComboBox_AddString(dlgItem(IDC_FLAG_POSITION), L"Column");
			ComboBox_AddString(dlgItem(IDC_FLAG_POSITION), L"Select");

			ComboBox_AddString(dlgItem(IDC_VERB), L"");
			ComboBox_AddString(dlgItem(IDC_VERB), L"open");
			ComboBox_AddString(dlgItem(IDC_VERB), L"print");
			ComboBox_AddString(dlgItem(IDC_VERB), L"runas");

			auto tabs = _settings.setDefault(L"tabs", Value::array());
			if (tabs.length() == 0)
			{
				auto def = Value::object();
				def[L"name"] = Value::string(L"Default");
				tabs.push(def);
				def[L"flags"] = Value::array();
				def[L"flags"].push(Value::object());
				def[L"flags"][size_t(0)][L"name"] = Value::string(L"App");
				def[L"flags"][size_t(0)][L"type"] = Value::string(L"app");
				def[L"flags"][size_t(0)][L"position"] = Value::number(0);
			}
			auto tabsWnd = dlgItem(IDC_TABS);
			for (auto tab : tabs)
			{
				ListBox_AddString(tabsWnd, tab[L"name"].asString().c_str());
			}
			ListBox_SetCurSel(tabsWnd, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_TABS, LBN_SELCHANGE), 0);
			Button_Enable(dlgItem(IDC_TAB_REMOVE), tabs.length() > 1);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_TABS:
				switch (HIWORD(wParam))
				{
				case LBN_SELCHANGE:
					auto sel = ListBox_GetCurSel(dlgItem(IDC_TABS));
					while (ListBox_GetCount(dlgItem(IDC_FLAGS)))
					{
						ListBox_DeleteString(dlgItem(IDC_FLAGS), 0);
					}
					if (sel >= 0)
					{
						Edit_SetText(dlgItem(IDC_TAB_NAME), _settings[L"tabs"][sel][L"name"].asString().c_str());
						for (auto flag : _settings[L"tabs"][sel][L"flags"])
						{
							ListBox_AddString(dlgItem(IDC_FLAGS), flag[L"name"].asString().c_str());
						}
						ListBox_SetCurSel(dlgItem(IDC_FLAGS), 0);
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FLAGS, LBN_SELCHANGE), 0);
					}
					else
					{
						Edit_SetText(dlgItem(IDC_TAB_NAME), L"");
					}
					Button_Enable(dlgItem(IDC_TAB_UP), sel > 0);
					Button_Enable(dlgItem(IDC_TAB_DOWN), sel + 1 < ListBox_GetCount(dlgItem(IDC_TABS)));
					break;
				}
				break;
			case IDC_TAB_NAME:
			{
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_TAB_NAME), name, 512);
				auto list = dlgItem(IDC_TABS);
				auto sel = ListBox_GetCurSel(list);
				ListBox_DeleteString(list, sel);
				ListBox_InsertString(list, sel, name);
				ListBox_SetCurSel(list, sel);
				_settings[L"tabs"][sel][L"name"] = Value::string(name);
				break;
			}
			case IDC_TAB_ADD:
			{
				auto name = L"new";
				auto list = dlgItem(IDC_TABS);
				auto index = ListBox_AddString(list, name);
				auto tab = Value::object();
				tab[L"name"] = Value::string(L"name");
				_settings[L"tabs"].push(tab);
				tab[L"flags"] = Value::array();
				tab[L"flags"].push(Value::object());
				tab[L"flags"][size_t(0)][L"name"] = Value::string(L"App");
				tab[L"flags"][size_t(0)][L"type"] = Value::string(L"app");
				tab[L"flags"][size_t(0)][L"position"] = Value::number(0);
				ListBox_SetCurSel(list, index);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_TABS, LBN_SELCHANGE), 0);
				Button_Enable(dlgItem(IDC_TAB_REMOVE), TRUE);
				SetFocus(dlgItem(IDC_TAB_NAME));
				Edit_SetSel(dlgItem(IDC_TAB_NAME), 0, Edit_GetTextLength(dlgItem(IDC_TAB_NAME)));
				break;
			}
			case IDC_TAB_REMOVE:
			{
				auto count = _settings[L"tabs"].length();
				if (count > 1)
				{
					if (MessageBox(hwndDlg, L"Remove tab?\nThis will remove all app and option settings for the tab.", L"Remove tab", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
					{
						auto list = dlgItem(IDC_TABS);
						auto sel = ListBox_GetCurSel(list);
						ListBox_DeleteString(list, sel);
						_settings[L"tabs"].remove(size_t(sel));
						if (sel < int(count) - 1)
						{
							ListBox_SetCurSel(list, sel);
						}
						else
						{
							ListBox_SetCurSel(list, sel - 1);
						}
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_TABS, LBN_SELCHANGE), 0);
						Button_Enable(dlgItem(IDC_TAB_REMOVE), count > 2);
					}
				}
				break;
			}
			case IDC_TAB_UP:
			{
				auto list = dlgItem(IDC_TABS);
				auto sel = ListBox_GetCurSel(list);
				if (sel > 0)
				{
					std::swap(_settings[L"tabs"][sel], _settings[L"tabs"][sel - 1]);
					ListBox_DeleteString(list, sel - 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel - 1);
					if (sel == 1)
					{
						Button_Enable(dlgItem(IDC_TAB_UP), FALSE);
					}
					Button_Enable(dlgItem(IDC_TAB_DOWN), TRUE);
				}
				break;
			}
			case IDC_TAB_DOWN:
			{
				auto list = dlgItem(IDC_TABS);
				auto sel = ListBox_GetCurSel(list);
				auto count = ListBox_GetCount(list);
				if (sel + 1 < count)
				{
					std::swap(_settings[L"tabs"][sel], _settings[L"tabs"][sel + 1]);
					ListBox_DeleteString(list, sel + 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel + 1);
					if (sel + 2 >= count)
					{
						Button_Enable(dlgItem(IDC_TAB_DOWN), FALSE);
					}
					Button_Enable(dlgItem(IDC_TAB_UP), TRUE);
				}
				break;
			}
			case IDC_FLAGS:
				switch (HIWORD(wParam))
				{
				case LBN_SELCHANGE:
				{
					auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
					auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
					Edit_SetText(dlgItem(IDC_FLAG_NAME), _settings[L"tabs"][tab][L"flags"][flag][L"name"].asString().c_str());
					Edit_SetText(dlgItem(IDC_FLAG_TYPE), _settings[L"tabs"][tab][L"flags"][flag][L"type"].asString().c_str());
					ComboBox_SetCurSel(dlgItem(IDC_FLAG_POSITION), int(_settings[L"tabs"][tab][L"flags"][flag][L"position"].asNumber()));

					while (ListBox_GetCount(dlgItem(IDC_VALUES)))
					{
						ListBox_DeleteString(dlgItem(IDC_VALUES), 0);
					}
					auto values = _settings[L"tabs"][tab][L"flags"][flag][L"values"];
					for (auto value : values)
					{
						ListBox_AddString(dlgItem(IDC_VALUES), value[L"name"].asString().c_str());
					}
					if (values.length())
					{
						ListBox_SetCurSel(dlgItem(IDC_VALUES), 0);
					}
					SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_VALUES, LBN_SELCHANGE), 0);
					Button_Enable(dlgItem(IDC_FLAG_UP), flag > 0);
					Button_Enable(dlgItem(IDC_FLAG_DOWN), flag + 1 < ListBox_GetCount(dlgItem(IDC_FLAGS)));
					bool isApp = _settings[L"tabs"][tab][L"flags"][flag][L"type"].asString() == L"app";
					Button_Enable(dlgItem(IDC_FLAG_REMOVE), !isApp);
					Static_SetText(dlgItem(IDC_VALUE_VALUE_LABEL), isApp ? L"Command" : L"Value");
					ShowWindow(dlgItem(IDC_ARGS_LABEL), isApp ? SW_SHOWNA : SW_HIDE);
					ShowWindow(dlgItem(IDC_ARGS), isApp ? SW_SHOWNA : SW_HIDE);
					ShowWindow(dlgItem(IDC_VERB_LABEL), isApp ? SW_SHOWNA : SW_HIDE);
					ShowWindow(dlgItem(IDC_VERB), isApp ? SW_SHOWNA : SW_HIDE);
					ShowWindow(dlgItem(IDC_HIDDEN), isApp ? SW_SHOWNA : SW_HIDE);
					break;
				}
				}
				break;
			case IDC_FLAG_NAME:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_FLAG_NAME), name, 512);
				auto list = dlgItem(IDC_FLAGS);
				auto sel = ListBox_GetCurSel(list);
				ListBox_DeleteString(list, sel);
				ListBox_InsertString(list, sel, name);
				ListBox_SetCurSel(list, sel);
				_settings[L"tabs"][tab][L"flags"][sel][L"name"] = Value::string(name);
				break;
			}
			case IDC_FLAG_POSITION:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				_settings[L"tabs"][tab][L"flags"][flag][L"position"] = Value::number(ComboBox_GetCurSel(dlgItem(IDC_FLAG_POSITION)));
				break;
			}
			case IDC_FLAG_ADD:
			{
				RECT rect;
				GetWindowRect(dlgItem(IDC_FLAG_ADD), &rect);
				//// Create a menu and add items.
				HMENU menu = GetSubMenu(LoadMenu(nullptr, MAKEINTRESOURCE(IDR_ADD_FLAG_MENU)), 0);
				TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, rect.left, rect.bottom, 0, hwndDlg, NULL);
				break;
			}
			case ID_FLAG_FLAG:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = Value::object();
				flag[L"name"] = Value::string(L"name");
				flag[L"type"] = Value::string(L"flag");
				flag[L"position"] = Value::number(0);
				flag[L"values"] = Value::array();
				_settings[L"tabs"][tab][L"flags"].push(flag);
				ListBox_AddString(dlgItem(IDC_FLAGS), L"name");
				ListBox_SetCurSel(dlgItem(IDC_FLAGS), _settings[L"tabs"][tab][L"flags"].length() - 1);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FLAGS, LBN_SELCHANGE), 0);
				SetFocus(dlgItem(IDC_FLAG_NAME));
				Edit_SetSel(dlgItem(IDC_FLAG_NAME), 0, Edit_GetTextLength(dlgItem(IDC_FLAG_NAME)));
				break;
			}
			case ID_FLAG_DIRECTORY:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = Value::object();
				flag[L"name"] = Value::string(L"name");
				flag[L"type"] = Value::string(L"dir");
				flag[L"position"] = Value::number(0);
				_settings[L"tabs"][tab][L"flags"].push(flag);
				ListBox_AddString(dlgItem(IDC_FLAGS), L"name");
				ListBox_SetCurSel(dlgItem(IDC_FLAGS), _settings[L"tabs"][tab][L"flags"].length() - 1);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FLAGS, LBN_SELCHANGE), 0);
				SetFocus(dlgItem(IDC_FLAG_NAME));
				Edit_SetSel(dlgItem(IDC_FLAG_NAME), 0, Edit_GetTextLength(dlgItem(IDC_FLAG_NAME)));
				break;
			}
			case ID_FLAG_FILE:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = Value::object();
				flag[L"name"] = Value::string(L"name");
				flag[L"type"] = Value::string(L"file");
				flag[L"position"] = Value::number(0);
				_settings[L"tabs"][tab][L"flags"].push(flag);
				ListBox_AddString(dlgItem(IDC_FLAGS), L"name");
				ListBox_SetCurSel(dlgItem(IDC_FLAGS), _settings[L"tabs"][tab][L"flags"].length() - 1);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FLAGS, LBN_SELCHANGE), 0);
				SetFocus(dlgItem(IDC_FLAG_NAME));
				Edit_SetSel(dlgItem(IDC_FLAG_NAME), 0, Edit_GetTextLength(dlgItem(IDC_FLAG_NAME)));
				break;
			}
			case IDC_FLAG_REMOVE:
			{
				auto list = dlgItem(IDC_FLAGS);
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto sel = ListBox_GetCurSel(list);
				if (_settings[L"tabs"][tab][L"flags"][sel][L"type"].asString() != L"app")
				{
					if (MessageBox(hwndDlg, L"Remove flag?", L"Remove flag", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
					{
						ListBox_DeleteString(list, sel);
						_settings[L"tabs"][tab][L"flags"].remove(size_t(sel));
						if (sel < ListBox_GetCount(list) - 1)
						{
							ListBox_SetCurSel(list, sel);
						}
						else
						{
							ListBox_SetCurSel(list, sel - 1);
						}
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FLAGS, LBN_SELCHANGE), 0);
					}
				}
				break;
			}
			case IDC_FLAG_UP:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto list = dlgItem(IDC_FLAGS);
				auto sel = ListBox_GetCurSel(list);
				if (sel > 0)
				{
					std::swap(_settings[L"tabs"][tab][L"flags"][sel - 1], _settings[L"tabs"][tab][L"flags"][sel]);
					ListBox_DeleteString(list, sel - 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][tab][L"flags"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel - 1);
					if (sel == 1)
					{
						Button_Enable(dlgItem(IDC_FLAG_UP), FALSE);
					}
					Button_Enable(dlgItem(IDC_FLAG_DOWN), TRUE);
				}
				break;
			}
			case IDC_FLAG_DOWN:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto list = dlgItem(IDC_FLAGS);
				auto sel = ListBox_GetCurSel(list);
				auto count = ListBox_GetCount(list);
				if (sel + 1 < count)
				{
					std::swap(_settings[L"tabs"][tab][L"flags"][sel + 1], _settings[L"tabs"][tab][L"flags"][sel]);
					ListBox_DeleteString(list, sel + 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][tab][L"flags"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel + 1);
					if (sel + 2 >= count)
					{
						Button_Enable(dlgItem(IDC_FLAG_DOWN), FALSE);
					}
					Button_Enable(dlgItem(IDC_FLAG_UP), TRUE);
				}
				break;
			}
			case IDC_VALUES:
				switch (HIWORD(wParam))
				{
				case LBN_SELCHANGE:
					auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
					auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
					auto value = ListBox_GetCurSel(dlgItem(IDC_VALUES));
					if (tab >= 0 && flag >= 0 && value >= 0)
					{
						Edit_SetText(dlgItem(IDC_VALUE_NAME), _settings[L"tabs"][tab][L"flags"][flag][L"values"][value][L"name"].asString().c_str());
						Edit_SetText(dlgItem(IDC_VALUE_VALUE), _settings[L"tabs"][tab][L"flags"][flag][L"values"][value][L"value"].asString().c_str());
					}
					else
					{
						Edit_SetText(dlgItem(IDC_VALUE_NAME), L"");
						Edit_SetText(dlgItem(IDC_VALUE_VALUE), L"");
					}
					Button_Enable(dlgItem(IDC_VALUE_UP), value > 0);
					Button_Enable(dlgItem(IDC_VALUE_DOWN), value + 1 < ListBox_GetCount(dlgItem(IDC_VALUES)));
					Button_Enable(dlgItem(IDC_VALUE_REMOVE), ListBox_GetCount(dlgItem(IDC_VALUES)) > 1);
					bool isApp = _settings[L"tabs"][tab][L"flags"][flag][L"type"].asString() == L"app";
					if (isApp)
					{
						Edit_SetText(dlgItem(IDC_ARGS), _settings[L"tabs"][tab][L"flags"][flag][L"values"][value][L"args"].asString().c_str());
						ComboBox_SetText(dlgItem(IDC_VERB), _settings[L"tabs"][tab][L"flags"][flag][L"values"][value][L"verb"].asString().c_str());
						Button_SetCheck(dlgItem(IDC_HIDDEN), _settings[L"tabs"][tab][L"flags"][flag][L"values"][value][L"hidden"].asNumber() != 0);
					}
					break;
				}
				break;
			case IDC_VALUE_ADD:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				auto value = Value::object();
				value[L"name"] = Value::string(L"name");
				value[L"value"] = Value::string(L"");
				_settings[L"tabs"][tab][L"flags"][flag].setDefault(L"values", Value::array()).push(value);
				ListBox_AddString(dlgItem(IDC_VALUES), L"name");
				ListBox_SetCurSel(dlgItem(IDC_VALUES), _settings[L"tabs"][tab][L"flags"][flag][L"values"].length() - 1);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_VALUES, LBN_SELCHANGE), 0);
				SetFocus(dlgItem(IDC_VALUE_NAME));
				Edit_SetSel(dlgItem(IDC_VALUE_NAME), 0, Edit_GetTextLength(dlgItem(IDC_VALUE_NAME)));
				break;
			}
			case IDC_VALUE_REMOVE:
			{
				auto list = dlgItem(IDC_VALUES);
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				auto sel = ListBox_GetCurSel(list);
				if (ListBox_GetCount(list) > 1)
				{
					if (MessageBox(hwndDlg, L"Remove value?", L"Remove value", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
					{
						ListBox_DeleteString(list, sel);
						_settings[L"tabs"][tab][L"flags"][flag][L"values"].remove(size_t(sel));
						if (sel < ListBox_GetCount(list) - 1)
						{
							ListBox_SetCurSel(list, sel);
						}
						else
						{
							ListBox_SetCurSel(list, sel - 1);
						}
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_VALUES, LBN_SELCHANGE), 0);
					}
				}
				break;
			}
			case IDC_VALUE_UP:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				if (sel > 0)
				{
					std::swap(_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel], _settings[L"tabs"][tab][L"flags"][flag][L"values"][sel - 1]);
					ListBox_DeleteString(list, sel - 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel - 1);
					if (sel == 1)
					{
						Button_Enable(dlgItem(IDC_VALUE_UP), FALSE);
					}
					Button_Enable(dlgItem(IDC_VALUE_DOWN), TRUE);
				}
				break;
			}
			case IDC_VALUE_DOWN:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				auto count = ListBox_GetCount(list);
				if (sel + 1 < count)
				{
					std::swap(_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel], _settings[L"tabs"][tab][L"flags"][flag][L"values"][sel + 1]);
					ListBox_DeleteString(list, sel + 1);
					ListBox_InsertString(list, sel, _settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel + 1);
					if (sel + 2 >= count)
					{
						Button_Enable(dlgItem(IDC_VALUE_DOWN), FALSE);
					}
					Button_Enable(dlgItem(IDC_VALUE_UP), TRUE);
				}
				break;
			}
			case IDC_VALUE_NAME:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_VALUE_NAME), name, 512);
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					ListBox_DeleteString(list, sel);
					ListBox_InsertString(list, sel, name);
					ListBox_SetCurSel(list, sel);
					_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"name"] = Value::string(name);
				}
				break;
			}
			case IDC_VALUE_VALUE:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_VALUE_VALUE), name, 512);
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"value"] = Value::string(name);
				break;
			}
			case IDC_ARGS:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_ARGS), name, 512);
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"args"] = Value::string(name);
				break;
			}
			case IDC_VERB:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				wchar_t verb[512];
				Edit_GetText(dlgItem(IDC_VERB), verb, 512);
				auto list = dlgItem(IDC_VALUES);
				auto sel = ListBox_GetCurSel(list);
				_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"verb"] = Value::string(verb);
				break;
			}
			case IDC_HIDDEN:
			{
				auto tab = ListBox_GetCurSel(dlgItem(IDC_TABS));
				auto flag = ListBox_GetCurSel(dlgItem(IDC_FLAGS));
				auto sel = ListBox_GetCurSel(dlgItem(IDC_VALUES));
				_settings[L"tabs"][tab][L"flags"][flag][L"values"][sel][L"hidden"] = Value::number(Button_GetCheck(dlgItem(IDC_HIDDEN)) ? 1.0 : 0.0);
				break;
			}
			}
			break;
		}
		return FALSE;
	}

	BOOL CALLBACK MenuProc(HWND hwndDlg,
		UINT message,
		WPARAM wParam,
		LPARAM lParam) {
		auto dlgItem = [=](int control) { return GetDlgItem(hwndDlg, control); };
		switch (message)
		{
		case WM_INITDIALOG:
		{
			ComboBox_AddString(dlgItem(IDC_MENU_VERB), L"");
			ComboBox_AddString(dlgItem(IDC_MENU_VERB), L"open");
			ComboBox_AddString(dlgItem(IDC_MENU_VERB), L"print");
			ComboBox_AddString(dlgItem(IDC_MENU_VERB), L"runas");

			auto menu = _settings[L"menu"];
			auto menuWnd = dlgItem(IDC_MENU_ITEMS);
			for (auto item : menu)
			{
				ListBox_AddString(menuWnd, item[L"name"].asString().c_str());
			}
			ListBox_SetCurSel(menuWnd, 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_MENU_ITEMS, LBN_SELCHANGE), 0);
			Button_Enable(dlgItem(IDC_MENU_REMOVE), menu.length() > 0);
			Edit_Enable(dlgItem(IDC_MENU_NAME), menu.length() > 0);
			Edit_Enable(dlgItem(IDC_MENU_PATH), menu.length() > 0);
			Edit_Enable(dlgItem(IDC_MENU_ARGS), menu.length() > 0);
			ComboBox_Enable(dlgItem(IDC_MENU_VERB), menu.length() > 0);
			Button_Enable(dlgItem(IDC_MENU_HIDDEN), menu.length() > 0);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_MENU_ITEMS:
				switch (HIWORD(wParam))
				{
				case LBN_SELCHANGE:
					auto sel = ListBox_GetCurSel(dlgItem(IDC_MENU_ITEMS));
					Edit_SetText(dlgItem(IDC_MENU_NAME), _settings[L"menu"][sel][L"name"].asString().c_str());
					Edit_SetText(dlgItem(IDC_MENU_PATH), _settings[L"menu"][sel][L"path"].asString().c_str());
					Edit_SetText(dlgItem(IDC_MENU_ARGS), _settings[L"menu"][sel][L"args"].asString().c_str());
					ComboBox_SetText(dlgItem(IDC_MENU_VERB), _settings[L"menu"][sel][L"verb"].asString().c_str());
					Button_SetCheck(dlgItem(IDC_MENU_HIDDEN), _settings[L"menu"][sel][L"hidden"].asNumber() != 0);
					Button_Enable(dlgItem(IDC_MENU_UP), sel > 0);
					Button_Enable(dlgItem(IDC_MENU_DOWN), sel + 1 < ListBox_GetCount(dlgItem(IDC_MENU_ITEMS)));
					break;
				}
				break;
			case IDC_MENU_NAME:
			{
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_MENU_NAME), name, 512);
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					ListBox_DeleteString(list, sel);
					ListBox_InsertString(list, sel, name);
					ListBox_SetCurSel(list, sel);
					_settings[L"menu"][sel][L"name"] = Value::string(name);
				}
				break;
			}
			case IDC_MENU_PATH:
			{
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_MENU_PATH), name, 512);
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					_settings[L"menu"][sel][L"path"] = Value::string(name);
				}
				break;
			}
			case IDC_MENU_ARGS:
			{
				wchar_t name[512];
				Edit_GetText(dlgItem(IDC_MENU_ARGS), name, 512);
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					_settings[L"menu"][sel][L"args"] = Value::string(name);
				}
				break;
			}
			case IDC_MENU_VERB:
			{
				wchar_t name[512];
				ComboBox_GetText(dlgItem(IDC_MENU_VERB), name, 512);
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					_settings[L"menu"][sel][L"verb"] = Value::string(name);
				}
				break;
			}
			case IDC_MENU_HIDDEN:
			{
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel >= 0)
				{
					_settings[L"menu"][sel][L"hidden"] = Value::number(Button_GetCheck(dlgItem(IDC_MENU_HIDDEN)) ? 1 : 0);
				}
				break;
			}
			case IDC_MENU_ADD:
			{
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto index = ListBox_AddString(list, L"");
				auto item = Value::object();
				item[L"name"] = Value::string(L"");
				item[L"path"] = Value::string(L"");
				if (!_settings.has(L"menu"))
				{
					_settings[L"menu"] = Value::array();
				}
				_settings[L"menu"].push(item);
				ListBox_SetCurSel(list, index);
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_MENU_ITEMS, LBN_SELCHANGE), 0);
				Button_Enable(dlgItem(IDC_MENU_REMOVE), TRUE);
				Edit_Enable(dlgItem(IDC_MENU_NAME), TRUE);
				Edit_Enable(dlgItem(IDC_MENU_PATH), TRUE);
				Edit_Enable(dlgItem(IDC_MENU_ARGS), TRUE);
				ComboBox_Enable(dlgItem(IDC_MENU_VERB), TRUE);
				Button_Enable(dlgItem(IDC_MENU_HIDDEN), TRUE);
				SetFocus(dlgItem(IDC_MENU_NAME));
				Edit_SetSel(dlgItem(IDC_MENU_NAME), 0, Edit_GetTextLength(dlgItem(IDC_MENU_NAME)));
				break;
			}
			case IDC_MENU_REMOVE:
			{
				auto count = _settings[L"menu"].length();
				if (count)
				{
					if (MessageBox(hwndDlg, L"Remove menu item?", L"Remove menu item", MB_OKCANCEL | MB_ICONWARNING) == IDOK)
					{
						auto list = dlgItem(IDC_MENU_ITEMS);
						auto sel = ListBox_GetCurSel(list);
						ListBox_DeleteString(list, sel);
						_settings[L"menu"].remove(size_t(sel));
						if (sel < int(count) - 1)
						{
							ListBox_SetCurSel(list, sel);
						}
						else
						{
							ListBox_SetCurSel(list, sel - 1);
						}
						SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_MENU_ITEMS, LBN_SELCHANGE), 0);
						Button_Enable(dlgItem(IDC_MENU_REMOVE), count > 1);
						Edit_Enable(dlgItem(IDC_MENU_NAME), count > 1);
						Edit_Enable(dlgItem(IDC_MENU_PATH), count > 1);
						Edit_Enable(dlgItem(IDC_MENU_ARGS), count > 0);
						ComboBox_Enable(dlgItem(IDC_MENU_VERB), count > 0);
						Button_Enable(dlgItem(IDC_MENU_HIDDEN), count > 0);
					}
				}
				break;
			}
			case IDC_MENU_UP:
			{
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				if (sel > 0)
				{
					std::swap(_settings[L"menu"][sel], _settings[L"menu"][sel - 1]);
					ListBox_DeleteString(list, sel - 1);
					ListBox_InsertString(list, sel, _settings[L"menu"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel - 1);
					if (sel == 1)
					{
						Button_Enable(dlgItem(IDC_MENU_UP), FALSE);
					}
					Button_Enable(dlgItem(IDC_MENU_DOWN), TRUE);
				}
				break;
			}
			case IDC_MENU_DOWN:
			{
				auto list = dlgItem(IDC_MENU_ITEMS);
				auto sel = ListBox_GetCurSel(list);
				auto count = ListBox_GetCount(list);
				if (sel + 1 < count)
				{
					std::swap(_settings[L"menu"][sel], _settings[L"menu"][sel + 1]);
					ListBox_DeleteString(list, sel + 1);
					ListBox_InsertString(list, sel, _settings[L"menu"][sel][L"name"].asString().c_str());
					ListBox_SetCurSel(list, sel + 1);
					if (sel + 2 >= count)
					{
						Button_Enable(dlgItem(IDC_MENU_DOWN), FALSE);
					}
					Button_Enable(dlgItem(IDC_MENU_UP), TRUE);
				}
				break;
			}
			default:
				return FALSE;
			}
			return TRUE;
		}
		return FALSE;
	}
}

bool ShowSettingsDialog(HWND parent, SettingsPage defaultPage)
{
	_settings = parse(dump(settings::settings()).c_str());

	PROPSHEETPAGE page;
	ZeroMemory(&page, sizeof(page));
	page.dwSize = sizeof(page);
	page.dwFlags = PSP_DEFAULT;
	page.hInstance = GetModuleHandle(nullptr);
	page.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
	page.pfnDlgProc = &GeneralProc;

	HPROPSHEETPAGE pages[3];
	pages[0] = CreatePropertySheetPage(&page);
	page.pszTemplate = MAKEINTRESOURCE(IDD_CONTENT);
	page.pfnDlgProc = &ContentProc;
	pages[1] = CreatePropertySheetPage(&page);
	page.pszTemplate = MAKEINTRESOURCE(IDD_MENU);
	page.pfnDlgProc = &MenuProc;
	pages[2] = CreatePropertySheetPage(&page);

	PROPSHEETHEADER header;
	ZeroMemory(&header, sizeof(header));
	header.dwSize = sizeof(header);
	header.hwndParent = parent;
	header.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	header.hInstance = GetModuleHandle(nullptr);
	header.nPages = 3;
	header.phpage = pages;
	header.pszCaption = L"Settings";
	header.nStartPage = defaultPage;
	if (PropertySheet(&header) > 0)
	{
		settings::setSettings(_settings);
		return true;
	}
	return false;
}