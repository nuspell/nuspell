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

#include "../src/nuspell/condition.hxx"
#include <boost/locale.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method match characters with standard character <char>",
          "[condition]")
{
	auto c1 = Condition<char>("");
	CHECK(true == c1.match(""));
	CHECK(false == c1.match("a"));

	auto c2 = Condition<char>("a");
	CHECK(false == c2.match(""));
	CHECK(true == c2.match("a"));
	CHECK(false == c2.match("b"));
	CHECK(true == c2.match("aa"));
	CHECK(true == c2.match("ab"));
	CHECK(true == c2.match("ba"));
	CHECK(true == c2.match("bab"));
	CHECK(true == c2.match("aba"));

	auto c3 = Condition<char>("ba");
	CHECK(false == c3.match(""));
	CHECK(false == c3.match("b"));
	CHECK(false == c3.match("a"));
	CHECK(false == c3.match("aa"));
	CHECK(false == c3.match("ab"));
	CHECK(true == c3.match("ba"));
	CHECK(true == c3.match("bab"));
	CHECK(true == c3.match("aba"));
}

TEST_CASE("method match wildcards with wide character <wchar_t>", "[condition]")
{
	auto c1 = Condition<wchar_t>(L".");
	CHECK(false == c1.match(L""));
	CHECK(true == c1.match(L"a"));
	CHECK(true == c1.match(L"b"));
	CHECK(true == c1.match(L"aa"));
	CHECK(true == c1.match(L"ab"));
	CHECK(true == c1.match(L"ba"));
	CHECK(true == c1.match(L"bab"));
	CHECK(true == c1.match(L"aba"));

	auto c2 = Condition<wchar_t>(L"..");
	CHECK(false == c2.match(L""));
	CHECK(false == c2.match(L"a"));
	CHECK(false == c2.match(L"b"));
	CHECK(true == c2.match(L"aa"));
	CHECK(true == c2.match(L"ab"));
	CHECK(true == c2.match(L"ba"));
	CHECK(true == c2.match(L"bab"));
	CHECK(true == c2.match(L"aba"));
}

TEST_CASE("method match begin and end with UTF-8 character <char>",
          "[condition]")
{
	auto c1 = Condition<char>(u8"^a");
	CHECK(false == c1.match(u8""));
	CHECK(true == c1.match(u8"a"));
	CHECK(false == c1.match(u8"b"));
	CHECK(true == c1.match(u8"aa"));
	CHECK(true == c1.match(u8"ab"));
	CHECK(false == c1.match(u8"ba"));
	CHECK(false == c1.match(u8"bab"));
	CHECK(true == c1.match(u8"aba"));

	auto c2 = Condition<char>(u8"a$");
	CHECK(false == c2.match(u8""));
	CHECK(true == c2.match(u8"a"));
	CHECK(false == c2.match(u8"b"));
	CHECK(true == c2.match(u8"aa"));
	CHECK(false == c2.match(u8"ab"));
	CHECK(true == c2.match(u8"ba"));
	CHECK(false == c2.match(u8"bab"));
	CHECK(true == c2.match(u8"aba"));

	auto c3 = Condition<char>(u8"^a$");
	CHECK(false == c3.match(u8""));
	CHECK(true == c3.match(u8"a"));
	CHECK(false == c3.match(u8"b"));
	CHECK(false == c3.match(u8"aa"));
	CHECK(false == c3.match(u8"ab"));
	CHECK(false == c3.match(u8"ba"));
	CHECK(false == c3.match(u8"bab"));
	CHECK(false == c3.match(u8"aba"));
}

TEST_CASE("method match selections with UTF-16 character <char16_t>",
          "[condition]")
{
	auto c1 = Condition<char16_t>(u"[]");
	// TODO Catch exception on construction or on matchon undefined or
	// error?

	auto c2 = Condition<char16_t>(u"[a]");
	CHECK(false == c2.match(u""));
	CHECK(true == c2.match(u"a"));
	CHECK(false == c2.match(u"b"));
	CHECK(true == c2.match(u"aa"));
	CHECK(true == c2.match(u"ab"));
	CHECK(true == c2.match(u"ba"));
	CHECK(true == c2.match(u"bab"));
	CHECK(true == c2.match(u"aba"));

	auto c3 = Condition<char16_t>(u"[aa]");
	CHECK(false == c3.match(u""));
	CHECK(true == c3.match(u"a"));
	CHECK(false == c3.match(u"b"));
	CHECK(true == c3.match(u"aa"));
	CHECK(true == c3.match(u"ab"));
	CHECK(true == c3.match(u"ba"));
	CHECK(true == c3.match(u"bab"));
	CHECK(true == c3.match(u"aba"));

	auto c4 = Condition<char16_t>(u"[ab]");
	CHECK(false == c4.match(u""));
	CHECK(true == c4.match(u"a"));
	CHECK(true == c4.match(u"b"));
	CHECK(true == c4.match(u"aa"));
	CHECK(true == c4.match(u"ab"));
	CHECK(true == c4.match(u"ba"));
	CHECK(true == c4.match(u"bab"));
	CHECK(true == c4.match(u"aba"));
}

TEST_CASE("method match exceptions with standard character <char>",
          "[condition]")
{
	auto c1 = Condition<char>("[^]");
	// TODO Catch exception on construction or on matchon undefined or
	// error?

	auto c2 = Condition<char>("[^a]");
	// TODO

	auto c3 = Condition<char>("[^aa]");
	// TODO

	auto c4 = Condition<char>("[^ab]");
	// TODO
}

TEST_CASE("method match hyphens with standard character <char>", "[condition]")
{
	auto c1 = Condition<char>("-");
	// TODO

	auto c2 = Condition<char>("-a");
	// TODO

	auto c3 = Condition<char>("a-");
	// TODO

	auto c4 = Condition<char>("-$");
	// TODO

	auto c5 = Condition<char>("^-");
	// TODO

	auto c6 = Condition<char>("[a-]");
	// TODO

	auto c7 = Condition<char>("[-a]");
	// TODO

	auto c8 = Condition<char>("[a-b]");
	// TODO

	auto c9 = Condition<char>("[b-a]");
	// TODO
}

// TEST_CASE("method match diacritics", "[condition]")
//{
//	auto c = Condition("áåĳßøæ");
// TODO
//
//	c = Condition("[áéiíóőuúüűy-àùø]"); // found two times in affix files
// TODO
//
// https://github.com/hunspell/nuspell/commit/b48bc2c916902c92afb20f243c4317414106d33e#diff-3c7b410b0f2a9f3e6e8f7011693dc65b
// https://github.com/hunspell/nuspell/commit/7541f31a900117a62d551de607f4c71467149597#diff-3c7b410b0f2a9f3e6e8f7011693dc65b
// https://github.com/hunspell/nuspell/commit/9fc57dcdb61e0a3d6a98f95bcaf1302632aa9f0d#diff-3c7b410b0f2a9f3e6e8f7011693dc65b
// https://github.com/hunspell/nuspell/commit/628a0c1ce67cad525ceff0d8d9e6006c4d331676#diff-3c7b410b0f2a9f3e6e8f7011693dc65b
//}

TEST_CASE("method match combinations", "[condition]") {}
