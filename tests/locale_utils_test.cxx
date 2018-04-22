/* Copyright 2017 Sander van Geloven
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

#include "catch.hpp"

#include <algorithm>
#include <iostream>

#include "../src/nuspell/locale_utils.hxx"

#include <boost/locale.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method decode_utf8", "[locale_utils]")
{
	CHECK(U""s == decode_utf8(""s));

	CHECK(
	    U"abczĳß«абвњ\U0001FFFFерњеӤ\u0801\u0912日本にреѐ"s ==
	    decode_utf8(u8"abczĳß«абвњ\U0001FFFFерњеӤ\u0801\u0912日本にреѐ"s));
	CHECK(U"日  Ӥ" != decode_utf8("Ӥ日本に"s));
	// need counter example too
}

TEST_CASE("method validate_utf8", "[locale_utils]")
{
	CHECK(validate_utf8(""s));
	CHECK(validate_utf8("the brown fox~"s));
	CHECK(validate_utf8("Ӥ日本に"s));
	// need counter example too
}

TEST_CASE("method is_ascii", "[locale_utils]")
{
	CHECK(is_ascii('a'));
	CHECK(is_ascii('\t'));

	CHECK_FALSE(is_ascii(char_traits<char>::to_char_type(128)));
}

TEST_CASE("method is_all_ascii", "[locale_utils]")
{
	CHECK(is_all_ascii(""s));
	CHECK(is_all_ascii("the brown fox~"s));
	CHECK_FALSE(is_all_ascii("brown foxĳӤ"s));
}

TEST_CASE("method latin1_to_ucs2", "[locale_utils]")
{
	CHECK(u"" == latin1_to_ucs2(""s));
	CHECK(u"abc\u0080" == latin1_to_ucs2("abc\x80"s));
	CHECK(u"²¿ýþÿ" != latin1_to_ucs2(u8"²¿ýþÿ"s));
	CHECK(u"Ӥ日本に" != latin1_to_ucs2(u8"Ӥ日本に"s));
}

TEST_CASE("method is_all_bmp", "[locale_utils]")
{
	CHECK(true == is_all_bmp(U"abcýþÿӤ"));
}

TEST_CASE("method u32_to_ucs2_skip_non_bmp", "[locale_utils]")
{
	CHECK(u" ABC" == u32_to_ucs2_skip_non_bmp(U"\U0010FFFF AB\U00010000C"));
}

TEST_CASE("to_wide", "[locale_utils]")
{
	auto in = u8"\U0010FFFF ß"s;
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	CHECK(L"\U0010FFFF ß" == to_wide(in, loc));

	in = "abcd\xDF";
	loc = g("en_US.ISO-8859-1");
	CHECK(L"abcdß" == to_wide(in, loc));
}

TEST_CASE("to_narrow", "[locale_utils]")
{
	auto in = L"\U0010FFFF ß"s;
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	CHECK(u8"\U0010FFFF ß" == to_narrow(in, loc));

	in = L"abcdß";
	loc = g("en_US.ISO-8859-1");
	CHECK("abcd\xDF" == to_narrow(in, loc));
}

TEST_CASE("boost locale has icu", "[locale_utils]")
{
	using lbm = boost::locale::localization_backend_manager;
	auto v = lbm::global().get_all_backends();
	CHECK(std::find(v.begin(), v.end(), "icu") != v.end());
}

TEST_CASE("icu ctype facets", "[locale_utils]")
{
	boost::locale::generator g;
	auto loc = g("en_US.UTF-8");
	install_ctype_facets_inplace(loc);

	// test narrow ctype
	CHECK(isupper('A', loc));
	CHECK_FALSE(isupper('a', loc));
	CHECK(islower('a', loc));
	CHECK(ispunct('.', loc));
	CHECK_FALSE(ispunct('a', loc));

	CHECK(tolower('I', loc) == 'i');
	CHECK(toupper('i', loc) == 'I');

	// test above ascii, shoud be false
	CHECK_FALSE(isupper('\xC0', loc));
	CHECK_FALSE(islower('\xC0', loc));
	CHECK_FALSE(isalnum('\xC0', loc));
	CHECK(tolower('\xC0', loc) == '\xC0');
	CHECK(toupper('\xC0', loc) == '\xC0');

	// test wide
	CHECK(isupper(L'A', loc));
	CHECK_FALSE(isupper(L'a', loc));
	CHECK(islower(L'a', loc));
	CHECK(ispunct(L'.', loc));
	CHECK_FALSE(ispunct(L'a', loc));

	CHECK(isupper(L'Ш', loc));
	CHECK_FALSE(isupper(L'ш', loc));
	CHECK(islower(L'ш', loc));
	CHECK(ispunct(L'¿', loc));
	CHECK_FALSE(ispunct(L'ш', loc));

	CHECK(tolower(L'I', loc) == L'i');
	CHECK(toupper(L'i', loc) == L'I');

	CHECK(toupper(L'г', loc) == L'Г');
	CHECK(tolower(L'Г', loc) == L'г');

	CHECK(toupper(L'У', loc) == L'У');
	CHECK(tolower(L'м', loc) == L'м');

	loc = g("ru_RU.ISO8859-5");
	install_ctype_facets_inplace(loc);

	CHECK(isupper('\xC8', loc));
	CHECK_FALSE(isupper('\xE8', loc));
	CHECK(islower('\xE8', loc));
	CHECK(ispunct(',', loc));
	CHECK_FALSE(ispunct('\xE8', loc));

	CHECK(tolower('\xC8', loc) == '\xE8'); // Ш to ш
	CHECK(tolower('\xE8', loc) == '\xE8'); // ш to ш

	CHECK(toupper('\xE8', loc) == '\xC8'); // ш to Ш
	CHECK(toupper('\xC8', loc) == '\xC8'); // Ш to Ш
}
