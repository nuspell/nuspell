/* Copyright 2018-2019 Sander van Geloven, Dimitrij Mijoski
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
using namespace nuspell;

TEST_CASE("Substr_Replacer", "[structures]")
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
	CHECK(rep.replace_copy("aa XYZ c ee g ii jj kk nn") ==
	      "bb XYZ d f hh ii ll");
}

// TODO add a third TEXT_CASE for twofold suffix stripping

TEST_CASE("Prefix", "[structures]")
{
	auto pfx_tests = Prefix<char>(u'U', true, "", "un", {}, "wr.");
	auto word = string("unwry");
	CHECK("wry" == pfx_tests.to_root(word));
	CHECK("wry" == word);

	word = string("unwry");
	CHECK("wry" == pfx_tests.to_root_copy(word));
	CHECK("unwry" == word);

	word = string("wry");
	CHECK("unwry" == pfx_tests.to_derived(word));
	CHECK("unwry" == word);

	word = string("wry");
	CHECK("unwry" == pfx_tests.to_derived_copy(word));
	CHECK("wry" == word);

	CHECK(true == pfx_tests.check_condition("wry"));
	CHECK(false == pfx_tests.check_condition("unwry"));
}

TEST_CASE("Suffix", "[structures]")
{
	auto sfx_tests = Suffix<char>(u'T', true, "y", "ies", {}, ".[^aeiou]y");
	auto sfx_sk_SK =
	    Suffix<char>(u'Z', true, "ata", "át", {}, "[^áéíóúý].[^iš]ata");
	auto sfx_pt_PT = Suffix<char>(u'X', true, "er", "a", {}, "[^cug^-]er");
	// TODO See above regarding "0"
	auto sfx_gd_GB = Suffix<char>(u'K', true, "", "-san", {}, "[^-]");
	auto sfx_ar = Suffix<char>(u'a', true, "ه", "ي", {}, "[^ءؤأ]ه");
	auto sfx_ko =
	    Suffix<char>(24, true, "ᅬ다", " ᅫᆻ어", {},
	                 "[ᄀᄁᄃᄄᄅᄆᄇᄈᄉᄊᄌᄍᄎᄏᄐᄑᄒ]ᅬ다");

	auto word = string("wries");
	CHECK("wry" == sfx_tests.to_root(word));
	CHECK("wry" == word);

	word = string("wries");
	CHECK("wry" == sfx_tests.to_root_copy(word));
	CHECK("wries" == word);

	word = string("wry");
	CHECK("wries" == sfx_tests.to_derived(word));
	CHECK("wries" == word);

	word = string("wry");
	CHECK("wries" == sfx_tests.to_derived_copy(word));
	CHECK("wry" == word);

	CHECK(true == sfx_tests.check_condition("wry"));
	CHECK(false == sfx_tests.check_condition("ey"));
	CHECK(false == sfx_tests.check_condition("wries"));
}

TEST_CASE("String_Set::String_Set", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u16string(u"abc"));
	auto ss3 = String_Set<char16_t>(u"abc");

	CHECK(0 == ss1.size());
	CHECK(ss2 == ss3);
	CHECK(u"abc" == ss2.data());
}

TEST_CASE("String_Set::operator=", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u"abc");
	ss1 = u"abc";
	CHECK(ss1 == ss2);
	auto s = u16string(u"abc");
	ss1 = s;
	CHECK(ss1 == ss2);
}

TEST_CASE("String_Set::size", "[structures]")
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

TEST_CASE("String_Set::begin end", "[structures]")
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

TEST_CASE("String_Set manipulation", "[structures]")
{
	auto ss1 = String_Set<char16_t>();
	auto ss2 = String_Set<char16_t>(u"abc");
	ss1 = u"abc";

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

TEST_CASE("String_Set comparison", "[structures]")
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

TEST_CASE("Break_Table", "[structures]")
{
	auto a = Break_Table<char>();
	auto b = Break_Table<char>({"--", "-"});
	auto c = Break_Table<char>({"--", "-"});
	auto d = Break_Table<char>();
	d = {"-", "--"};
	auto e = Break_Table<char>();
	e = {"-", "--"};
	a = d;
}

TEST_CASE("String_Pair", "[structures]")
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

	x = String_Pair<char>(string("6789"), string("zxcvbnm"));
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	x = String_Pair<char>("6789zxcvbnm", 4);
	CHECK(x.str() == "6789zxcvbnm");
	CHECK(x.idx() == 4);
	CHECK(x.first() == "6789");
	CHECK(x.second() == "zxcvbnm");

	CHECK_NOTHROW(String_Pair<char>("6789", 4));
	CHECK_THROWS_AS(String_Pair<char>("6789", 5), std::out_of_range);
	CHECK_THROWS_WITH(String_Pair<char>("6789", 5),
	                  "word split is too long");
}

TEST_CASE("Phonetic_Table", "[structures]")
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

TEST_CASE("Similarity_Group", "[structures]")
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

TEST_CASE("List_Strings", "[structures]")
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
