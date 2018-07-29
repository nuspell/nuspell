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
 *
 * The library differentiates three encodings:
 *
 * 1. Entry point/input encoding. Can be anything.
 * 2. Intermediate - fixed length, either singlebyte or wide (UTF-32).
 * 3. Dictionary encoding, either singlebyte or narrow multibyte utf-8.
 *
 * If the dictionary is utf-8, then the wide instantiations of the template
 * agorithms will be used. If the dictionary is singlebyte then everything is
 * char.
 *
 * The functions Locale_Input::cvt_for_byte_dict() and
 * Locale_Input::cvt_for_u8_dict() convert from input
 * into intermediate.
 *
 * If the dictionary is UTF-8, we should still store large data in it because
 * storing the wordlist in UTF-32 will take more memory.
 *
 * For conversion between intermediate and dictionary encoding we have
 * the functions to_dict_encoding() and from_dict_to_wide_encoding().
 */

#ifndef LOCALE_UTILS_HXX
#define LOCALE_UTILS_HXX

#include <locale>
#include <string>
#include <type_traits>

#include <boost/locale/encoding_utf.hpp>

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

class Locale_Input {
	std::locale external_locale;

	auto static cvt_for_byte_dict(const std::string& in,
	                              const std::locale& inloc,
	                              std::string& out,
	                              const std::locale& dicloc,
	                              std::wstring& wide_buffer) -> bool;

	auto static cvt_for_u8_dict(const std::string& in,
	                            const std::locale& inloc, std::wstring& out)
	    -> bool;

      public:
	auto imbue(const std::locale& loc) { external_locale = loc; }
	// getloc() calls in iostreams return by value doing a ref-counted copy
	// of pointer. Doing that we pay little extra than just returning a
	// const reference. Here we fix that and return a const reference.
	auto& getloc() const { return external_locale; }

      protected:
	auto cvt_for_byte_dict(const std::string& in, std::string& out,
	                       const std::locale& dicloc,
	                       std::wstring& wide_buffer) const
	{
		return cvt_for_byte_dict(in, getloc(), out, dicloc,
		                         wide_buffer);
	}

	auto cvt_for_u8_dict(const std::string& in, std::wstring& out) const
	{
		return cvt_for_u8_dict(in, getloc(), out);
	}
};

template <class CharT>
struct Unicode_Input {
	auto static cvt_for_byte_dict(const std::basic_string<CharT>& in,
	                              const std::locale& dicloc)
	{
		using namespace boost::locale::conv;
		return to_narrow(utf_to_utf<wchar_t>(in), dicloc);
	}

	auto static cvt_for_u8_dict(const std::basic_string<CharT>& in)
	{
		using namespace boost::locale::conv;
		return utf_to_utf<wchar_t>(in);
	}
};

/**
 * Casing type enum, ignoring neutral case characters.
 */
enum class Casing {
	SMALL /**< all lower case or neutral case, e.g. "lowercase" or "123" */,
	INIT_CAPITAL /**< start upper case, rest lower case, e.g. "InitCap" */,
	ALL_CAPITAL /**< all upper case, e.g. "UPPERCASE" or "ALL4ONE" */,
	CAMEL /**< camel case, start lower case, e.g. "camelCase" */,
	PASCAL /**< pascal case, start upper case, e.g. "PascalCase" */
};

/**
 * Determines casing (capitalization) type for a word.
 *
 * Casing is sometimes referred to as capitalization.
 *
 * @param s word for which casing is determined.
 * @return The casing type.
 */
template <class CharT>
auto classify_casing(const std::basic_string<CharT>& s,
                     const std::locale& loc = std::locale()) -> Casing;

} // namespace nuspell
#endif // LOCALE_UTILS_HXX
