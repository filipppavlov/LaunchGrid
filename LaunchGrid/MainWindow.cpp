#include "stdafx.h"
#include "MainWindow.h"
#include "resource.h"
#include "Settings.h"
#include "Table.h"
#include "Themes.h"
#include "SettingsDialog.h"
#include "Logging.h"


namespace
{
const int IDC_MENU = 1001;
const int IDC_TAB = 1002;
const int IDC_EMPTY_LINK = 1003;
const int ID_USER_MENU = 1100;
const int MAX_LOADSTRING = 100;

BOOL CALLBACK findDefView(HWND wnd, LPARAM param)
{
	wchar_t name[512];
	GetClassName(wnd, name, 512);
	if (wcscmp(name, L"WorkerW") == 0)
	{
		auto child = FindWindowEx(wnd, nullptr, L"SHELLDLL_DefView", nullptr);
		if (child)
		{
			*reinterpret_cast<HWND*>(param) = child;
			return FALSE;
		}
	}
	return TRUE;
}

HWND getRealDesktop()
{
	auto progman = FindWindow(L"Progman", nullptr);
	if (progman)
	{
		return progman;
	}
	auto explorer = FindWindow(L"WorkerW", nullptr);
	EnumWindows(&findDefView, reinterpret_cast<LPARAM>(&explorer));
	return explorer;
}

HWND createTransparentWindow(HINSTANCE hInstance, int nCmdShow, const wchar_t* className)
{
	auto parent = getRealDesktop();
	auto wnd = CreateWindowEx(WS_EX_NOACTIVATE, className, L"", WS_POPUP,
		0, 0, 100, 100, parent, nullptr, hInstance, nullptr);
	SetParent(wnd, parent);
	return wnd;
}

INT_PTR CALLBACK about(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CLICK:
		case NM_RETURN:
		{
			PNMLINK pNMLink = (PNMLINK)lParam;
			LITEM   item = pNMLink->item;
			ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
			break;
		}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

std::wstring loadString(HINSTANCE instance, int id)
{
	const wchar_t* p = nullptr;
	int len = LoadString(instance, id, reinterpret_cast<LPWSTR>(&p), 0);
	if (len > 0) 
	{
		return std::wstring(p, size_t(len));
	}
	else 
	{
		return std::wstring();
	}
}

}

MainWindow::MainWindow(HINSTANCE instance)
:	m_inDialog(false),
	m_parent(nullptr),
	m_instance(instance),
	m_emptyLink(nullptr)
{
	enableLogging(settings::settings()[L"general"][L"log"].asNumber() != 0);

	LOG(L"creating MainWindow");

	registerWindowClass(instance, loadString(instance, IDC_LAUNCHGRID).c_str());

	m_wnd = createMainWindow();

	createFonts();

	LOG(L"creating static controls");
	m_titleLabel = CreateWindow(L"STATIC", loadString(instance, IDS_APP_TITLE).c_str(), WS_CHILD | WS_VISIBLE, 8, 4, 120, 30, m_wnd, nullptr, instance, nullptr);
	SendMessage(m_titleLabel, WM_SETFONT, (WPARAM)m_titleFont, 0);

	m_menu = CreateWindow(L"BUTTON", L"Menu", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 240 - 8 - 24, 4, 24, 24, m_wnd, nullptr, instance, nullptr);
	SetWindowLong(m_menu, GWL_ID, IDC_MENU);

	createControls();

	LOG(L"showing window");
	ShowWindow(m_wnd, settings::settings()[L"general"][L"bottomWindow"].asNumber() ? SW_SHOWNOACTIVATE : SW_SHOW);
	if (settings::settings()[L"general"][L"bottomWindow"].asNumber())
	{
		LOG(L"moving window to background");
		SetWindowPos(m_wnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	}
	UpdateWindow(m_wnd);
}

void MainWindow::createFonts()
{
	NONCLIENTMETRICS metrics;
	ZeroMemory(&metrics, sizeof(metrics));
	metrics.cbSize = sizeof(metrics);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0);
	metrics.lfCaptionFont.lfHeight = -20;
	metrics.lfCaptionFont.lfWeight = FW_BOLD;
	m_titleFont = CreateFontIndirect(&metrics.lfCaptionFont);

	metrics.lfCaptionFont.lfHeight = 18;
	metrics.lfCaptionFont.lfWeight = FW_BOLD;
	m_tabFont = CreateFontIndirect(&metrics.lfCaptionFont);
}

HWND MainWindow::createMainWindow()
{
	auto windowClass = loadString(m_instance, IDC_LAUNCHGRID);

	DWORD exStyle = 0;
	if (settings::settings()[L"general"][L"bottomWindow"].asNumber())
	{
		LOG(L"creating on-desktop window");
		m_parent = createTransparentWindow(m_instance, SW_SHOWNOACTIVATE, windowClass.c_str());
	}
	else
	{
		LOG(L"creating topmost window");
		exStyle = WS_EX_WINDOWEDGE | WS_EX_TOPMOST;
		m_parent = nullptr;
	}
	LOG(L"window position: %i, %i", int(settings::cache()[L"left"].asNumber(10)), int(settings::cache()[L"top"].asNumber(10)));
	auto wnd = CreateWindowEx(exStyle, windowClass.c_str(), loadString(m_instance, IDS_APP_TITLE).c_str(), WS_POPUP,
		int(settings::cache()[L"left"].asNumber(10)), int(settings::cache()[L"top"].asNumber(10)), 240, 120, m_parent, nullptr, m_instance, nullptr);
	LOG(L"window created: %x", unsigned(wnd));
	SetWindowLong(wnd, GWL_USERDATA, reinterpret_cast<LONG>(this));
	return wnd;
}

void MainWindow::createControls()
{
	LOG(L"creating tabs");

	int yOffset = 40;

	if (settings::settings()[L"tabs"].length() == 0)
	{
		if (!m_emptyLink)
		{
			LOG(L"creating \"no tabs\" link");
			m_emptyLink = CreateWindow(L"BUTTON", loadString(m_instance, IDS_EMPTY_LINK).c_str(), 
				WS_CHILD | WS_VISIBLE | BS_COMMANDLINK, 4, yOffset + 10, 232, 50, m_wnd, nullptr, m_instance, 0);
			SetWindowLong(m_emptyLink, GWL_ID, IDC_EMPTY_LINK);
			SendMessage(m_emptyLink, BCM_SETNOTE, 0, (LPARAM)loadString(m_instance, IDS_EMPTY_LINK_NOTE).c_str());
		}
		return;
	}

	if (m_emptyLink)
	{
		LOG(L"destroying \"no tabs\" link");
		DestroyWindow(m_emptyLink);
		m_emptyLink = nullptr;
	}

	auto tabs = settings::settings()[L"tabs"];

	if (tabs.length() > 1)
	{
		yOffset = 70;
	}

	int tabWidth = 0;
	HDC dc = GetDC(m_wnd);
	for (auto tab : tabs)
	{
		auto name = tab[L"name"].asString();
		RECT r = { 0 };
		DrawText(dc, name.c_str(), -1, &r, DT_CALCRECT | DT_SINGLELINE);
		tabWidth = std::max(tabWidth, int(r.right));
	}
	tabWidth += 20;

	LOG(L"tab button width: %i", tabWidth);

	size_t selectedTab = size_t(settings::cache()[L"tab"].asNumber());
	if (selectedTab >= tabs.length())
	{
		selectedTab = 0;
	}

	for (size_t i = 0; i < tabs.length(); ++i)
	{
		if (tabs.length() > 1)
		{
			auto name = tabs[i][L"name"].asString();
			LOG(L"ceating tab button %ls", name.c_str());
			auto tab = CreateWindow(WC_BUTTON, name.c_str(), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_CENTER | BS_FLAT | BS_PUSHLIKE | BS_TEXT, 
				10 + i * (tabWidth + 4), 40, tabWidth, 24, m_wnd, nullptr, m_instance, nullptr);
			SetWindowLong(tab, GWL_ID, IDC_TAB);
			SendMessage(tab, WM_SETFONT, (WPARAM)m_tabFont, 0);
			m_tabs.push_back(tab);
			if (i == selectedTab)
			{
				Button_SetCheck(tab, 1);
			}
		}

		LOG(L"ceating table");
		auto table = new Table(m_instance, m_wnd, i, 8, yOffset);
		LOG(L"table created");
		m_tables.push_back(table);
		if (i == selectedTab)
		{
			table->show(true);
		}
	}

	switchTab();
}

void MainWindow::switchTab()
{
	LOG(L"switching tab");
	size_t selected = 0;
	for (size_t i = 0; i < m_tabs.size(); ++i)
	{
		if (Button_GetCheck(m_tabs[i]))
		{
			selected = i;
			m_tables[i]->show(true);
		}
		else
		{
			m_tables[i]->show(false);
		}
	}
	LOG(L"selected tab: %i", int(selected));

	int width = std::max(m_tables[selected]->width(), 200);
	int height = m_tables[selected]->height();
	if (!m_tabs.empty())
	{
		RECT tab;
		GetWindowRect(m_tabs.back(), &tab);
		ScreenToClient(m_wnd, reinterpret_cast<LPPOINT>(&tab.right));
		width = std::max(width, int(tab.right));
	}

	RECT r = { 0, 0, width + 16, height + 44 };
	if (!m_tabs.empty())
	{
		r.bottom += 30;
	}
	AdjustWindowRectEx(&r, GetWindowLong(m_wnd, GWL_STYLE), FALSE, GetWindowLong(m_wnd, GWL_EXSTYLE));
	LOG(L"new size: %i x %i", int(r.right - r.left), int(r.bottom - r.top));
	SetWindowPos(m_wnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	RECT menuRect;
	GetClientRect(m_menu, &menuRect);
	SetWindowPos(m_menu, nullptr, width + 8 - (menuRect.right - menuRect.left), 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	settings::cache()[L"tab"] = simplejson::Value::number(selected);
	settings::saveCache();
}

ATOM MainWindow::registerWindowClass(HINSTANCE hInstance, const wchar_t* name)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
	wcex.lpfnWndProc = wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAUNCHGRID));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = name;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void MainWindow::refresh()
{
	for (auto tab : m_tabs)
	{
		DestroyWindow(tab);
	}
	m_tabs.clear();
	for (auto table : m_tables)
	{
		delete table;
	}
	m_tables.clear();

	createControls();
}

void MainWindow::registerAutorun()
{
	const DWORD size = 1024;
	wchar_t path[size];
	auto result = GetModuleFileName(m_instance, path, size);
	if (result && result < size)
	{
		HKEY key = nullptr;
		RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &key);
		if (key)
		{
			RegSetValueEx(key, L"LaunchGrid", 0, REG_SZ, (BYTE*)path, (wcslen(path) + 1) * 2);
			RegCloseKey(key);
		}
	}
}

void MainWindow::unregisterAutorun()
{
	const DWORD size = 1024;
	wchar_t path[size];
	auto result = GetModuleFileName(m_instance, path, size);
	if (result && result < size)
	{
		HKEY key = nullptr;
		RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &key);
		if (key)
		{
			RegDeleteValue(key, L"LaunchGrid");
			RegCloseKey(key);
		}
	}
}

void MainWindow::applySettings()
{
	enableLogging(settings::settings()[L"general"][L"log"].asNumber() != 0);
	for (auto tab : m_tabs)
	{
		DestroyWindow(tab);
	}
	m_tabs.clear();
	for (auto table : m_tables)
	{
		delete table;
	}
	m_tables.clear();

	if (settings::settings()[L"general"][L"bottomWindow"].asNumber())
	{
		if (!m_parent)
		{
			auto newWnd = createMainWindow();
			SetParent(m_titleLabel, newWnd);
			SetParent(m_menu, newWnd);
			DestroyWindow(m_wnd);
			m_wnd = newWnd;
			ShowWindow(m_wnd, SW_SHOW);
		}
		if (settings::settings()[L"general"][L"autoStart"])
		{
			registerAutorun();
		}
		else
		{
			unregisterAutorun();
		}
	}
	else
	{
		if (m_parent)
		{
			auto parent = m_parent;
			
			auto newWnd = createMainWindow();
			SetParent(m_titleLabel, newWnd);
			SetParent(m_menu, newWnd);
			DestroyWindow(m_wnd);
			m_wnd = newWnd;
			ShowWindow(m_wnd, SW_SHOW);

			DestroyWindow(parent);
		}
		unregisterAutorun();
	}

	createControls();
	InvalidateRect(m_wnd, nullptr, TRUE);
}

MainWindow::~MainWindow()
{

}

LRESULT CALLBACK MainWindow::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto data = GetWindowLong(hWnd, GWL_USERDATA);
	if (data)
	{
		return reinterpret_cast<MainWindow*>(data)->wndProc(message, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

HMENU MainWindow::createMenu()
{
	HMENU menu = GetSubMenu(LoadMenu(nullptr, MAKEINTRESOURCE(IDR_MENU)), 0);
	auto items = settings::settings()[L"menu"];
	size_t count = 0;
	Table* table = nullptr;
	for (auto t : m_tables)
	{
		if (t->isVisible())
		{
			table = t;
			break;
		}
	}
	m_menuItems.clear();
	for (auto item : items)
	{
		auto name = item[L"name"].asString();
		auto path = item[L"path"].asString();
		auto args = item[L"args"].asString();
		auto verb = item[L"verb"].asString();
		auto hidden = item[L"hidden"].asNumber() != 0;
		if (table)
		{
			name = table->expandString(name);
			path = table->expandString(path);
			args = table->expandString(args);
		}
		findFiles(name, L"MenuItem", path, false, [&](const std::wstring& name, const std::wstring& path) {
			MenuItem menuItem = { path, args, verb, hidden };
			m_menuItems.push_back(menuItem);
			MENUITEMINFO item = { 0 };
			item.cbSize = sizeof(item);
			item.fMask = MIIM_STRING | MIIM_ID | MIIM_DATA;
			item.fType = MFT_STRING;
			item.wID = ID_USER_MENU;
			item.dwItemData = DWORD(m_menuItems.size() - 1);
			item.dwTypeData = const_cast<LPWSTR>(name.c_str());
			item.cch = path.length();
			InsertMenuItem(menu, count++, TRUE, &item);
		});
	}
	if (count)
	{
		MENUITEMINFO item = { 0 };
		item.cbSize = sizeof(item);
		item.fMask = MIIM_FTYPE;
		item.fType = MFT_SEPARATOR;
		InsertMenuItem(menu, count, TRUE, &item);
	}

	MENUINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIM_STYLE;
	mi.dwStyle = MNS_NOTIFYBYPOS;
	SetMenuInfo(menu, &mi);

	return menu;
}

LRESULT MainWindow::wndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCHITTEST:
		return HTCAPTION;
	case WM_CTLCOLORSTATIC:
		SetTextColor(reinterpret_cast<HDC>(wParam), theme::color(theme::WINDOW_TITLE));
		SetBkColor(reinterpret_cast<HDC>(wParam), theme::color(theme::BACKGROUND));
		return (LRESULT)GetStockObject(NULL_BRUSH);
	case WM_CTLCOLORBTN:
		return (LRESULT)theme::brush(theme::BACKGROUND);
	case WM_ERASEBKGND:
	{
		HDC dc = HDC(wParam);
		RECT r;
		GetClientRect(m_wnd, &r);
		SelectObject(dc, theme::brush(theme::BACKGROUND));
		SelectObject(dc, theme::pen(theme::LINE));
		Rectangle(dc, r.left, r.top, r.right, r.bottom);
		return 1;
	}
	case WM_MENUCOMMAND:
	{
		HMENU menu = (HMENU)lParam;
		int idx = wParam;
		switch (GetMenuItemID(menu, idx))
		{
		case ID_USER_MENU:
		{
			MENUITEMINFO info;
			info.cbSize = sizeof(info);
			info.fMask = MIIM_DATA;
			GetMenuItemInfo(menu, idx, TRUE, &info);
			auto& item = m_menuItems[info.dwItemData];
			Table* table = nullptr;
			for (auto t : m_tables)
			{
				if (t->isVisible())
				{
					table = t;
					break;
				}
			}
			table->launch(item.verb.c_str(), item.path.c_str(), item.args.c_str(), false);
			break;
		}
		case ID_MENU_REFRESH:
			refresh();
			break;
		case IDM_ABOUT:
			m_inDialog = true;
			DialogBox(m_instance, MAKEINTRESOURCE(IDD_ABOUTBOX), m_wnd, about);
			m_inDialog = false;
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_SETTINGS:
		{
			m_inDialog = true;
			auto ok = ShowSettingsDialog(m_wnd);
			if (ok)
			{
				applySettings();
			}
			m_inDialog = false;
			break;
		}
		}
		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDC_MENU:
		{
			HMENU menu = createMenu(); 
			RECT rect;
			GetWindowRect(GetDlgItem(m_wnd, IDC_MENU), &rect);
			TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_TOPALIGN, rect.right, rect.bottom, 0, m_wnd, NULL);
			break;
		}
		case IDC_TAB:
			switchTab();
			return 0;
		case IDC_EMPTY_LINK:
		{
			m_inDialog = true;
			auto ok = ShowSettingsDialog(m_wnd, SETTINGS_CONTENT);
			if (ok)
			{
				applySettings();
			}
			m_inDialog = false;
			break;
		}
		default:
			return DefWindowProc(m_wnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DRAWITEM:
	{
		auto draw = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
		FillRect(draw->hDC, &draw->rcItem, theme::brush(theme::BACKGROUND));
		SelectPen(draw->hDC, theme::pen(theme::TEXT));
		SelectBrush(draw->hDC, theme::brush(theme::TEXT));
		const int radius = 4;
		Ellipse(draw->hDC, 0, 12, radius, 12 + radius);
		Ellipse(draw->hDC, 8, 12, radius + 8, 12 + radius);
		Ellipse(draw->hDC, 16, 12, radius + 16, 12 + radius);
		break;
	}
	case WM_ACTIVATEAPP:
		if (!m_inDialog && wParam == FALSE && settings::settings()[L"general"][L"bottomWindow"].asNumber() == 0)
		{
			PostQuitMessage(0);
		}
		break;
	case WM_MOVE:
	{
		RECT r;
		GetWindowRect(m_wnd, &r);
		settings::cache()[L"left"] = simplejson::Value::number(r.left);
		settings::cache()[L"top"] = simplejson::Value::number(r.top);
		settings::saveCache();
		break;
	}
	default:
		return DefWindowProc(m_wnd, message, wParam, lParam);
	}
	return 0;
}
