#include "stdafx.h"
#include "Settings.h"
#include "Logging.h"

namespace
{
	std::wstring settingsPath(const wchar_t* fileName)
	{
		return settings::settingsDirectory() + L"\\" + fileName;
	}

	simplejson::Value load(const wchar_t* fileName)
	{
		std::wstring filename = settingsPath(fileName);
		LOG(L"loading settings from %ls", filename.c_str());
		auto file = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			LOG(L"failed to open settings file");
			return simplejson::Value::object();
		}
		DWORD size = GetFileSize(file, nullptr);
		if (size)
		{
			char* buffer = new char[size + 1];
			DWORD read = 0;
			ReadFile(file, buffer, size, &read, nullptr);
			CloseHandle(file);
			if (read)
			{
				buffer[read] = 0;
				LOG(L"parsing settings");
				auto value = simplejson::parse(buffer);
				LOG(L"done parsing settings");
				return value;
			}
		}
		else
		{
			CloseHandle(file);
		}
		return simplejson::Value::object();
	}

	void saveDirectly(const std::string& text, const wchar_t* fileName)
	{
		LOG(L"saving settings to %ls", fileName);
		auto file = CreateFile(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (file == INVALID_HANDLE_VALUE)
		{
			LOG(L"failed to open the file");
			return;
		}
		DWORD size = DWORD(text.length());
		DWORD written = 0;
		WriteFile(file, text.c_str(), size, &written, nullptr);
		CloseHandle(file);
		LOG(L"saved settings");
	}

	void saveSettings(simplejson::Value settings, const wchar_t* fileName)
	{
		LOG(L"saving settings");
		auto str = simplejson::dump(settings);

		std::wstring filename = settingsPath(fileName);
		CreateDirectory(settings::settingsDirectory().c_str(), nullptr);
		wchar_t filePath[MAX_PATH];
		if (GetTempFileName(settings::settingsDirectory().c_str(), L"", 0, filePath))
		{
			auto file = CreateFile(filePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (file == INVALID_HANDLE_VALUE)
			{
				LOG(L"failed to create temp settings file");
				saveDirectly(str, filename.c_str());
				return;
			}
			DWORD written = 0;
			WriteFile(file, str.c_str(), str.length(), &written, nullptr);
			CloseHandle(file);

			if (!MoveFileEx(filePath, filename.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
			{
				LOG(L"failed to move temp settings file - attempting to write settings directly");
				saveDirectly(str, filename.c_str());
			}
		}
		else
		{
			LOG(L"failed to get temp file - attempting to write settings directly");
			saveDirectly(str, filename.c_str());
		}
	}

	const wchar_t* SETTINGS_FILE_NAME = L"settings.json";
	const wchar_t* CACHE_FILE_NAME = L"cache.json";

	simplejson::Value& sSettings()
	{
		static simplejson::Value value = load(SETTINGS_FILE_NAME);
		return value;
	}

	simplejson::Value& sCache()
	{
		static simplejson::Value value = load(CACHE_FILE_NAME);
		return value;
	}
}

namespace settings
{
	std::wstring settingsDirectory()
	{
		std::wstring filename;
		PWSTR appData = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData)) && appData)
		{
			filename = appData;
			filename += L"\\";
			CoTaskMemFree(appData);
		}
		filename += L"LaunchGrid";
		return filename;
	}

	simplejson::Value settings()
	{
		return sSettings();
	}

	void setSettings(simplejson::Value settings)
	{
		sSettings() = settings;
		saveSettings(settings, SETTINGS_FILE_NAME);
	}

	simplejson::Value cache()
	{
		return sCache();
	}

	void setCache(simplejson::Value settings)
	{
		sCache() = settings;
		saveSettings(settings, CACHE_FILE_NAME);
	}

	void saveCache()
	{
		saveSettings(sCache(), CACHE_FILE_NAME);
	}
}