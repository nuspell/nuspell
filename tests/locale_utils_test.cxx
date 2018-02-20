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

#include <iostream>

#include "../src/nuspell/locale_utils.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("header locale_utils", "[locale_utils]")
{
	SECTION("method decode_utf8")
	{
		CHECK(U"" == decode_utf8(""s));
		// Omit constructor "..."s without risk for UTF-8?
		CHECK(U"azĳß«" == decode_utf8("azĳß«"s));
		CHECK(U"日  Ӥ" != decode_utf8("Ӥ日本に"s));
		// need counter example too
	}

	SECTION("method validate_utf8")
	{
		CHECK(validate_utf8(""s));
		CHECK(validate_utf8("the brown fox~"s));
		CHECK(validate_utf8("Ӥ日本に"s));
		// need counter example too
	}

	SECTION("method is_ascii")
	{
		CHECK(is_ascii('a'));
		CHECK(is_ascii('\t'));
		// Results in warning "multi-character character constant
		// [-Wmultichar]"
		// FIXME Add this to Makefile.am for only this source file.
		//        CHECK_FALSE(is_ascii('Ӥ'));
	}

	SECTION("method is_all_ascii")
	{
		CHECK(is_all_ascii(""s));
		CHECK(is_all_ascii("the brown fox~"s));
		CHECK_FALSE(is_all_ascii("brown foxĳӤ"s));
	}

	SECTION("method latin1_to_ucs2")
	{
		CHECK(u"" == latin1_to_ucs2(""s));
		CHECK(u"abc" == latin1_to_ucs2("abc"s));
		// QUESTION Is next line OK?
		CHECK(u"²¿ýþÿ" != latin1_to_ucs2("²¿ýþÿ"s));
		CHECK(u"Ӥ日本に" != latin1_to_ucs2("Ӥ日本に"s));
	}

	SECTION("method latin1_to_u32")
	{
		CHECK(U"" == latin1_to_u32(""s));
		CHECK(U"abc~" == latin1_to_u32("abc~"s));
		// QUESTION Is next line OK?
		CHECK(U"²¿ýþÿ" != latin1_to_u32("²¿ýþÿ"s));
		CHECK(U"Ӥ日本に" != latin1_to_u32("Ӥ日本に"s));
	}

	SECTION("method is_all_bmp") { CHECK(true == is_all_bmp(U"abcýþÿӤ")); }

	SECTION("method u32_to_ucs2_skip_non_bmp")
	{
		CHECK(u"ABC" == u32_to_ucs2_skip_non_bmp(U"ABC"));
	}
}
