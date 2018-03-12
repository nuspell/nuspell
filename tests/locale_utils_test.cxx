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
	// Results in warning "multi-character character constant
	// [-Wmultichar]"
	// FIXME Add this to Makefile.am for only this source file.
	//        CHECK_FALSE(is_ascii('Ӥ'));
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
	CHECK(u"abc" == latin1_to_ucs2("abc"s));
	// QUESTION Is next line OK?
	CHECK(u"²¿ýþÿ" != latin1_to_ucs2("²¿ýþÿ"s));
	CHECK(u"Ӥ日本に" != latin1_to_ucs2("Ӥ日本に"s));
}

TEST_CASE("method is_all_bmp", "[locale_utils]")
{
	CHECK(true == is_all_bmp(U"abcýþÿӤ"));
}

TEST_CASE("method u32_to_ucs2_skip_non_bmp", "[locale_utils]")
{
	CHECK(u" ABC" == u32_to_ucs2_skip_non_bmp(U"\U0010FFFF AB\U00010000C"));
}

TEST_CASE("boost locale has icu", "[locale_utils]")
{
	using lbm = boost::locale::localization_backend_manager;
	auto v = lbm::global().get_all_backends();
	CHECK(std::find(v.begin(), v.end(), "icu") != v.end());
}
