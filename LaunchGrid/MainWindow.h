#pragma once

class Table;

class MainWindow
{
public:
	MainWindow(HINSTANCE instance);
	~MainWindow();

private:
	static ATOM registerWindowClass(HINSTANCE instance, const wchar_t* name);
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT wndProc(UINT message, WPARAM wParam, LPARAM lParam);
	void refresh();
	void applySettings();
	HWND createMainWindow();
	void createControls();
	void switchTab();
	HMENU createMenu();
	void registerAutorun();
	void unregisterAutorun();
	void createFonts();

	HINSTANCE m_instance;
	HWND m_wnd;
	HWND m_titleLabel;
	HWND m_menu;
	HWND m_emptyLink;
	std::vector<HWND> m_tabs;
	HWND m_parent;
	bool m_inDialog;

	HFONT m_titleFont;
	HFONT m_tabFont;

	std::vector<Table*> m_tables;

	struct MenuItem
	{
		std::wstring path;
		std::wstring args;
		std::wstring verb;
		bool hidden;
	};
	std::vector<MenuItem> m_menuItems;
};