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

#include <nuspell/structures.hxx>

#include <catch2/catch.hpp>

using namespace std;
using namespace std::literals::string_literals;
using namespace nuspell;

TEST_CASE("method substring replace copy", "[structures]")
{
	using Substring_Replacer = Substr_Replacer<char>;
	auto rep = Substring_Replacer({{"aa", "bb"},
	                               {"c", "d"},
	                               {"ee", "f"},
	                               {"g", "hh"},
	                               {"ii  ", ""},
	                               {"jj kk", "ll"},
	                               {"", "mm"},
	                               {" nn", ""}});
	CHECK(rep.replace_copy("aa XYZ c ee g ii jj kk nn"s) ==
	      "bb XYZ d f hh ii ll");
}

// TODO add a third TEXT_CASE for twofold suffix stripping

TEST_CASE("class Prefix_Entry", "[structures]")
{
	// TODO ignore "0" to make method more failsafe? See aff_data.cxx with
	// elem.stripping == "0"
	// auto pfx_tests = nuspell::Prefix_Entry(u'U', true, "0"s, "un"s,
	// "wr."s);

	auto pfx_tests =
	    Prefix<char>(u'U', true, ""s, "un"s, Flag_Set(), "wr."s);

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
	auto sfx_tests =
	    Suffix<char>(u'T', true, "y"s, "ies"s, Flag_Set(), ".[^aeiou]y"s);
	auto sfx_sk_SK = Suffix<char>(u'Z', true, "ata"s, "át"s, Flag_Set(),
	                              "[^áéíóúý].[^iš]ata"s);
	auto sfx_pt_PT =
	    Suffix<char>(u'X', true, "er"s, "a"s, Flag_Set(), "[^cug^-]er"s);
	// TODO See above regarding "0"
	auto sfx_gd_GB =
	    Suffix<char>(u'K', true, "0"s, "-san"s, Flag_Set(), "[^-]"s);
	auto sfx_ar =
	    Suffix<char>(u'a', true, "ه"s, "ي"s, Flag_Set(), "[^ءؤأ]ه"s);
	auto sfx_ko =
	    Suffix<char>(24, true, "ᅬ다"s, " ᅫᆻ어"s, Flag_Set(),
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
		CHECK(false == sfx_tests.check_condition("ey"s));
		CHECK(false == sfx_tests.check_condition("wries"s));
	}
}

TEST_CASE("class String_Set", "[structures]")
{
	SECTION("initialization")
	{
		auto ss1 = String_Set<char16_t>();
		auto ss2 = String_Set<char16_t>(u"abc"s);
		auto ss3 = String_Set<char16_t>(u"abc");

		CHECK(0 == ss1.size());
		CHECK(ss2 == ss3);
		CHECK(u"abc"s == ss2.data());
	}

	SECTION("assignment")
	{
		auto ss1 = String_Set<char16_t>();
		auto ss2 = String_Set<char16_t>(u"abc");
		ss1 = u"abc"s;
		CHECK(ss1 == ss2);
		auto s = u16string(u"abc"s);
		ss1 = s;
		CHECK(ss1 == ss2);
	}

	SECTION("size")
	{
		auto ss1 = String_Set<char16_t>();
		CHECK(true == ss1.empty());
		ss1.insert(u"abc");
		ss1.insert(u"def");
		ss1.insert(u"ghi");
		CHECK(false == ss1.empty());
		CHECK(9 == ss1.size());
		CHECK(1024 < ss1.max_size());
		ss1.clear();
		CHECK(true == ss1.empty());
	}

	SECTION("iterators")
	{
		auto ss1 = String_Set<char16_t>();
		ss1.insert(u"aa");
		ss1.insert(u"bb");
		auto b = ss1.begin();
		auto e = ss1.end();
		b++;
		b++;
		CHECK(*b == *e);

		auto rb = ss1.rbegin();
		auto re = ss1.rend();
		rb++;
		rb++;
		CHECK(*rb == *re);

		auto cb = ss1.cbegin();
		auto ce = ss1.cend();
		cb++;
		cb++;
		CHECK(*cb == *ce);

		auto crb = ss1.crbegin();
		auto cre = ss1.crend();
		crb++;
		crb++;
		CHECK(*crb == *cre);

		auto lba = ss1.lower_bound('a');
		auto lbb = ss1.lower_bound('b');
		auto uba = ss1.upper_bound('a');
		auto ubb = ss1.upper_bound('b');
		CHECK(*uba == *lbb);
		lba++;
		lba++;
		CHECK(*lba == *ubb);

		auto res = ss1.find('b');
		CHECK(*e == *res);
		auto p = ss1.equal_range('b');
		b = ss1.begin();
		b++;
		CHECK(*b == *(p.first));
		CHECK(*e == *(p.second));
	}

	SECTION("manipulation")
	{
		auto ss1 = String_Set<char16_t>();
		auto ss2 = String_Set<char16_t>(u"abc");
		ss1 = u"abc"s;

		CHECK(ss1 == ss2);
		ss1.erase(ss1.begin());
		auto ss3 = String_Set<char16_t>(u"bc");
		CHECK(ss1 == ss3);

		ss1.clear();
		CHECK(ss1 != ss3);
		CHECK(true == ss1.empty());

		ss2.erase(ss2.begin(), ss2.end());
		CHECK(true == ss2.empty());

		ss2.insert(u"abc");
		auto res = ss2.insert(ss2.find('b'), 'x');
		CHECK(*(ss2.begin() + 3) == *(res));
	}

	SECTION("comparison")
	{
		auto ss1 = String_Set<char16_t>();
		auto ss2 = String_Set<char16_t>();
		auto ss3 = String_Set<char16_t>();
		auto ss4 = String_Set<char16_t>();
		ss1.insert(u"abc");
		ss2.insert(u"abc");
		ss3.insert(u"abcd");
		ss4.insert(u"abcd");
		CHECK(ss1 == ss2);
		CHECK(ss3 == ss4);
		CHECK(ss1 != ss4);
		CHECK(ss3 != ss2);
		CHECK(ss1 < ss3);
		CHECK(ss4 > ss2);
		CHECK(ss1 <= ss2);
		CHECK(ss1 <= ss3);
		CHECK(ss3 >= ss4);
		CHECK(ss3 >= ss1);
		ss1.swap(ss3);
		CHECK(ss1 == ss4);
		CHECK(ss2 == ss3);
		CHECK(ss1 != ss2);
		CHECK(ss3 != ss4);
		CHECK(1 == ss3.count('c'));
		ss3.insert(u"c");
		ss3.insert(u"c");
		CHECK(1 == ss3.count('c'));
		CHECK(0 == ss3.count('z'));
	}
}

TEST_CASE("class Break_Table", "[structures]")
{
	auto a = Break_Table<char>();
	auto b = Break_Table<char>({"--"s, "-"s});
	auto c = Break_Table<char>({"--", "-"});
	auto d = Break_Table<char>();
	d = {"-", "--"};
	auto e = Break_Table<char>();
	e = {"-"s, "--"s};
	a = d;
}

TEST_CASE("class String_Pair", "[structures]")
{
	auto x = String_Pair<char>();
	CHECK(x.str() == "");
	CHECK(x.idx() == 0);
	CHECK(x.first() == "");
	CHECK(x.second() == "");

	x.first("123qwe");
	CHECK(x.str() == "123qwe");
	CHECK(x.idx() == 6);
	CHECK(x.first() == "123qwe");
	CHECK(x.second() == "");

	x.second("456z");
	CHECK(x.str() == "123qwe456z");
	CHECK(x.idx() == 6);
	CHECK(x.first() == "123qwe");
	CHECK(x.second() == "456z");

	x = String_Pair<char>("6789"s, "zxcvbnm"s);
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	x = String_Pair<char>("6789zxcvbnm", 4);
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	CHECK_NOTHROW(String_Pair<char>("6789"s, 4));
	CHECK_THROWS_AS(String_Pair<char>("6789", 5), std::out_of_range);
	CHECK_THROWS_WITH(String_Pair<char>("6789", 5),
	                  "word split is too long");
}

TEST_CASE("class Phonetic_Table", "[structures]")
{
	auto p1 = pair<string, string>({"CC", "_"});
	auto p2 = pair<string, string>({"AA", "BB"});
	auto p3 = pair<string, string>({"AH(AEIOUY)-^", "*H"});
	auto p4 = pair<string, string>({"A(HR)", "_"});
	auto p5 = pair<string, string>({"CC<", "C"});
	auto p6 = pair<string, string>({"", "BB"});
	auto p7 = pair<string, string>({"MB$", "M"});
	auto p8 = pair<string, string>({"GG9", "K"});
	auto v1 =
	    vector<pair<string, string>>({p1, p2, p3, p4, p5, p6, p7, p8});
	auto f1 = Phonetic_Table<char>(v1);
	auto f2 = Phonetic_Table<char>();
	f2 = v1;
	auto f3 = Phonetic_Table<char>(v1);
	auto f4 = Phonetic_Table<char>();
	auto f5 = Phonetic_Table<char>();

	auto p9 = pair<string, string>({"AA(", "N"}); // bad rule
	auto v2 = vector<pair<string, string>>({p9});
	f5 = v2;
	auto word = string("AA");
	auto exp = string("AA");
	CHECK(false == f5.replace(word));

	f5 = v1;
	word = string("AA");
	exp = string("BB");
	CHECK(true == f5.replace(word));
	CHECK(exp == word);

	word = string("CCF");
	exp = string("F");
	CHECK(true == f5.replace(word));
	CHECK(exp == word);

	word = string("AABB");
	exp = string("BBBB");
	CHECK(true == f5.replace(word));
	CHECK(exp == word);

	word = string("ABBA");
	exp = string(word);
	CHECK(false == f4.replace(word));
	CHECK(exp == word);
	CHECK(false == f5.replace(word));
	CHECK(exp == word);
}

TEST_CASE("class Similarity_Group", "[structures]")
{
	auto s1 = Similarity_Group<char>();
	s1.parse("a(bb)");
	s1.parse("c(dd"); // non-regular
	s1.parse("e(f)");
	s1.parse(")"); // non-regular
	auto s2 = Similarity_Group<char>();
	s2 = "(bb)a";
	s2 = "c(dd"; // non-regular
	s2 = "e";
	s2 = "f)"; // non-regular
	CHECK(s2.strings == s1.strings);
	CHECK(s2.chars == s1.chars);
	auto exp = string("acef)");
	CHECK(exp == s1.chars);
	auto v = vector<string>();
	v.push_back("bb");
	CHECK(v == s1.strings);
}

TEST_CASE("class List_Strings", "[structures]")
{
	auto l = List_Strings();
	CHECK(l.size() == 0);
	CHECK(begin(l) == end(l));
	l.push_back("1");
	l.push_back("2");
	l.push_back("3");
	CHECK(l.size() == 3);
	CHECK(begin(l) + 3 == end(l));
	CHECK(l[0] == "1");
	CHECK(l[1] == "2");
	CHECK(l[2] == "3");

	l.insert(begin(l) + 1, {"qwe", "asd"});
	CHECK(l.size() == 5);
	CHECK(begin(l) + 5 == end(l));
	CHECK(l[0] == "1");
	CHECK(l[1] == "qwe");
	CHECK(l[2] == "asd");
	CHECK(l[3] == "2");
	CHECK(l[4] == "3");

	l.erase(begin(l) + 3);
	CHECK(l.size() == 4);
	CHECK(begin(l) + 4 == end(l));
	CHECK(l[0] == "1");
	CHECK(l[1] == "qwe");
	CHECK(l[2] == "asd");
	CHECK(l[3] == "3");

	l.clear();
	CHECK(l.size() == 0);
	CHECK(begin(l) == end(l));
}
