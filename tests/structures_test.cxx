/* Copyright 2018 Sander van Geloven
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

#include "../src/nuspell/structures.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method substring replace copy", "[structures]")
{
	auto rep = Substring_Replacer({{"asd", "zxc"},
	                               {"as", "rtt"},
	                               {"a", "A"},
	                               {"abbb", "ABBB"},
	                               {"asd  ", ""},
	                               {"asd ZXC", "YES"},
	                               {"sd ZXC as", "NO"},
	                               {"", "123"},
	                               {" TT", ""}});
	CHECK(rep.replace_copy("QWE asd ZXC as TT"s) == "QWE YES rtt");
}

// See also tests/v1cmdline/condition.* and individual language support.

// TODO add a third TEXT_CASE for twofold suffix stripping

TEST_CASE("class Prefix_Entry", "[structures]")
{
	// TODO ignore "0" to make method more failsafe? See aff_data.cxx with
	// elem.stripping == "0"
	// auto pfx_tests = nuspell::Prefix_Entry(u'U', true, "0"s, "un"s,
	// "wr."s);

	auto pfx_tests = Prefix<char>(u'U', true, ""s, "un"s, "wr."s);

	SECTION("method to_root")
	{
		auto word = "unwry"s;
		CHECK("wry"s == pfx_tests.to_root(word));
		CHECK("wry"s == word);
	}

	SECTION("method to_root_copy")
	{
		auto word = "unwry"s;
		CHECK("wry"s == pfx_tests.to_root_copy(word));
		CHECK("unwry"s == word);
	}

	SECTION("method to_derived")
	{
		auto word = "wry"s;
		CHECK("unwry"s == pfx_tests.to_derived(word));
		CHECK("unwry"s == word);
	}

	SECTION("method to_derived_copy")
	{
		auto word = "wry"s;
		CHECK("unwry"s == pfx_tests.to_derived_copy(word));
		CHECK("wry"s == word);
	}

	SECTION("method check_condition")
	{
		CHECK(true == pfx_tests.check_condition("wry"s));
		CHECK(false == pfx_tests.check_condition("unwry"s));
	}
}

TEST_CASE("class Suffix", "[structures]")
{
	auto sfx_tests = Suffix<char>(u'T', true, "y"s, "ies"s, ".[^aeiou]y"s);
	auto sfx_sk_SK =
	    Suffix<char>(u'Z', true, "ata"s, "át"s, "[^áéíóúý].[^iš]ata"s);
	auto sfx_pt_PT = Suffix<char>(u'X', true, "er"s, "a"s, "[^cug^-]er"s);
	// TODO See above regarding "0"
	auto sfx_gd_GB = Suffix<char>(u'K', true, "0"s, "-san"s, "[^-]"s);
	auto sfx_ar = Suffix<char>(u'a', true, "ه"s, "ي"s, "[^ءؤأ]ه"s);
	auto sfx_ko =
	    Suffix<char>(24, true, "ᅬ다"s, " ᅫᆻ어"s,
	                 "[ᄀᄁᄃᄄᄅᄆᄇᄈᄉᄊᄌᄍᄎᄏᄐᄑᄒ]ᅬ다"s);

	SECTION("method to_root")
	{
		auto word = "wries"s;
		CHECK("wry"s == sfx_tests.to_root(word));
		CHECK("wry"s == word);
	}

	SECTION("method to_root_copy")
	{
		auto word = "wries"s;
		CHECK("wry"s == sfx_tests.to_root_copy(word));
		CHECK("wries"s == word);
	}

	SECTION("method to_derived")
	{
		auto word = "wry"s;
		CHECK("wries"s == sfx_tests.to_derived(word));
		CHECK("wries"s == word);
	}

	SECTION("method to_derived_copy")
	{
		auto word = "wry"s;
		CHECK("wries"s == sfx_tests.to_derived_copy(word));
		CHECK("wry"s == word);
	}

	SECTION("method check_condition")
	{
		CHECK(true == sfx_tests.check_condition("wry"s));
		CHECK(false == sfx_tests.check_condition("wries"s));
	}
}
TEST_CASE("class String_Set", "[structures]")
{
	auto ss1 = String_Set<char>();
	auto ss2 = String_Set<char>();
	auto ss3 = String_Set<char>();
	ss1.insert("one");
	ss1.insert("two");
	ss1.insert("three");
	ss2.insert("one");
	ss2.insert("two");
	ss2.insert("three");
	ss3.insert("one");
	ss3.insert("two");

	SECTION("method insert")
	{
		CHECK(ss1 == ss2);
		CHECK(ss1 != ss3);
	}
}
