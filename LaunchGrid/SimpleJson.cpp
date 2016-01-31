#include "stdafx.h"
#include "SimpleJson.h"
#include <locale>
#include <codecvt>

namespace
{
	enum TokenType
	{
		TOC_NUMBER,
		TOC_STRING,
		TOC_COMA,
		TOC_LSBRACKET,
		TOC_RSBRACKET,
		TOC_LCBRACKET,
		TOC_RCBRACKET,
		TOC_COLUMN,
		TOC_END,
		TOC_ERROR,
	};
	struct Token
	{
		Token(TokenType t)
			:type(t)
		{
		}

		explicit Token(double d)
			:type(TOC_NUMBER),
			number(d)
		{
		}

		explicit Token(const std::wstring& s)
			:type(TOC_STRING),
			string(s)
		{
		}

		TokenType type;
		double number;
		std::wstring string;
	};

	class Tokenizer
	{
	public:
		Tokenizer(const wchar_t* stream)
			:m_stream(stream),
			m_hasPutBack(false),
			m_putBack(TOC_END)
		{
		}

		Token operator()()
		{
			if (m_hasPutBack)
			{
				m_hasPutBack = false;
				return m_putBack;
			}
			if (!skipws())
			{
				return TOC_ERROR;
			}
			switch (*m_stream)
			{
			case L',':
				++m_stream;
				return TOC_COMA;
			case L':':
				++m_stream;
				return TOC_COLUMN;
			case L'[':
				++m_stream;
				return TOC_LSBRACKET;
			case L']':
				++m_stream;
				return TOC_RSBRACKET;
			case L'{':
				++m_stream;
				return TOC_LCBRACKET;
			case L'}':
				++m_stream;
				return TOC_RCBRACKET;
			case 0:
				++m_stream;
				return TOC_END;
			case L'"':
				return parseString();
			case L'.':
			case L'-':
			case L'0':
			case L'1':
			case L'2':
			case L'3':
			case L'4':
			case L'5':
			case L'6':
			case L'7':
			case L'8':
			case L'9':
				return parseNumber();
			default:
				return TOC_ERROR;
			}
		}

		void putBack(const Token& token)
		{
			m_hasPutBack = true;
			m_putBack = token;
		}

		bool expect(TokenType type)
		{
			auto t = operator()();
			if (t.type == type)
			{
				return true;
			}
			putBack(t);
			return false;
		}
	private:
		bool skipws()
		{
			while (true)
			{
				while (*m_stream && isspace(*m_stream))
				{
					++m_stream;
				}
				if (m_stream[0] == L'/' && m_stream[1] == L'/')
				{
					while (*m_stream && m_stream[0] != L'\n')
					{
						++m_stream;
					}
				}
				else if (m_stream[0] == L'/' && m_stream[1] == L'*')
				{
					while (*m_stream && m_stream[0] != L'*' && m_stream[0] != L'/')
					{
						++m_stream;
					}
					if (m_stream[0] == 0)
					{
						return false;
					}
					else
					{
						m_stream += 2;
					}
				}
				else
				{
					break;
				}
			}
			return true;
		}

		Token parseNumber()
		{
			wchar_t* end;
			Token t(std::wcstod(m_stream, &end));
			if (end == m_stream)
			{
				return TOC_ERROR;
			}
			m_stream = end;
			return t;
		}

		Token parseString()
		{
			auto t = Token(std::wstring());
			++m_stream;
			while (*m_stream)
			{
				switch (*m_stream)
				{
				case L'"':
					++m_stream;
					return t;
				case L'\\':
					++m_stream;
					switch (*m_stream)
					{
					case L'b':
						t.string += L'\b';
						break;
					case L'\f':
						t.string += L'\f';
						break;
					case L'\n':
						t.string += L'\n';
						break;
					case L'\r':
						t.string += L'\r';
						break;
					case L'\t':
						t.string += L'\t';
						break;
					default:
						t.string += *m_stream;
					}
					++m_stream;
					break;
				default:
					t.string += *m_stream++;
				}
			}
			return t;
		}
		const wchar_t* m_stream;
		bool m_hasPutBack;
		Token m_putBack;
	};

	simplejson::Value parseObject(Tokenizer& tokenizer);
	simplejson::Value parseArray(Tokenizer& tokenizer);

	simplejson::Value parse(Tokenizer& tokenizer)
	{
		auto t = tokenizer();
		switch (t.type)
		{
		case TOC_NUMBER:
			return simplejson::Value::number(t.number);
		case TOC_STRING:
			return simplejson::Value::string(t.string);
		case TOC_LCBRACKET:
			return parseObject(tokenizer);
		case TOC_LSBRACKET:
			return parseArray(tokenizer);
		default:
			tokenizer.putBack(t);
			return simplejson::Value::undefined();
		}
	}

	simplejson::Value parseObject(Tokenizer& tokenizer)
	{
		auto result(simplejson::Value::object());
		while (true)
		{
			auto key = parse(tokenizer);

			if (key.type() == simplejson::VT_UNDEFINED)
			{
				return tokenizer.expect(TOC_RCBRACKET) ? result : key;
			}
			if (key.type() != simplejson::VT_STRING)
			{
				return simplejson::Value::undefined();
			}
			if (!tokenizer.expect(TOC_COLUMN))
			{
				return simplejson::Value::undefined();
			}
			auto value = parse(tokenizer);
			if (value.type() == simplejson::VT_UNDEFINED)
			{
				return simplejson::Value::undefined();
			}
			result[key.asString()] = std::move(value);
			auto t = tokenizer();
			switch (t.type)
			{
			case TOC_RCBRACKET:
				return result;
			case TOC_COMA:
				break;
			default:
				tokenizer.putBack(t);
				return simplejson::Value::undefined();
			}
		}
	}

	simplejson::Value parseArray(Tokenizer& tokenizer)
	{
		auto result(simplejson::Value::array());
		while (true)
		{
			auto value = parse(tokenizer);
			if (value.type() == simplejson::VT_UNDEFINED)
			{
				return tokenizer.expect(TOC_RSBRACKET) ? result : simplejson::Value::undefined();
			}
			result.push(value);
			auto t = tokenizer();
			switch (t.type)
			{
			case TOC_RSBRACKET:
				return result;
			case TOC_COMA:
				break;
			default:
				tokenizer.putBack(t);
				return simplejson::Value::undefined();
			}
		}
	}

	std::wstring escape(const std::wstring &str)
	{
		std::wstring result;
		for (auto it = str.begin(); it != str.end(); ++it)
		{
			switch (*it)
			{
			case L'\b':
				result += L"\\b";
				break;
			case L'\f':
				result += L"\\f";
				break;
			case L'\n':
				result += L"\\n";
				break;
			case L'\r':
				result += L"\\r";
				break;
			case L'\t':
				result += L"\\t";
				break;
			case L'\\':
				result += L"\\\\";
				break;
			case L'\"':
				result += L"\\\"";
				break;
			default:
				result += *it;
			}
		}
		return result;
	}
}

namespace simplejson
{
	Value::Value()
	{
	}

	Value::Value(ValueType t)
	{
		m_impl.reset(new ValueImpl);
		m_impl->m_type = t;
	}

	Value& Value::undefined()
	{
		static Value u;
		return u;
	}

	Value Value::number(double number)
	{
		Value result(VT_NUMBER);
		result.m_impl->m_number = number;
		return result;
	}

	Value Value::string(const wchar_t* str)
	{
		Value result(VT_STRING);
		result.m_impl->m_string = str;
		return result;
	}

	Value Value::string(const std::wstring& str)
	{
		Value result(VT_STRING);
		result.m_impl->m_string = str;
		return result;
	}

	Value Value::object()
	{
		return Value(VT_OBJECT);
	}

	Value Value::array()
	{
		return Value(VT_ARRAY);
	}

	ValueType Value::type() const
	{
		return m_impl ? m_impl->m_type : VT_UNDEFINED;
	}

	Value::operator bool() const
	{
		switch (type())
		{
		case VT_NUMBER:
			return m_impl->m_number != 0.0;
		case VT_STRING:
			return !m_impl->m_string.empty();
		case VT_UNDEFINED:
			return false;
		default:
			return true;
		}
	}

	double Value::asNumber(double defaultValue) const
	{
		return type() == VT_NUMBER ? m_impl->m_number : defaultValue;
	}

	std::wstring Value::asString(const wchar_t *defaultValue) const
	{
		return type() == VT_STRING ? m_impl->m_string : defaultValue;
	}

	size_t Value::length() const
	{
		return type() == VT_ARRAY ? m_impl->m_array.size() : 0;
	}

	ValueRef Value::operator[](size_t index) const
	{
		if (type() == VT_ARRAY)
		{
			return ValueRef(m_impl, index);// index < m_impl->m_array.size() ? m_impl->m_array[index] : undefined();
		}
		return ValueRef();// undefined();
	}

	void Value::push(const Value& value)
	{
		if (type() == VT_ARRAY)
		{
			m_impl->m_array.push_back(value);
		}
	}

	void Value::remove(size_t index)
	{
		if (type() == VT_ARRAY)
		{
			if (m_impl->m_array.size() >= index)
			{
				m_impl->m_array.erase(m_impl->m_array.begin() + index);
			}
		}
	}

	ValueRef Value::operator[](const wchar_t* key) const
	{
		if (type() != VT_OBJECT)
		{
			return ValueRef();
		}
		return ValueRef(m_impl, key);
	}

	ValueRef Value::operator[](const std::wstring& key) const
	{
		return operator[](key.c_str());
	}

	std::vector<std::wstring> Value::objectKeys() const
	{
		std::vector<std::wstring> result;
		if (type() != VT_OBJECT)
		{
			return result;
		}
		result.reserve(m_impl->m_object.size());
		for (auto it = m_impl->m_object.begin(); it != m_impl->m_object.end(); ++it)
		{
			result.push_back(it->first);
		}
		return result;
	}

	Value Value::setDefault(const wchar_t* key, const Value& value)
	{
		if (type() != VT_OBJECT)
		{
			return undefined();
		}
		auto found = m_impl->m_object.find(key);
		if (found != m_impl->m_object.end())
		{
			return found->second;
		}
		return m_impl->m_object[key] = value;
	}

	bool Value::has(const wchar_t* key) const
	{
		return type() == VT_OBJECT && m_impl->m_object.find(key) != m_impl->m_object.end();
	}

	Value::iterator Value::begin()
	{
		iterator it;
		switch (type())
		{
		case VT_ARRAY:
			it.m_parent = *this;
			it.m_arrayIndex = 0;
			break;
		case VT_OBJECT:
			it.m_parent = *this;
			it.m_objectIterator = m_impl->m_object.begin();
			break;
		default:
			break;
		}
		return it;
	}
	Value::iterator Value::end()
	{
		iterator it;
		switch (type())
		{
		case VT_ARRAY:
			it.m_parent = *this;
			it.m_arrayIndex = m_impl->m_array.size();
			break;
		case VT_OBJECT:
			it.m_parent = *this;
			it.m_objectIterator = m_impl->m_object.end();
			break;
		default:
			break;
		}
		return it;
	}



	ValueRef::ValueRef()
	{
	}

	ValueRef::ValueRef(const std::shared_ptr<ValueImpl>& parent, size_t index)
		:m_parent(parent),
		m_arrayIndex(index)
	{
		m_impl = index < m_parent->m_array.size() ? m_parent->m_array[index].m_impl : undefined().m_impl;
	}

	ValueRef::ValueRef(const std::shared_ptr<ValueImpl>& parent, const wchar_t* index)
		:m_parent(parent),
		m_objectIndex(index)
	{
		auto found = m_parent->m_object.find(m_objectIndex);
		if (found != m_parent->m_object.end())
		{
			m_impl = found->second.m_impl;
		}
	}

	ValueRef& ValueRef::operator=(const Value& other)
	{
		if (!m_parent)
		{
			return *this;
		}
		if (m_parent->m_type == VT_ARRAY)
		{
			if (m_parent->m_array.size() <= m_arrayIndex)
			{
				m_parent->m_array.resize(m_arrayIndex + 1);
			}
			m_parent->m_array[m_arrayIndex] = other;
		}
		else
		{
			m_parent->m_object[m_objectIndex] = other;
		}
		return *this;
	}

	ValueRef& ValueRef::operator=(const ValueRef& other)
	{
		return operator=((const Value&)other);
	}




	Value parse(const char* stream)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
		std::wstring src = convert.from_bytes(stream);

		auto tokenizer = Tokenizer(src.c_str());
		auto result = parse(tokenizer);
		if (tokenizer().type != TOC_END)
		{
			return simplejson::Value::undefined();
		}
		return result;
	}

	std::wstring wdump(const Value& value)
	{
		std::u16string s;
		switch (value.type())
		{
		case VT_NUMBER:
			return std::to_wstring(value.asNumber());
		case VT_STRING:
			return L"\"" + escape(value.asString()) + L"\"";
		case VT_OBJECT:
		{
			std::wstring result = L"{";
			auto keys = value.objectKeys();
			bool first = true;
			for (auto it = keys.begin(); it != keys.end(); ++it)
			{
				if (first)
				{
					first = false;
				}
				else
				{
					result += L", ";
				}
				result += L"\"" + escape(*it) + L"\": " + wdump(value[*it]);
			}
			result += L"}";
			return result;
		}
		case VT_ARRAY:
		{
			std::wstring result = L"[";
			auto length = value.length();
			for (size_t i = 0; i < length; ++i)
			{
				if (i)
				{
					result += L", ";
				}
				result += wdump(value[i]);
			}
			result += L"]";
			return result;
		}
		}
		return L"";
	}

	std::string dump(const Value& value)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
		return convert.to_bytes(wdump(value));
	}
}
