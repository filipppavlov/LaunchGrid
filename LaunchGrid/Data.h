#pragma once

#include "SimpleJson.h"

enum OptionPosition
{
	OP_ROW,
	OP_COLUMN,
	OP_SELECT,
};

class BaseOption
{
public:
	struct Value
	{
		std::wstring name;
		std::wstring value;
	};

	static BaseOption* create(simplejson::Value value);

	virtual ~BaseOption() {}
	
	const wchar_t* displayName() const;
	OptionPosition position() const;

	std::wstring expandValue(size_t index, const std::map<BaseOption*, size_t>& values);
	static std::wstring expandString(const std::wstring& string, const std::map<BaseOption*, size_t>& values);
	static std::wstring expandString(const std::wstring& string, const std::map<std::wstring, std::wstring>& substitutions);

	virtual void load(simplejson::Value value);
	virtual void prepareValues() {};
	virtual size_t valueCount() const = 0;
	virtual Value value(size_t index) const = 0;
	virtual std::wstring defaultValue() const = 0;
private:
	OptionPosition m_position;
	std::wstring m_name;
};

class TextOption : public BaseOption
{
public:
	TextOption();

	void load(simplejson::Value value);
	size_t valueCount() const;
	Value value(size_t index) const;
	std::wstring defaultValue() const;
private:
	std::vector<Value> m_values;
	std::wstring m_defaultValue;
};

class AppOption : public TextOption
{
public:
	void load(simplejson::Value value);
	std::wstring arguments(size_t index, const std::map<BaseOption*, size_t>& values) const;
	bool runHidden(size_t index) const;
	std::wstring verb(size_t index) const;
private:
	struct AppOptions
	{
		std::wstring arguments;
		std::wstring verb;
		bool runHidden;
	};
	std::vector<AppOptions> m_options;
};


class BaseDirectoryOption : public BaseOption
{
	size_t valueCount() const;
	Value value(size_t index) const;
	std::wstring defaultValue() const;

	void load(simplejson::Value value);
	void prepareValues();
	size_t patternCount() const;
	const std::wstring& pattern(size_t index) const;
protected:
	BaseDirectoryOption(bool directory);
private:
	bool m_directory;
	std::vector<Value> m_patterns;
	std::vector<Value> m_values;
};

class DirectoryOption : public BaseDirectoryOption
{
public:
	DirectoryOption() : BaseDirectoryOption(true)
	{
	}
};

class FileOption : public BaseDirectoryOption
{
public:
	FileOption() : BaseDirectoryOption(false)
	{
	}
};


void splitPath(const wchar_t * path, std::vector<std::wstring>& components);

template <typename R>
void findFiles(const std::wstring& name, const std::wstring& variable, const std::wstring& path, bool directory, R& r)
{
	std::vector<std::wstring> components;
	splitPath(path.c_str(), components);
	std::wstring prefix;
	findFiles(prefix, name, variable, components, 0, directory, r);
}

template <typename R>
void findFiles(std::wstring prefix, std::wstring nameTemplate, const std::wstring& variable, const std::vector<std::wstring>& components, size_t start, bool directory, R& r)
{
	for (size_t i = start; i < components.size(); ++i)
	{
		if (components[i].find(L'*') != std::wstring::npos || components[i].find(L'?') != std::wstring::npos)
		{
			WIN32_FIND_DATAW data;
			auto find = FindFirstFileW((prefix + components[i]).c_str(), &data);
			if (find == INVALID_HANDLE_VALUE)
			{
				return;
			}
			do
			{
				if (((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) == directory && wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
				{
					findFiles(prefix + data.cFileName, nameTemplate, variable, components, i + 1, directory, r);
				}
			} while (FindNextFileW(find, &data) != 0);
			FindClose(find);
			return;
		}
		else
		{
			prefix += components[i];
		}
	}

	WIN32_FIND_DATAW data;
	auto find = FindFirstFileW(prefix.c_str(), &data);
	if (find != INVALID_HANDLE_VALUE)
	{
		FindClose(find);

		if (nameTemplate.empty())
		{
			r(prefix, prefix);
		}
		else
		{
			std::vector<std::wstring> nameComponents;
			splitPath(prefix.c_str(), nameComponents);

			auto name = BaseOption::expandString(nameTemplate, std::map<std::wstring, std::wstring>({ { variable, prefix } }));
			r(name, prefix);
		}
	}
}