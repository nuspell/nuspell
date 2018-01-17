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

#include "../src/nuspell/string_utils.hxx"

using namespace std;
using namespace std::literals;
using namespace hunspell;

TEST_CASE("method split_on_any_of", "[string_utils]")
{
	auto in = string("^abc;.qwe/zxc/");
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, string(".;^/"), back_inserter(out));
	CHECK(exp == out);
}

TEST_CASE("method split", "[string_utils]")
{
	// tests split(), also tests split_v

	auto in = string(";abc;;qwe;zxc;");
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_v(in, ';', out);
	CHECK(exp == out);

	in = "<>1<>234<>qwe<==><><>";
	exp = {"", "1", "234", "qwe<==>", "", ""};
	split_v(in, "<>", out);
	CHECK(exp == out);
}

TEST_CASE("method split_first", "[string_utils]")
{
	auto in = string("first\tsecond");
	auto first = string("first");
	auto out = split_first(in, '\t');
	CHECK(first == out);

	in = string("first");
	out = split_first(in, '\t');
	CHECK(first == out);

	in = string("\tsecond");
	first = string("");
	out = split_first(in, '\t');
	CHECK(first == out);

	in = string("");
	first = string("");
	out = split_first(in, '\t');
	CHECK(first == out);
}

TEST_CASE("method split_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = string("   qwe ert  \tasd ");
	auto exp = vector<string>{"qwe", "ert", "asd"};
	auto out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = string("   \t\r\n  ");
	exp = vector<string>{};
	// out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
}

TEST_CASE("method capitalize", "[string_utils]")
{
	auto in = string("");
	CHECK("" == capitalize(in));
	in = string("a");
	CHECK("A" == capitalize(in));
	CHECK("a" == in); // Check that input has not been affected.
	in = string("A");
	CHECK("A" == capitalize(in));

	in = string("car");
	CHECK("Car" == capitalize(in));
	in = string("Car");
	CHECK("Car" == capitalize(in));
	in = string("cAr");
	CHECK("CAr" == capitalize(in));
	in = string("CAr");
	CHECK("CAr" == capitalize(in));
	in = string("caR");
	CHECK("CaR" == capitalize(in));
	in = string("CaR");
	CHECK("CaR" == capitalize(in));
	in = string("cAR");
	CHECK("CAR" == capitalize(in));
	in = string("CAR");
	CHECK("CAR" == capitalize(in));

	in = string("ελλάδα");
	// FIXME	CHECK("Ελλάδα" == capitalize(in));
	in = string("Ελλάδα");
	CHECK("Ελλάδα" == capitalize(in));
	in = string("ΕΛΛΆΔΑ");
	CHECK("ΕΛΛΆΔΑ" == capitalize(in));
	in = string("σίγμα");
	// FIXME	CHECK("Σίγμα" == capitalize(in));
	in = string("ςίγμα");
	// FIXME	CHECK("Σίγμα" == capitalize(in));
	in = string("Σίγμα");
	CHECK("Σίγμα" == capitalize(in));

	in = string("ij");
	CHECK("IJ" == capitalize(in, true));
	in = string("Ij");
	CHECK("IJ" == capitalize(in, true));
	in = string("iJ");
	CHECK("IJ" == capitalize(in, true));
	in = string("IJ");
	CHECK("IJ" == capitalize(in, true));
	in = string("ijsselmeer");
	CHECK("IJsselmeer" == capitalize(in, true));
	in = string("Ijsselmeer");
	CHECK("IJsselmeer" == capitalize(in, true));
	in = string("iJsselmeer");
	CHECK("IJsselmeer" == capitalize(in, true));
	in = string("IJsselmeer");
	CHECK("IJsselmeer" == capitalize(in, true));

	in = string("ĳ");
	// FIXME	CHECK("Ĳ" == capitalize(in));
	in = string("Ĳ");
	CHECK("Ĳ" == capitalize(in));
	in = string("ĳsselmeer");
	// FIXME	CHECK("Ĳsselmeer" == capitalize(in));
	in = string("Ĳsselmeer");
	CHECK("Ĳsselmeer" == capitalize(in));
	in = string("ĳsselmeer");
	// FIXME	CHECK("Ĳsselmeer" == capitalize(in, true));
	in = string("Ĳsselmeer");
	CHECK("Ĳsselmeer" == capitalize(in, true));
}

TEST_CASE("method classify_casing", "[string_utils]")
{
	// TODO this test
	CHECK(Casing::SMALL == classify_casing("alllowercase"s));
	CHECK(Casing::SMALL == classify_casing("alllowercase3"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase"s));
	CHECK(Casing::INIT_CAPITAL == classify_casing("Initandlowercase_"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE"s));
	CHECK(Casing::ALL_CAPITAL == classify_casing("ALLUPPERCASE."s));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase"s));
	CHECK(Casing::CAMEL == classify_casing("iCamelCase@"s));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase"s));
	CHECK(Casing::PASCAL == classify_casing("InitCamelCase "s));
}
