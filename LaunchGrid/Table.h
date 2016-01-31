#pragma once

#include "Data.h"

class Table
{
public:
	Table(HINSTANCE instance, HWND wnd, size_t tab, int x, int y);
	~Table();

	typedef void(*Callback)(Table* table);

	void onresize(Callback callback);

	int width() const;
	int height() const;
	void show(bool);
	bool isVisible() const;
	std::wstring expandString(std::wstring& string) const;

	void launch(const wchar_t* verb, const wchar_t* command, const wchar_t* arguments, bool hidden);
private:
	struct TableCell
	{
		size_t rowSpan;
		size_t columnSpan;
		bool columnHeader;
		bool rowHeader;
		BaseOption* headerFlag;
		size_t headerValue;
		TableCell* spanFrom;
		std::vector<size_t> values;
	};

	void createFonts();
	void registerClass();
	void loadOptions(size_t tab);
	void rebuildOptions();
	std::vector<BaseOption*> getOptions(OptionPosition position);
	std::map<BaseOption*, SIZE> getOptionSizes(HDC dc, SIZE& cellSize, const SIZE& padding);
	void buildTable();
	void fixSpans();
	void mergeCells();
	void renderTable();
	void cellClicked(int x, int y);
	void createLines(int* rowHeight, int* columnWidth);
	void createSelects(HDC dc);
	void getTableOptions(int x, int y, std::map<BaseOption*, size_t>& options);
	static LRESULT CALLBACK tableProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HINSTANCE m_instance;
	size_t m_tab;
	HWND m_tableWnd;
	HFONT m_linkFont;
	HFONT m_titleFont;
	HFONT m_selectFont;
	std::vector<std::unique_ptr<BaseOption>> m_options;
	std::unique_ptr<TableCell[]> m_table;
	size_t m_columnCount;
	size_t m_rowCount;
	Callback m_onResize;
	std::vector<std::pair<BaseOption*, HWND>> m_selects;
	std::vector<RECT> m_lines;
	int m_width;
	int m_height;
	size_t m_rowHeaders, m_columnHeaders;
};