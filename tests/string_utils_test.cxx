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

#include <boost/locale.hpp>
#include <iostream>

#include "../src/nuspell/locale_utils.hxx"
#include "../src/nuspell/string_utils.hxx"

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method split_on_any_of", "[string_utils]")
{
	auto in = "^abc;.qwe/zxc/"s;
	auto exp = vector<string>{"", "abc", "", "qwe", "zxc", ""};
	auto out = vector<string>();
	split_on_any_of(in, ".;^/"s, back_inserter(out));
	CHECK(exp == out);
}

TEST_CASE("method split", "[string_utils]")
{
	// tests split(), also tests split_v

	auto in = ";abc;;qwe;zxc;"s;
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
	auto in = "first\tsecond"s;
	auto first = "first"s;
	auto out = split_first(in, '\t');
	CHECK(first == out);

	in = "first"s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = "\tsecond"s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);

	in = ""s;
	first = ""s;
	out = split_first(in, '\t');
	CHECK(first == out);
}

TEST_CASE("method split_on_whitespace", "[string_utils]")
{
	// also tests _v variant

	auto in = "   qwe ert  \tasd "s;
	auto exp = vector<string>{"qwe", "ert", "asd"};
	auto out = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);

	in = "   \t\r\n  "s;
	exp = vector<string>();
	split_on_whitespace_v(in, out);
	CHECK(exp == out);
}

TEST_CASE("method is_number", "[string_utils]")
{
	CHECK(false == is_number(""s));
	CHECK(false == is_number("a"s));
	CHECK(false == is_number("1a"s));
	CHECK(false == is_number("a1"s));
	CHECK(false == is_number(".a"s));
	CHECK(false == is_number("a."s));
	CHECK(false == is_number(",a"s));
	CHECK(false == is_number("a,"s));
	CHECK(false == is_number("-a"s));
	CHECK(false == is_number("a-"s));

	CHECK(false == is_number("1..1"s));
	CHECK(false == is_number("1.,1"s));
	CHECK(false == is_number("1.-1"s));
	CHECK(false == is_number("1,.1"s));
	CHECK(false == is_number("1,,1"s));
	CHECK(false == is_number("1,-1"s));
	CHECK(false == is_number("1-.1"s));
	CHECK(false == is_number("1-,1"s));
	CHECK(false == is_number("1--1"s));

	CHECK(true == is_number("1,1111"s));
	CHECK(true == is_number("-1,1111"s));
	CHECK(true == is_number("1,1111.00"s));
	CHECK(true == is_number("-1,1111.00"s));
	CHECK(true == is_number("1.1111"s));
	CHECK(true == is_number("-1.1111"s));
	CHECK(true == is_number("1.1111,00"s));
	CHECK(true == is_number("-1.1111,00"s));

	// below needs extra review

	CHECK(true == is_number("1"s));
	CHECK(true == is_number("-1"s));
	CHECK(false == is_number("1-"s));

	CHECK(false == is_number("1."s));
	CHECK(false == is_number("-1."s));
	CHECK(false == is_number("1.-"s));

	CHECK(false == is_number("1,"s));
	CHECK(false == is_number("-1,"s));
	CHECK(false == is_number("1,-"s));

	CHECK(true == is_number("1.1"s));
	CHECK(true == is_number("-1.1"s));
	CHECK(false == is_number("1.1-"s));

	CHECK(true == is_number("1,1"s));
	CHECK(true == is_number("-1,1"s));
	CHECK(false == is_number("1,1-"s));

	CHECK(false == is_number(".1"s));
	CHECK(false == is_number("-.1"s));
	CHECK(false == is_number(".1-"s));

	CHECK(false == is_number(",1"s));
	CHECK(false == is_number("-,1"s));
	CHECK(false == is_number(",1-"s));
}
