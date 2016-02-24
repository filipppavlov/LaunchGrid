#include "stdafx.h"
#include "Table.h"
#include "Settings.h"
#include "Themes.h"

namespace
{
	const DWORD ID_LAUNCH = 2000;
	const DWORD ID_COMBO = 2001;

	WNDPROC s_staticProc = nullptr;
	LRESULT CALLBACK LinkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_SETCURSOR:
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			return TRUE;
		default:
			return s_staticProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

}

Table::Table(HINSTANCE instance, HWND wnd, size_t tab, int x, int y)
	:m_instance(instance),
	m_tab(tab),
	m_onResize(nullptr),
	m_width(0),
	m_height(0)
{
	registerClass();

	m_tableWnd = CreateWindow(L"table", L"table", WS_CHILD, x, y, 200, 300, wnd, nullptr, instance, nullptr);

	createFonts();
	loadOptions(tab);
	rebuildOptions();
	buildTable();
	fixSpans();
	mergeCells();
	renderTable();

	SetWindowPos(m_tableWnd, nullptr, 0, 0, m_width, m_height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (m_onResize)
	{
		(*m_onResize)(this);
	}
}

Table::~Table()
{
	DestroyWindow(m_tableWnd);
	DeleteFont(m_titleFont);
	DeleteFont(m_linkFont);
	DeleteFont(m_selectFont);
}

void Table::createFonts()
{
	NONCLIENTMETRICS metrics;
	ZeroMemory(&metrics, sizeof(metrics));
	metrics.cbSize = sizeof(metrics);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0);
	metrics.lfCaptionFont.lfHeight = 15;
	m_selectFont = CreateFontIndirect(&metrics.lfCaptionFont);
	metrics.lfCaptionFont.lfUnderline = 1;
	m_linkFont = CreateFontIndirect(&metrics.lfCaptionFont);
	metrics.lfCaptionFont.lfUnderline = 0;
	metrics.lfCaptionFont.lfWeight = FW_BOLD;
	metrics.lfCaptionFont.lfHeight = 15;
	m_titleFont = CreateFontIndirect(&metrics.lfCaptionFont);
}

void Table::onresize(Callback callback)
{
	m_onResize = callback;
	if (m_onResize)
	{
		(*m_onResize)(this);
	}
}

int Table::width() const
{
	return m_width;
}

int Table::height() const
{
	return m_height;
}

void Table::show(bool show)
{
	ShowWindow(m_tableWnd, show ? SW_SHOWNA : SW_HIDE);
}

bool Table::isVisible() const
{
	return (GetWindowLong(m_tableWnd, GWL_STYLE) & WS_VISIBLE) != 0;
}

std::wstring Table::expandString(std::wstring& string) const
{
	std::map<BaseOption*, size_t> options;
	for (auto p : m_selects)
	{
		options[p.first] = ComboBox_GetCurSel(p.second);
	}
	return BaseOption::expandString(string, options);
}

void Table::registerClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &tableProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_instance;
	wcex.hIcon = nullptr;
	wcex.hCursor = nullptr;
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"table";
	wcex.hIconSm = nullptr;

	RegisterClassExW(&wcex);
}

void Table::loadOptions(size_t tab)
{
	for (auto flag : settings::settings()[L"tabs"][tab][L"flags"])
	{
		m_options.push_back(std::unique_ptr<BaseOption>(BaseOption::create(flag)));
	}
}

void Table::rebuildOptions()
{
	std::vector<size_t> remove;
	for (size_t i = 0; i < m_options.size(); ++i)
	{
		auto& o = m_options[i];
		o->prepareValues();
		if (!o->valueCount())
		{
			remove.push_back(i);
		}
	}
	for (auto it = remove.rbegin(); it != remove.rend(); ++it)
	{
		m_options.erase(m_options.begin() + *it);
	}
}

std::vector<BaseOption*> Table::getOptions(OptionPosition position)
{
	std::vector<BaseOption*> result;
	for (auto& opt : m_options)
	{
		if (opt->position() == position)
		{
			result.push_back(opt.get());
		}
	}
	return std::move(result);
}

size_t valuePermutationCount(const std::vector<BaseOption*>& options)
{
	size_t result = 1;
	for (auto opt : options)
	{
		result *= opt->valueCount();
	}
	return result;
}

void Table::buildTable()
{
	std::vector<BaseOption*> columns = getOptions(OP_COLUMN), rows = getOptions(OP_ROW);

	m_columnCount = valuePermutationCount(columns) + rows.size();
	m_rowCount = valuePermutationCount(rows) + columns.size();
	m_table.reset(new TableCell[m_rowCount * m_columnCount]());

	size_t multiplier = m_columnCount - rows.size();
	for (size_t i = 0; i < columns.size(); ++i)
	{
		auto times = (m_columnCount - rows.size()) / multiplier;
		auto repeatSpan = (m_columnCount - rows.size()) / times;
		multiplier /= columns[i]->valueCount();
		for (size_t k = 0; k < times; ++k)
		{
			for (size_t j = 0; j < columns[i]->valueCount(); ++j)
			{
				auto& cell = m_table[i * m_columnCount + rows.size() + j * multiplier + k * repeatSpan];
				cell.columnHeader = true;
				cell.rowSpan = 1;
				cell.columnSpan = multiplier;
				cell.headerFlag = columns[i];
				cell.headerValue = j;
			}
		}
	}
	multiplier = m_rowCount - columns.size();
	for (size_t i = 0; i < rows.size(); ++i)
	{
		auto times = (m_rowCount - columns.size()) / multiplier;
		auto repeatSpan = (m_rowCount - columns.size()) / times;
		multiplier /= rows[i]->valueCount();
		for (size_t k = 0; k < times; ++k)
		{
			for (size_t j = 0; j < rows[i]->valueCount(); ++j)
			{
				auto& cell = m_table[i + (columns.size() + j * multiplier + k * repeatSpan) * m_columnCount];
				cell.rowHeader = true;
				cell.rowSpan = multiplier;
				cell.columnSpan = 1;
				cell.headerFlag = rows[i];
				cell.headerValue = j;
			}
		}
	}

	auto& cell = m_table[0];
	if (!columns.empty() && !rows.empty())
	{
		cell.rowSpan = std::max(columns.size(), size_t(1));
		cell.columnSpan = std::max(rows.size(), size_t(1));
	}

	m_rowHeaders = rows.size();
	m_columnHeaders = columns.size();


}

void Table::mergeCells()
{
	std::vector<BaseOption*> columns = getOptions(OP_COLUMN), rows = getOptions(OP_ROW);

	AppOption* app = nullptr;
	for (auto i : columns)
	{
		if (auto a = dynamic_cast<AppOption*>(i))
		{
			app = a;
			break;
		}
	}
	for (auto i : rows)
	{
		if (auto a = dynamic_cast<AppOption*>(i))
		{
			app = a;
			break;
		}
	}
	if (app)
	{
		for (size_t row = columns.size(); row < m_rowCount; ++row)
		{
			for (size_t column = rows.size(); column < m_columnCount; ++column)
			{
				auto& cell = m_table[column + row * m_columnCount];
				if (cell.spanFrom || cell.headerValue)
				{
					continue;
				}
				std::map<BaseOption*, size_t> options;
				getTableOptions(int(column), int(row), options);
				auto str0 = app->expandValue(options[app], options);
				auto arg0 = app->arguments(options[app], options);
				for (size_t c1 = column + 1; c1 < m_columnCount; ++c1)
				{
					options.clear();
					getTableOptions(int(c1), int(row), options);
					auto str1 = app->expandValue(options[app], options);
					auto arg1 = app->arguments(options[app], options);
					if (str0 != str1 || arg0 != arg1)
					{
						break;
					}
					cell.columnSpan += 1;
					m_table[c1 + row * m_columnCount].spanFrom = &cell;
				}
				for (size_t r1 = row + 1; r1 < m_rowCount; ++r1)
				{
					bool rowOk = true;
					for (size_t c1 = column; c1 < column + cell.columnSpan; ++c1)
					{
						options.clear();
						getTableOptions(int(c1), int(r1), options);
						auto str1 = app->expandValue(options[app], options);
						auto arg1 = app->arguments(options[app], options);
						if (str0 != str1 || arg0 != arg1)
						{
							rowOk = false;
							break;
						}
					}
					if (rowOk)
					{
						for (size_t c1 = column; c1 < column + cell.columnSpan; ++c1)
						{
							cell.rowSpan += 1;
							m_table[c1 + r1 * m_columnCount].spanFrom = &cell;
						}
						cell.rowSpan -= 1;
					}
				}
			}
		}
	}
}

void Table::fixSpans()
{
	for (size_t row = 0; row < m_rowCount; ++row)
	{
		for (size_t column = 0; column < m_columnCount; ++column)
		{
			auto& cell = m_table[column + row * m_columnCount];
			if (cell.rowSpan == 0)
			{
				cell.rowSpan = 1;
			}
			if (cell.columnSpan == 0)
			{
				cell.columnSpan = 1;
			}
			for (size_t y = 0; y < cell.rowSpan; ++y)
			{
				for (size_t x = 0; x < cell.columnSpan; ++x)
				{
					if (x || y)
					{
						m_table[column + x + (row + y) * m_columnCount].spanFrom = &cell;
					}
				}
			}
		}
	}
}

SIZE measureText(HDC dc, const wchar_t* text)
{
	RECT r = { 0 };
	DrawText(dc, text, -1, &r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
	return SIZE{ r.right, r.bottom };
}

SIZE operator+(SIZE x, SIZE y)
{
	return SIZE{ x.cx + y.cx, x.cy + y.cy };
}

SIZE operator*(SIZE x, int y)
{
	return SIZE{ x.cx * y, x.cy * y };
}

namespace std
{
	SIZE max(SIZE x, SIZE y)
	{
		return SIZE{ std::max(x.cx, y.cx), std::max(x.cy, y.cy) };
	}
}

std::map<BaseOption*, SIZE> Table::getOptionSizes(HDC dc, SIZE& cellSize, const SIZE& padding)
{
	std::map<BaseOption*, SIZE> optionSizes;

	int columnSpans = 1;
	int rowSpans = 1;

	for (size_t i = 0; i < m_options.size(); ++i)
	{
		auto& option = m_options[m_options.size() - 1 - i];
		if (option->position() != OP_ROW && option->position() != OP_COLUMN)
		{
			continue;
		}
		int& span = option->position() == OP_ROW ? columnSpans : rowSpans;

		SIZE size = cellSize;
		for (size_t j = 0; j < option->valueCount(); ++j)
		{
			size = std::max(measureText(dc, option->value(j).name.c_str()) + padding * 2, size);
		}
		optionSizes[option.get()] = size;
		if (option->position() == OP_ROW)
		{
			size.cy = size.cy / rowSpans + (size.cy % rowSpans ? 1 : 0);
			rowSpans *= option->valueCount();
			cellSize.cy = std::max(cellSize.cy, size.cy);
		}
		else
		{
			size.cx = size.cx / columnSpans + (size.cx % columnSpans ? 1 : 0);
			columnSpans *= option->valueCount();
			cellSize.cx = std::max(cellSize.cx, size.cx);
		}
	}
	return optionSizes;
}

void Table::renderTable()
{
	const SIZE padding = { 2, 2 };

	HDC dc = GetDC(m_tableWnd);

	std::unique_ptr<int[]> columnWidth( new int[m_columnCount]() );
	std::unique_ptr<int[]> rowHeight( new int[m_rowCount]() );

	SelectObject(dc, (HGDIOBJ)m_linkFont);
	SIZE cellSize = measureText(dc, L"Run") + padding * 2;
	SelectObject(dc, (HGDIOBJ)m_titleFont);

	std::map<BaseOption*, SIZE> optionSizes = getOptionSizes(dc, cellSize, padding);

	for (size_t row = 0; row < m_rowCount; ++row)
	{
		auto cell = &m_table[row * m_columnCount + m_columnCount - 1];
		if (cell->spanFrom)
		{
			cell = cell->spanFrom;
		}
		rowHeight[row] = cell->headerFlag ? optionSizes[cell->headerFlag].cy : cellSize.cy;
	}
	for (size_t column = 0; column < m_columnCount; ++column)
	{
		auto cell = &m_table[(m_rowCount - 1) * m_columnCount + column];
		if (cell->spanFrom)
		{
			cell = cell->spanFrom;
		}
		columnWidth[column] = cell->headerFlag ? optionSizes[cell->headerFlag].cx : cellSize.cx;
	}

	m_width = 0;
	m_height = 0;
	for (size_t row = 0; row < m_rowCount; ++row)
	{
		m_height += rowHeight[row];
	}
	for (size_t column = 0; column < m_columnCount; ++column)
	{
		m_width += columnWidth[column];
	}

	int top = 0;
	for (size_t row = 0; row < m_rowCount; ++row)
	{
		int left = 0;
		for (size_t column = 0; column < m_columnCount; ++column)
		{
			auto& cell = m_table[column + row * m_columnCount];
			if (!cell.spanFrom)
			{
				if (row || column || cell.headerFlag)
				{
					int width = 0;
					int height = 0;
					for (size_t y = 0; y < cell.rowSpan; ++y)
					{
						height += rowHeight[row + y];
					}
					for (size_t x = 0; x < cell.columnSpan; ++x)
					{
						width += columnWidth[column + x];
					}
					RECT r = { left, top, left + width, top + height };
					if (cell.headerFlag)
					{
						auto style = cell.rowHeader ? SS_RIGHT : SS_CENTER;
						auto c = CreateWindow(L"STATIC", cell.headerFlag ? cell.headerFlag->value(cell.headerValue).name.c_str() : L"Run", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | style, left + padding.cx, top + padding.cy, width - 2 * padding.cx, height - 2 * padding.cy, m_tableWnd, nullptr, m_instance, nullptr);
						SendMessage(c, WM_SETFONT, (WPARAM)m_titleFont, 0);
					}
					else
					{
						auto c = CreateWindow(L"BUTTON", cell.headerFlag ? cell.headerFlag->value(cell.headerValue).name.c_str() : L"Run", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_CENTER, left + padding.cx, top + padding.cy, width - 2 * padding.cx, height - 2 * padding.cy, m_tableWnd, nullptr, m_instance, nullptr);
						SetWindowLong(c, GWL_USERDATA, column + row * m_columnCount);
						s_staticProc = (WNDPROC)GetWindowLong(c, GWL_WNDPROC);
						SetWindowLong(c, GWL_WNDPROC, (LONG)&LinkProc);
						SetWindowLong(c, GWL_ID, ID_LAUNCH);
						SendMessage(c, WM_SETFONT, (WPARAM)m_linkFont, 0);
					}
				}
			}
			left += columnWidth[column];
		}
		top += rowHeight[row];
	}

	createLines(rowHeight.get(), columnWidth.get());
	createSelects(dc);

	ReleaseDC(m_tableWnd, dc);
	SetWindowLong(m_tableWnd, GWL_USERDATA, reinterpret_cast<LONG>(this));
}

void Table::createSelects(HDC dc)
{
	auto select = getOptions(OP_SELECT);
	if (!select.empty())
	{
		m_height += 18;
		auto top = m_height;

		LONG selectTitleWidth = 0;
		LONG selectWidth = 0;
		for (auto it : select)
		{
			SelectObject(dc, (HGDIOBJ)m_titleFont);
			selectTitleWidth = std::max(selectTitleWidth, measureText(dc, it->displayName()).cx);
			SelectObject(dc, (HGDIOBJ)m_linkFont);
			for (size_t j = 0; j < it->valueCount(); ++j)
			{
				selectWidth = std::max(selectWidth, measureText(dc, it->value(j).name.c_str()).cx);
			}
		}
		m_width = std::max(m_width, int(selectWidth + 40 + selectTitleWidth));
		RECT selectRect = { top, m_width - selectWidth - 16, top + 18, m_width };
		RECT selectTitleRect = { top, selectRect.left - 4 - selectTitleWidth, top + 18, selectRect.left - 4 };
		for (auto it : select)
		{
			auto c = CreateWindow(L"STATIC", it->displayName(), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_RIGHT, m_width - selectWidth - 40 - selectTitleWidth, top,
				selectTitleWidth, 24, m_tableWnd, nullptr, m_instance, nullptr);
			SendMessage(c, WM_SETFONT, (WPARAM)m_titleFont, 0);

			c = CreateWindow(L"COMBOBOX", it->displayName(), WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, m_width - selectWidth - 32, top,
				selectWidth + 32, 48, m_tableWnd, nullptr, m_instance, nullptr);
			m_selects.push_back(std::make_pair(it, c));
			SendMessage(c, WM_SETFONT, (WPARAM)m_selectFont, 0);
			auto selectedString = settings::cache()[L"tabSelects"][m_tab][it->displayName()].asString();
			bool foundSelected = false;
			for (size_t j = 0; j < it->valueCount(); ++j)
			{
				ComboBox_AddString(c, it->value(j).name.c_str());
				if (it->value(j).name == selectedString)
				{
					ComboBox_SetCurSel(c, j);
					foundSelected = true;
				}
			}
			if (!foundSelected)
			{
				ComboBox_SetCurSel(c, 0);
			}
			SetWindowLong(c, GWL_ID, ID_COMBO);

			top += 28;
			m_height += 28;
		}
	}
}

void Table::createLines(int* rowHeight, int* columnWidth)
{
	int left = 0;
	for (size_t column = 0; column < m_columnCount;)
	{
		auto&cell = m_table[column];
		for (size_t i = 0; i < cell.columnSpan; ++i)
		{
			left += columnWidth[column + i];
		}
		if (cell.headerFlag && cell.columnSpan > 1 && cell.columnSpan + column < m_columnCount && m_table[cell.columnSpan + column].headerFlag)
		{
			RECT r = { left, 0, left, m_height };
			m_lines.push_back(r);
		}
		column += cell.columnSpan;
	}
	int top = 0;
	for (size_t row = 0; row < m_rowCount;)
	{
		auto&cell = m_table[row * m_columnCount];
		for (size_t i = 0; i < cell.rowSpan; ++i)
		{
			top += rowHeight[row + i];
		}
		if (cell.headerFlag && cell.rowSpan > 1 && cell.rowSpan + row < m_rowCount && m_table[(cell.rowSpan + row) * m_columnCount].headerFlag)
		{
			RECT r = { 0, top, m_width, top };
			m_lines.push_back(r);
		}
		row += cell.rowSpan;
	}
}

void Table::getTableOptions(int x, int y, std::map<BaseOption*, size_t>& options)
{
	for (int i = x; i >= 0; --i)
	{
		auto cell = &m_table[i + y * m_columnCount];
		if (cell->spanFrom)
		{
			cell = cell->spanFrom;
		}
		if (cell->headerFlag)
		{
			options[cell->headerFlag] = cell->headerValue;
		}
	}
	for (int i = y; i >= 0; --i)
	{
		auto cell = &m_table[x + i * m_columnCount];
		if (cell->spanFrom)
		{
			cell = cell->spanFrom;
		}
		if (cell->headerFlag)
		{
			options[cell->headerFlag] = cell->headerValue;
		}
	}
}

void Table::launch(const wchar_t* verb, const wchar_t* command, const wchar_t* arguments, bool hidden)
{
	if (verb && !*verb)
	{
		verb = nullptr;
	}
	int showCommand;
	if (hidden)
	{
		showCommand = SW_HIDE;
	}
	else if (settings::settings()[L"general"][L"doNotClose"].asNumber())
	{
		showCommand = SW_SHOWNOACTIVATE;
	}
	else
	{
		showCommand = SW_SHOW;
	}

	auto fileName = PathFindFileName(command);
	std::wstring directory;
	if (fileName != command)
	{
		directory = std::wstring(command, fileName - command);
		if (!PathFileExists(directory.c_str()) || !PathIsDirectory(directory.c_str()))
		{
			directory.clear();
		}
	}
	auto result = ShellExecute(GetParent(m_tableWnd), verb, command, arguments, directory.empty() ? nullptr : directory.c_str(), showCommand);
	if (int(result) < 32)
	{
		LPTSTR error = NULL;

		// Search for the message description in the std windows
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&error,
			0,
			NULL);
		
		DWORD_PTR args[] = { (DWORD_PTR)command, (DWORD_PTR)arguments, (DWORD_PTR)(error ? error : L"Unknown error") };

		LPWSTR buffer = NULL;
		FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			L"Running file\n\n%1\n\nwith arguments:\n\n%2\n\nproduced a error: %3",
			0,
			0,
			(LPWSTR)&buffer,
			0,
			(va_list*)args);
		if (buffer)
		{
			MessageBox(GetParent(m_tableWnd), buffer, L"Error launching app", MB_ICONERROR);
			LocalFree(buffer);
		}
		else
		{
			MessageBox(GetParent(m_tableWnd), L"Unexpected error when launching the application", L"Error launching app", MB_ICONERROR);
		}
		if (error)
		{
			LocalFree(error);
		}
	}
}

void Table::cellClicked(int x, int y)
{
	std::map<BaseOption*, size_t> options;
	getTableOptions(x, y, options);
	for (auto p : m_selects)
	{
		options[p.first] = ComboBox_GetCurSel(p.second);
	}
	AppOption* app = nullptr;
	for (auto o : options)
	{
		if (auto a = dynamic_cast<AppOption*>(o.first))
		{
			auto cmdLine = a->expandValue(o.second, options);
			launch(a->verb(o.second).c_str(), cmdLine.c_str(), a->arguments(o.second, options).c_str(), a->runHidden(o.second));
			break;
		}
	}
}

void Table::onPaint()
{
	PAINTSTRUCT ps;
	auto hdc = BeginPaint(m_tableWnd, &ps);

	SelectObject(hdc, theme::pen(theme::LINE));
	for (auto& r : m_lines)
	{
		MoveToEx(hdc, r.left, r.top, nullptr);
		LineTo(hdc, r.right, r.bottom);
	}
	EndPaint(m_tableWnd, &ps);
}

void Table::onLaunch(HWND button)
{
	auto userData = GetWindowLong(button, GWL_USERDATA);
	auto x = userData % m_columnCount;
	auto y = userData / m_columnCount;
	cellClicked(x, y);
}

void Table::onComboChange()
{
	auto selects = simplejson::Value::object();
	for (auto p : m_selects)
	{
		selects[p.first->displayName()] = simplejson::Value::string(p.first->value(ComboBox_GetCurSel(p.second)).name.c_str());
	}
	settings::cache().setDefault(L"tabSelects", simplejson::Value::array())[m_tab] = selects;
	settings::saveCache();
}

void Table::onDrawButton(const DRAWITEMSTRUCT* draw)
{
	auto length = GetWindowTextLength(draw->hwndItem);
	wchar_t shortBuffer[256];
	wchar_t* longBuffer = nullptr;
	wchar_t* buffer;
	if (length >= sizeof(shortBuffer) / sizeof(wchar_t))
	{
		longBuffer = new wchar_t[length + 1];
		GetWindowText(draw->hwndItem, longBuffer, length + 1);
		buffer = longBuffer;
	}
	else
	{
		GetWindowText(draw->hwndItem, shortBuffer, sizeof(shortBuffer) / sizeof(wchar_t));
		buffer = shortBuffer;
	}
	FillRect(draw->hDC, &draw->rcItem, theme::brush(theme::BACKGROUND));
	auto style = GetWindowLong(draw->hwndItem, GWL_STYLE);
	SetBkColor(draw->hDC, theme::color(theme::BACKGROUND));
	SetTextColor(draw->hDC, theme::color(theme::TEXT));
	auto rect = draw->rcItem;
	if (draw->itemState & ODS_FOCUS)
	{
		rect.left += 1;
		rect.top += 1;
	}
	DrawText(draw->hDC, buffer, -1, &rect, DT_SINGLELINE | DT_CENTER);
	delete[] longBuffer;
}

LRESULT CALLBACK Table::tableProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto table = reinterpret_cast<Table*>(GetWindowLong(hWnd, GWL_USERDATA));
	switch (message)
	{
	case WM_ERASEBKGND:
		return FALSE;
	case WM_PAINT:
	{
		table->onPaint();
		return 0;
	}
	case WM_SETCURSOR:
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		return TRUE;
	case WM_CTLCOLORSTATIC:
		SetTextColor(reinterpret_cast<HDC>(wParam), theme::color(theme::TEXT));
		SetBkColor(reinterpret_cast<HDC>(wParam), theme::color(theme::BACKGROUND));
		return (LRESULT)GetStockObject(NULL_BRUSH);
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_LAUNCH:
		{
			table->onLaunch(HWND(lParam));
			break;
		}
		case ID_COMBO:
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE:
			{
				table->onComboChange();
				break;
			}
			}
			break;
		}
		break;
	}
	case WM_DRAWITEM:
	{
		auto draw = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
		table->onDrawButton(draw);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
