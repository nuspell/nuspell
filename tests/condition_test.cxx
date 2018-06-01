/* Copyright 2018 Dimitrij Mijoski and Sander van Geloven
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
	auto c1 = Condition<char16_t>(u"[a]");
	CHECK(false == c1.match(u""));
	CHECK(true == c1.match(u"a"));
	CHECK(false == c1.match(u"b"));
	CHECK(true == c1.match(u"aa"));
	CHECK(true == c1.match(u"ab"));
	CHECK(true == c1.match(u"ba"));
	CHECK(true == c1.match(u"bab"));
	CHECK(true == c1.match(u"aba"));

	auto c2 = Condition<char16_t>(u"[aa]");
	CHECK(false == c2.match(u""));
	CHECK(true == c2.match(u"a"));
	CHECK(false == c2.match(u"b"));
	CHECK(true == c2.match(u"aa"));
	CHECK(true == c2.match(u"ab"));
	CHECK(true == c2.match(u"ba"));
	CHECK(true == c2.match(u"bab"));
	CHECK(true == c2.match(u"aba"));

	auto c3 = Condition<char16_t>(u"[ab]");
	CHECK(false == c3.match(u""));
	CHECK(true == c3.match(u"a"));
	CHECK(true == c3.match(u"b"));
	CHECK(true == c3.match(u"aa"));
	CHECK(true == c3.match(u"ab"));
	CHECK(true == c3.match(u"ba"));
	CHECK(true == c3.match(u"bab"));
	CHECK(true == c3.match(u"aba"));
}

TEST_CASE("method match selections with runtime exceptions", "[condition]")
{
	auto cond1 = u"]";
	try {
		auto c1 = Condition<char16_t>(cond1);
	} catch (const std::invalid_argument& e) {
		CHECK("Cannot construct Condition without opening bracket for condition " + cond1 == e.what());
	}
	auto cond2 = u"ab]";
	try {
		auto c2 = Condition<char16_t>(cond2);
	} catch (const std::invalid_argument& e) {
		CHECK("Cannot construct Condition without opening bracket for condition " + cond2 == e.what());
	}
	auto cond3 = u"[ab";
	try {
		auto c3 = Condition<char16_t>(cond3);
	} catch (const std::invalid_argument& e) {
		CHECK("Cannot construct Condition without closing bracket for condition " + cond3 == e.what());
	}
	auto cond4 = u"[";
	try {
		auto c4 = Condition<char16_t>(cond4);
	} catch (const std::invalid_argument& e) {
		CHECK("Cannot construct Condition without closing bracket for condition " + cond4 == e.what());
	}
	auto cond5 = u"[]";
	try {
		auto c5 = Condition<char16_t>(cond5);
	} catch (const std::invalid_argument& e) {
		CHECK("Cannot construct Condition with empty brackets for condition " + cond5 == e.what());
	}
}

#if 0
// Need to discuss this, related to optimization of .e.g. [abb] and [^abb] into [ab] and [^ab].
TEST_CASE("method match exceptions with runtime exceptions",
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
#endif

#if 0
// Might leave this out as hyphen doesn't have a special meaning.
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
#endif

TEST_CASE("method match diacritics and ligatues", "[condition]")
{
	auto c1 = Condition<char>(u8"áåĳßøæ");
	CHECK(true == c1.match(u8"áåĳßøæ"));
	CHECK(false == c1.match(u8"ñ"));
}

TEST_CASE("method match real-life combinations", "[condition]") {
	// found 2 times in affix files
	c1 = Condition<char>(u8"[áéiíóőuúüűy-àùø]");
	CHECK(true == c1.match(u8"á"));
	CHECK(true == c1.match(u8"é"));
	CHECK(true == c1.match(u8"i"));
	CHECK(true == c1.match(u8"í"));
	CHECK(true == c1.match(u8"ó"));
	CHECK(true == c1.match(u8"ő"));
	CHECK(true == c1.match(u8"u"));
	CHECK(true == c1.match(u8"ú"));
	CHECK(true == c1.match(u8"ü"));
	CHECK(true == c1.match(u8"ű"));
	CHECK(true == c1.match(u8"y"));
	CHECK(true == c1.match(u8"-"));
	CHECK(true == c1.match(u8"à"));
	CHECK(true == c1.match(u8"ù"));
	CHECK(true == c1.match(u8"ø"));
	CHECK(false == c1.match(u8"ñ"));

	// found 850 times in affix files
	c2 = Condition<char>(u8"[cghjmsxyvzbdfklnprt-]");
	CHECK(true == c2.match(u8"b"));
	CHECK(true == c2.match(u8"i"));
	CHECK(false == c2.match(u8"ñ"));

	// found 744 times in affix files
	c3 = Condition<char>(u8"[áéiíóőuúüűy-àùø]");
	CHECK(true == c3.match(u8"ő"));
	CHECK(true == c3.match(u8"-"));
	CHECK(false == c3.match(u8"ñ"));

	// found 8 times in affix files
	c4 = Condition<char>(u8"[^-]");
	CHECK(true == c4.match(u8"a"));
	CHECK(true == c4.match(u8"b"));
	CHECK(false == c4.match(u8"-"));

	// found 4 times in affix files
	c5 = Condition<char>(u8"[^cts]Z-");
	CHECK(true == c5.match(u8"aZ-"));
	CHECK(false == c5.match(u8"cZ-"));
	CHECK(false == c5.match(u8"Z-"));

	// found 2 times in affix files
	// TODO Question: identiecal to [6cug][^-] ? What is valid and what not?
	c6 = Condition<char>(u8"[^cug^-]er");

	// found 74 times in affix files
	auto c7 = Condition<char>(u8"[^дж]ерти");
	CHECK(true == c7.match(u8"рерти"));
	CHECK(false == c7.match(u8"ерти"));
	CHECK(false == c7.match(u8"дерти"));
}
