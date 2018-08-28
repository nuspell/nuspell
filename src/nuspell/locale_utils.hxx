/* Copyright 2016-2018 Dimitrij Mijoski
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
 * @file locale_utils.hxx
 * Encoding transformations.
 */

#ifndef LOCALE_UTILS_HXX
#define LOCALE_UTILS_HXX

#include <locale>
#include <string>

namespace nuspell {

auto utf8_to_32_alternative(const std::string& s) -> std::u32string;
auto validate_utf8(const std::string& s) -> bool;

auto wide_to_utf8(const std::wstring& in, std::string& out) -> void;
auto wide_to_utf8(const std::wstring& in) -> std::string;

auto utf8_to_wide(const std::string& in, std::wstring& out) -> bool;
auto utf8_to_wide(const std::string& in) -> std::wstring;

auto utf8_to_32(const std::string& in) -> std::u32string;

auto is_ascii(char c) -> bool;
auto is_all_ascii(const std::string& s) -> bool;

auto latin1_to_ucs2(const std::string& s) -> std::u16string;
auto latin1_to_ucs2(const std::string& s, std::u16string& out) -> void;

auto is_bmp(char32_t c) -> bool;
auto is_all_bmp(const std::u32string& s) -> bool;
auto u32_to_ucs2_skip_non_bmp(const std::u32string& s) -> std::u16string;
auto u32_to_ucs2_skip_non_bmp(const std::u32string& s, std::u16string& out)
    -> void;

auto to_wide(const std::string& in, const std::locale& inloc, std::wstring& out)
    -> bool;
auto to_wide(const std::string& in, const std::locale& inloc) -> std::wstring;
auto to_narrow(const std::wstring& in, std::string& out,
               const std::locale& outloc) -> bool;
auto to_narrow(const std::wstring& in, const std::locale& outloc)
    -> std::string;

auto install_ctype_facets_inplace(std::locale& boost_loc) -> void;

enum class Encoding_Details {
	EXTERNAL_U8_INTERNAL_U8,
	EXTERNAL_OTHER_INTERNAL_U8,
	EXTERNAL_U8_INTERNAL_OTHER,
	EXTERNAL_OTHER_INTERNAL_OTHER,
	EXTERNAL_SAME_INTERNAL_AND_SINGLEBYTE
};

auto analyze_encodings(const std::locale& external, const std::locale& internal)
    -> Encoding_Details;

/**
 * Casing type enum, ignoring neutral case characters.
 */
enum class Casing {
	SMALL /**< all lower case or neutral case, e.g. "lowercase" or "123" */,
	INIT_CAPITAL /**< start upper case, rest lower case, e.g. "Initcap" */,
	ALL_CAPITAL /**< all upper case, e.g. "UPPERCASE" or "ALL4ONE" */,
	CAMEL /**< camel case, start lower case, e.g. "camelCase" */,
	PASCAL /**< pascal case, start upper case, e.g. "PascalCase" */
};

template <class CharT>
auto classify_casing(const std::basic_string<CharT>& s,
                     const std::locale& loc = std::locale()) -> Casing;

template <class CharT>
auto has_uppercase_at_compound_word_boundary(
    const std::basic_string<CharT>& word, size_t i, const std::locale& loc)
    -> bool;

class Encoding {
	std::string name;

	auto normalize_name() -> void;

      public:
	enum Enc_Type { SINGLEBYTE = false, UTF8 = true };

	Encoding() = default;
	Encoding(const std::string& e) : name(e) { normalize_name(); }
	Encoding(std::string&& e) : name(move(e)) { normalize_name(); }
	Encoding(const char* e) : name(e) { normalize_name(); }
	auto& operator=(const std::string& e)
	{
		name = e;
		normalize_name();
		return *this;
	}
	auto& operator=(std::string&& e)
	{
		name = move(e);
		normalize_name();
		return *this;
	}
	auto& operator=(const char* e)
	{
		name = e;
		normalize_name();
		return *this;
	}
	auto empty() const { return name.empty(); }
	operator const std::string&() const { return name; }
	auto& value() const { return name; }
	auto is_utf8() const { return name == "UTF-8"; }
	auto value_or_default() -> std::string
	{
		if (name.empty())
			return "ISO8859-1";
		else
			return name;
	}
	operator Enc_Type() const { return is_utf8() ? UTF8 : SINGLEBYTE; }
};
} // namespace nuspell
#endif // LOCALE_UTILS_HXX
