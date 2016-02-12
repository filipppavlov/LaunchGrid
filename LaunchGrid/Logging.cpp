#include "stdafx.h"
#include "Logging.h"
#include "Settings.h"
#include <varargs.h>

void log(const wchar_t* format, ...)
{
	auto dir = settings::settingsDirectory();
	CreateDirectory(dir.c_str(), nullptr);
	auto file = CreateFile((dir + L"\\log.txt").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return;
	}
	SetFilePointer(file, 0, nullptr, FILE_END);

	va_list args1, args2;
	va_start(args1, format);
	va_copy(args2, args1);
	int length = _vscwprintf(format, args1);
	if (length > 0)
	{
		size_t size = size_t(length + 3);
		wchar_t* buffer = new wchar_t[size];
		buffer[0] = 0;
		vswprintf_s(buffer, size, format, args2);
		wcscat_s(buffer, size, L"\r\n");
		DWORD written;
		WriteFile(file, buffer, (length + 2) * sizeof(wchar_t), &written, nullptr);
		delete[] buffer;
	}
	va_end(args1);
	va_end(args2);
	CloseHandle(file);
}