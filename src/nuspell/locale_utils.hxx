/* Copyright 2016-2019 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Encoding transformations, private header.
 */

#ifndef NUSPELL_LOCALE_UTILS_HXX
#define NUSPELL_LOCALE_UTILS_HXX

#include <locale>
#include <string>

#include <clocale>

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) ||               \
                         (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#endif

#include <boost/container/small_vector.hpp>
#include <unicode/locid.h>

struct UConverter; // unicode/ucnv.h

namespace nuspell {

auto validate_utf8(const std::string& s) -> bool;

auto wide_to_utf8(const std::wstring& in, std::string& out) -> void;
auto wide_to_utf8(const std::wstring& in) -> std::string;
auto wide_to_utf8(const std::wstring& in,
                  boost::container::small_vector_base<char>& out) -> void;

auto utf8_to_wide(const std::string& in, std::wstring& out) -> bool;
auto utf8_to_wide(const std::string& in) -> std::wstring;

auto utf8_to_16(const std::string& in) -> std::u16string;
auto utf8_to_16(const std::string& in, std::u16string& out) -> bool;

auto is_ascii(char c) -> bool;
auto is_all_ascii(const std::string& s) -> bool;

auto latin1_to_ucs2(const std::string& s) -> std::u16string;
auto latin1_to_ucs2(const std::string& s, std::u16string& out) -> void;

auto is_all_bmp(const std::u16string& s) -> bool;

auto to_wide(const std::string& in, const std::locale& inloc, std::wstring& out)
    -> bool;
auto to_wide(const std::string& in, const std::locale& inloc) -> std::wstring;
auto to_narrow(const std::wstring& in, std::string& out,
               const std::locale& outloc) -> bool;
auto to_narrow(const std::wstring& in, const std::locale& outloc)
    -> std::string;

auto is_locale_known_utf8(const std::locale& loc) -> bool;

auto wide_to_icu(const std::wstring& in, icu::UnicodeString& out) -> bool;
auto icu_to_wide(const icu::UnicodeString& in, std::wstring& out) -> bool;

auto to_upper(const std::wstring& in, const icu::Locale& loc) -> std::wstring;
auto to_title(const std::wstring& in, const icu::Locale& loc) -> std::wstring;
auto to_lower(const std::wstring& in, const icu::Locale& loc) -> std::wstring;

/**
 * @brief Casing type enum, ignoring neutral case characters.
 */
enum class Casing {
	SMALL /**< all lower case or neutral case, e.g. "lowercase" or "123" */,
	INIT_CAPITAL /**< start upper case, rest lower case, e.g. "Initcap" */,
	ALL_CAPITAL /**< all upper case, e.g. "UPPERCASE" or "ALL4ONE" */,
	CAMEL /**< camel case, start lower case, e.g. "camelCase" */,
	PASCAL /**< pascal case, start upper case, e.g. "PascalCase" */
};

auto classify_casing(const std::wstring& s) -> Casing;

auto has_uppercase_at_compound_word_boundary(const std::wstring& word, size_t i)
    -> bool;

class Encoding_Converter {
	UConverter* cnv = nullptr;

      public:
	Encoding_Converter() = default;
	explicit Encoding_Converter(const char* enc);
	explicit Encoding_Converter(const std::string& enc)
	    : Encoding_Converter(enc.c_str())
	{
	}
	~Encoding_Converter();
	Encoding_Converter(const Encoding_Converter& other);
	Encoding_Converter(Encoding_Converter&& other)
	{
		cnv = other.cnv;
		cnv = nullptr;
	};
	auto operator=(const Encoding_Converter& other) -> Encoding_Converter&;
	auto operator=(Encoding_Converter&& other) -> Encoding_Converter&
	{
		std::swap(cnv, other.cnv);
		return *this;
	}
	auto to_wide(const std::string& in, std::wstring& out) -> bool;
	auto to_wide(const std::string& in) -> std::wstring;
	auto valid() -> bool { return cnv != nullptr; }
};

//#if _POSIX_VERSION >= 200809L
#ifdef _POSIX_VERSION
class Setlocale_To_C_In_Scope {
	locale_t old_loc = nullptr;

      public:
	Setlocale_To_C_In_Scope()
	    : old_loc{uselocale(newlocale(0, "C", nullptr))}
	{
	}
	~Setlocale_To_C_In_Scope()
	{
		auto new_loc = uselocale(old_loc);
		if (new_loc != old_loc)
			freelocale(new_loc);
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#else
class Setlocale_To_C_In_Scope {
	std::string old_name;
#ifdef _WIN32
	int old_per_thread;
#endif
      public:
	Setlocale_To_C_In_Scope() : old_name(setlocale(LC_ALL, nullptr))
	{
#ifdef _WIN32
		old_per_thread = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif
		auto x = setlocale(LC_ALL, "C");
		if (!x)
			old_name.clear();
	}
	~Setlocale_To_C_In_Scope()
	{
#ifdef _WIN32
		_configthreadlocale(old_per_thread);
		if (old_per_thread == _ENABLE_PER_THREAD_LOCALE)
#endif
		{
			if (!old_name.empty())
				setlocale(LC_ALL, old_name.c_str());
		}
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#endif

} // namespace nuspell
#endif // NUSPELL_LOCALE_UTILS_HXX
