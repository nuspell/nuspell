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
	CHECK(false == c2.match("aa"));
	CHECK(false == c2.match("ab"));
	CHECK(false == c2.match("aba"));
	CHECK(false == c2.match("b"));
	CHECK(false == c2.match("ba"));
	CHECK(false == c2.match("bab"));

	CHECK(false == c2.match_prefix(""));
	CHECK(true == c2.match_prefix("a"));
	CHECK(true == c2.match_prefix("aa"));
	CHECK(true == c2.match_prefix("ab"));
	CHECK(true == c2.match_prefix("aba"));
	CHECK(false == c2.match_prefix("b"));
	CHECK(false == c2.match_prefix("ba"));
	CHECK(false == c2.match_prefix("bab"));

	CHECK(false == c2.match_suffix(""));
	CHECK(true == c2.match_suffix("a"));
	CHECK(true == c2.match_suffix("aa"));
	CHECK(false == c2.match_suffix("ab"));
	CHECK(true == c2.match_suffix("aba"));
	CHECK(false == c2.match_suffix("b"));
	CHECK(true == c2.match_suffix("ba"));
	CHECK(false == c2.match_suffix("bab"));

	auto c3 = Condition<char>("ba");
	CHECK(false == c3.match(""));
	CHECK(false == c3.match("b"));
	CHECK(true == c3.match("ba"));
	CHECK(false == c3.match("bab"));
	CHECK(false == c3.match("a"));
	CHECK(false == c3.match("aa"));
	CHECK(false == c3.match("ab"));
	CHECK(false == c3.match("aba"));

	CHECK(false == c3.match_prefix(""));
	CHECK(false == c3.match_prefix("b"));
	CHECK(true == c3.match_prefix("ba"));
	CHECK(true == c3.match_prefix("bab"));
	CHECK(false == c3.match_prefix("a"));
	CHECK(false == c3.match_prefix("aa"));
	CHECK(false == c3.match_prefix("ab"));
	CHECK(false == c3.match_prefix("aba"));

	CHECK(false == c3.match_suffix(""));
	CHECK(false == c3.match_suffix("b"));
	CHECK(true == c3.match_suffix("ba"));
	CHECK(false == c3.match_suffix("bab"));
	CHECK(false == c3.match_suffix("a"));
	CHECK(false == c3.match_suffix("aa"));
	CHECK(false == c3.match_suffix("ab"));
	CHECK(true == c3.match_suffix("aba"));
}

TEST_CASE("method match wildcards with wide character <wchar_t>", "[condition]")
{
	auto c1 = Condition<wchar_t>(L".");
	CHECK(false == c1.match_prefix(L""));
	CHECK(true == c1.match_prefix(L"a"));
	CHECK(true == c1.match_prefix(L"b"));
	CHECK(true == c1.match_prefix(L"aa"));
	CHECK(true == c1.match_prefix(L"ab"));
	CHECK(true == c1.match_prefix(L"ba"));
	CHECK(true == c1.match_prefix(L"bab"));
	CHECK(true == c1.match_prefix(L"aba"));

	auto c2 = Condition<wchar_t>(L"..");
	CHECK(false == c2.match_prefix(L""));
	CHECK(false == c2.match_prefix(L"a"));
	CHECK(false == c2.match_prefix(L"b"));
	CHECK(true == c2.match_prefix(L"aa"));
	CHECK(true == c2.match_prefix(L"ab"));
	CHECK(true == c2.match_prefix(L"ba"));
	CHECK(true == c2.match_prefix(L"bab"));
	CHECK(true == c2.match_prefix(L"aba"));
}

#if 0
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
#endif
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
	auto c1 = Condition<wchar_t>(L"áåĳßøæ");
	CHECK(true == c1.match(L"áåĳßøæ"));
	CHECK(false == c1.match(L"ñ"));
}

TEST_CASE("method match real-life combinations", "[condition]")
{
	// found 2 times in affix files
	auto c1 = Condition<wchar_t>(L"[áéiíóőuúüűy-àùø]");
	CHECK(true == c1.match(L"á"));
	CHECK(true == c1.match(L"é"));
	CHECK(true == c1.match(L"i"));
	CHECK(true == c1.match(L"í"));
	CHECK(true == c1.match(L"ó"));
	CHECK(true == c1.match(L"ő"));
	CHECK(true == c1.match(L"u"));
	CHECK(true == c1.match(L"ú"));
	CHECK(true == c1.match(L"ü"));
	CHECK(true == c1.match(L"ű"));
	CHECK(true == c1.match(L"y"));
	CHECK(true == c1.match(L"-"));
	CHECK(true == c1.match(L"à"));
	CHECK(true == c1.match(L"ù"));
	CHECK(true == c1.match(L"ø"));
	CHECK(false == c1.match(L"ñ"));

	// found 850 times in affix files
	auto c2 = Condition<wchar_t>(L"[cghjmsxyvzbdfklnprt-]");
	CHECK(true == c2.match(L"b"));
	CHECK(true == c2.match(L"i"));
	CHECK(false == c2.match(L"ñ"));

	// found 744 times in affix files
	auto c3 = Condition<wchar_t>(L"[áéiíóőuúüűy-àùø]");
	CHECK(true == c3.match(L"ő"));
	CHECK(true == c3.match(L"-"));
	CHECK(false == c3.match(L"ñ"));

	// found 8 times in affix files
	auto c4 = Condition<wchar_t>(L"[^-]");
	CHECK(true == c4.match(L"a"));
	CHECK(true == c4.match(L"b"));
	CHECK(false == c4.match(L"-"));

	// found 4 times in affix files
	auto c5 = Condition<wchar_t>(L"[^cts]Z-");
	CHECK(true == c5.match(L"aZ-"));
	CHECK(false == c5.match(L"cZ-"));
	CHECK(false == c5.match(L"Z-"));

	// found 2 times in affix files
	// TODO Question: identiecal to [6cug][^-] ? What is valid and what not?
	auto c6 = Condition<wchar_t>(L"[^cug^-]er");

	// found 74 times in affix files
	auto c7 = Condition<wchar_t>(L"[^дж]ерти");
	CHECK(true == c7.match(L"рерти"));
	CHECK(false == c7.match(L"ерти"));
	CHECK(false == c7.match(L"дерти"));
}
