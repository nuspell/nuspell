/* Copyright 2021-2023 Dimitrij Mijoski
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

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <nuspell/dictionary.hxx>
#include <nuspell/utils.hxx>

using namespace std;
using namespace nuspell;

TEST_CASE("Subrange")
{
	auto str = "abc"s;
	auto s = Subrange(str);
	STATIC_REQUIRE(is_same_v<decltype(s), Subrange<string::iterator>>);
	REQUIRE(s.begin() == begin(str));
	REQUIRE(s.end() == end(str));
}

TEST_CASE("String_Set")
{
	auto ss = String_Set(u"zaZAa");
	STATIC_REQUIRE(is_same_v<decltype(ss), String_Set<char16_t>>);
	REQUIRE(ss == String_Set(u"AZaz"));
	REQUIRE(ss.str() == u"AZaz");
	REQUIRE(ss.size() == 4);
	REQUIRE_FALSE(ss.empty());
	REQUIRE(ss.contains(u'a'));
	REQUIRE(ss.contains(u'A'));
	REQUIRE(ss.contains(u'z'));
	REQUIRE(ss.contains(u'Z'));
	REQUIRE_FALSE(ss.contains(u'\0'));
	REQUIRE_FALSE(ss.contains(u'b'));
	REQUIRE_FALSE(ss.contains(u'B'));
	REQUIRE(ss.lower_bound(u'a') == ss.begin() + 2);
	REQUIRE(ss.upper_bound(u'a') == ss.begin() + 3);
	REQUIRE(ss.equal_range(u'a') == pair(ss.begin() + 2, ss.begin() + 3));
	REQUIRE(ss.lower_bound(u'b') == ss.begin() + 3);
	REQUIRE(ss.upper_bound(u'b') == ss.begin() + 3);
	REQUIRE(ss.equal_range(u'b') == pair(ss.begin() + 3, ss.begin() + 3));

	ss.insert(u'b');
	REQUIRE(ss == String_Set(u"AZabz"));
	REQUIRE(ss.str() == u"AZabz");
	REQUIRE(ss.size() == 5);
	REQUIRE_FALSE(ss.empty());
	REQUIRE(ss.contains(u'a'));
	REQUIRE(ss.contains(u'A'));
	REQUIRE(ss.contains(u'z'));
	REQUIRE(ss.contains(u'Z'));
	REQUIRE(ss.contains(u'b'));
	REQUIRE_FALSE(ss.contains(u'\0'));
	REQUIRE_FALSE(ss.contains(u'B'));

	ss.erase(u'A');
	ss.erase(u'b');
	REQUIRE(ss == String_Set(u"Zaz"));
	REQUIRE(ss.str() == u"Zaz");
	REQUIRE(ss.size() == 3);
	REQUIRE_FALSE(ss.empty());
	REQUIRE(ss.contains(u'a'));
	REQUIRE(ss.contains(u'z'));
	REQUIRE(ss.contains(u'Z'));
	REQUIRE_FALSE(ss.contains(u'\0'));
	REQUIRE_FALSE(ss.contains(u'A'));
	REQUIRE_FALSE(ss.contains(u'b'));
	REQUIRE_FALSE(ss.contains(u'B'));
}

TEST_CASE("Substr_Replacer")
{
	auto rep = Substr_Replacer({{"asd", "zxc"},
	                            {"as", "rtt"},
	                            {"a", "A"},
	                            {"abbb", "ABBB"},
	                            {"asd  ", ""},
	                            {"asd ZXC", "YES"},
	                            {"sd ZXC as", "NO"},
	                            {"", "123"},
	                            {" TT", ""}});
	REQUIRE(rep.replace_copy("QWE asd ZXC as TT") == "QWE YES rtt");
}

TEST_CASE("Break_Table")
{
	auto b = Break_Table({
	    "bsd",
	    "zxc",
	    "asd",
	    "^bar",
	    "^zoo",
	    "^abc",
	    "car$",
	    "yoyo$",
	    "air$",
	});
	auto swb = b.start_word_breaks();
	auto swb_v = vector<string>(begin(swb), end(swb));
	sort(begin(swb_v), end(swb_v));
	REQUIRE(swb_v == vector{"abc"s, "bar"s, "zoo"s});

	auto mwb = b.middle_word_breaks();
	auto mwb_v = vector<string>(begin(mwb), end(mwb));
	sort(begin(mwb_v), end(mwb_v));
	REQUIRE(mwb_v == vector{"asd"s, "bsd"s, "zxc"s});

	auto ewb = b.end_word_breaks();
	auto ewb_v = vector<string>(begin(ewb), end(ewb));
	sort(begin(ewb_v), end(ewb_v));
	REQUIRE(ewb_v == vector{"air"s, "car"s, "yoyo"s});
}

TEST_CASE("Hash_Multimap")
{
	auto h = Hash_Multimap<string, int>();
	REQUIRE(h.empty());
	REQUIRE(h.size() == 0);
	auto res = h.equal_range("hello");
#if !defined(_GLIBCXX_DEBUG) || _GLIBCXX_RELEASE >= 11
	// ifdef because of
	// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70303
	REQUIRE(res.first == res.second);
#endif

	h.emplace("hello", 5);
	res = h.equal_range("hello");
	REQUIRE_FALSE(h.empty());
	REQUIRE(h.size() == 1);
	REQUIRE(res.first != res.second);
	REQUIRE(next(res.first) == res.second);
	REQUIRE(*res.first == pair("hello"s, 5));
	res = h.equal_range("Hi");
	REQUIRE(res.first == res.second);
	res = h.equal_range("");
	REQUIRE(res.first == res.second);

	h.emplace("hello", 4);
	res = h.equal_range("hello");
	REQUIRE_FALSE(h.empty());
	REQUIRE(h.size() == 2);
	REQUIRE(res.first != res.second);
	REQUIRE(next(res.first, 2) == res.second);
	REQUIRE(*res.first == pair("hello"s, 5));
	REQUIRE(*next(res.first) == pair("hello"s, 4));
	res = h.equal_range("Hi");
	REQUIRE(res.first == res.second);
	res = h.equal_range("");
	REQUIRE(res.first == res.second);
}

TEST_CASE("Condition")
{
	auto c = Condition();
	REQUIRE(c.match_prefix(""));
	REQUIRE(c.match_prefix("a"));

	REQUIRE(c.match_suffix(""));
	REQUIRE(c.match_suffix("b"));

	c = "abcd";
	REQUIRE(c.match_prefix("abcd"));
	REQUIRE(c.match_prefix("abcdXYZ"));
	REQUIRE(c.match_prefix("abcdБВГДШ\uABCD\U0010ABCD"));
	REQUIRE_FALSE(c.match_prefix(""));
	REQUIRE_FALSE(c.match_prefix("abc"));
	REQUIRE_FALSE(c.match_prefix("abcX"));
	REQUIRE_FALSE(c.match_prefix("XYZ"));
	REQUIRE_FALSE(c.match_prefix("АаБбВвГгШш\uABCD\U0010ABCD"));

	REQUIRE(c.match_suffix("abcd"));
	REQUIRE(c.match_suffix("XYZabcd"));
	REQUIRE(c.match_suffix("БВГДШ\uABCD\U0010ABCDabcd"));
	REQUIRE_FALSE(c.match_suffix(""));
	REQUIRE_FALSE(c.match_suffix("bcd"));
	REQUIRE_FALSE(c.match_suffix("Xbcd"));
	REQUIRE_FALSE(c.match_suffix("XYZ"));
	REQUIRE_FALSE(c.match_suffix("АаБбВвГгШш\uABCD\U0010ABCD"));

	c = ".";
	REQUIRE(c.match_prefix("Y"));
	REQUIRE(c.match_prefix("abc"));
	REQUIRE(c.match_prefix("БВГДШ\uABCD\U0010ABCD"));
	REQUIRE_FALSE(c.match_prefix(""));

	REQUIRE(c.match_suffix("Y"));
	REQUIRE(c.match_suffix("qwe"));
	REQUIRE(c.match_suffix("БВГДШ\uABCD\U0010ABCD"));
	REQUIRE_FALSE(c.match_suffix(""));

	c = "[vbn]";
	REQUIRE(c.match_prefix("v"));
	REQUIRE(c.match_prefix("vAAш"));
	REQUIRE(c.match_prefix("b"));
	REQUIRE(c.match_prefix("bBBш"));
	REQUIRE(c.match_prefix("n"));
	REQUIRE(c.match_prefix("nCCш"));
	REQUIRE_FALSE(c.match_prefix(""));
	REQUIRE_FALSE(c.match_prefix("Q"));
	REQUIRE_FALSE(c.match_prefix("Qqqq"));
	REQUIRE_FALSE(c.match_prefix("1342234"));
	REQUIRE_FALSE(c.match_prefix("V"));
	REQUIRE_FALSE(c.match_prefix("бвгдш"));

	REQUIRE(c.match_suffix("v"));
	REQUIRE(c.match_suffix("шVVv"));
	REQUIRE(c.match_suffix("b"));
	REQUIRE(c.match_suffix("шBBb"));
	REQUIRE(c.match_suffix("n"));
	REQUIRE(c.match_suffix("шNNn"));
	REQUIRE_FALSE(c.match_suffix(""));
	REQUIRE_FALSE(c.match_suffix("Q"));
	REQUIRE_FALSE(c.match_suffix("Qqqq"));
	REQUIRE_FALSE(c.match_suffix("123123"));
	REQUIRE_FALSE(c.match_suffix("V"));
	REQUIRE_FALSE(c.match_suffix("бвгдш"));

	c = "[бш\u1234]";
	REQUIRE(c.match_prefix("б"));
	REQUIRE(c.match_prefix("беТ"));
	REQUIRE(c.match_prefix("ш"));
	REQUIRE(c.match_prefix("шок"));
	REQUIRE(c.match_prefix("\u1234кош"));
	REQUIRE_FALSE(c.match_prefix(""));
	REQUIRE_FALSE(c.match_prefix("Q"));
	REQUIRE_FALSE(c.match_prefix("Qqqq"));
	REQUIRE_FALSE(c.match_prefix("пан"));
	REQUIRE_FALSE(c.match_prefix("\uABCD\u1234"));
	REQUIRE_FALSE(c.match_prefix("вбгдш"));

	REQUIRE(c.match_suffix("б"));
	REQUIRE(c.match_suffix("еТб"));
	REQUIRE(c.match_suffix("ш"));
	REQUIRE(c.match_suffix("кош"));
	REQUIRE(c.match_suffix("кош\u1234"));
	REQUIRE_FALSE(c.match_suffix(""));
	REQUIRE_FALSE(c.match_suffix("Q"));
	REQUIRE_FALSE(c.match_suffix("Qqqq"));
	REQUIRE_FALSE(c.match_suffix("пан"));
	REQUIRE_FALSE(c.match_suffix("\u1234\uABCD"));
	REQUIRE_FALSE(c.match_suffix("вбгдз"));

	c = "[^zш\u1234\U0010ABCD]";
	REQUIRE_FALSE(c.match_prefix("z"));
	REQUIRE_FALSE(c.match_prefix("ш"));
	REQUIRE_FALSE(c.match_prefix("\u1234"));
	REQUIRE_FALSE(c.match_prefix("\U0010ABCD"));
	REQUIRE_FALSE(c.match_prefix("zљње"));
	REQUIRE_FALSE(c.match_prefix("шabc"));
	REQUIRE_FALSE(c.match_prefix("\u1234 ytyty"));
	REQUIRE_FALSE(c.match_prefix("\U0010ABCD tytyty"));
	REQUIRE(c.match_prefix("q"));
	REQUIRE(c.match_prefix("r"));
	REQUIRE(c.match_prefix("\uFFFD"));
	REQUIRE(c.match_prefix("\U0010FFFF"));
	REQUIRE(c.match_prefix("qљње"));
	REQUIRE(c.match_prefix("фabc"));
	REQUIRE(c.match_prefix("\uFFFD ytyty"));
	REQUIRE(c.match_prefix("\U0010FFFF tytyty"));

	REQUIRE_FALSE(c.match_suffix("z"));
	REQUIRE_FALSE(c.match_suffix("ш"));
	REQUIRE_FALSE(c.match_suffix("\u1234"));
	REQUIRE_FALSE(c.match_suffix("\U0010ABCD"));
	REQUIRE_FALSE(c.match_suffix("љњеz"));
	REQUIRE_FALSE(c.match_suffix("abcш"));
	REQUIRE_FALSE(c.match_suffix("ytyty \u1234"));
	REQUIRE_FALSE(c.match_suffix("tytyty \U0010ABCD"));
	REQUIRE(c.match_suffix("q"));
	REQUIRE(c.match_suffix("r"));
	REQUIRE(c.match_suffix("\uFFFD"));
	REQUIRE(c.match_suffix("\U0010FFFF"));
	REQUIRE(c.match_suffix("љњеq"));
	REQUIRE(c.match_suffix("abcф"));
	REQUIRE(c.match_suffix("ytyty \uFFFD"));
	REQUIRE(c.match_suffix("tytyty \U0010FFFF"));

	c = "abc АБВ..[zбш\u1234][^zш\u1234\U0010ABCD]X";
	REQUIRE(c.match_prefix("abc АБВ \u2345z\U00011111X"));
	REQUIRE(c.match_prefix("abc АБВ\u2345 ш\U00011112Xопop"));
	REQUIRE_FALSE(c.match_prefix("abc ШШШ \u2345z\U00011111X"));
	REQUIRE_FALSE(c.match_prefix("abc АБВ\u2345 t\U00011112Xопop"));
	REQUIRE_FALSE(c.match_prefix("abc АБВ \u2345z\u1234X"));
}

TEST_CASE("Prefix")
{
	auto pfx = Prefix{'F', false, "qw", "Qwe", {}, {}};
	REQUIRE(pfx.to_derived_copy("qwrty") == "Qwerty");
	REQUIRE(pfx.to_root_copy("Qwerty") == "qwrty");
}

TEST_CASE("Suffix")
{
	auto sfx = Suffix{'F', false, "ie", "ying", {}, {}};
	REQUIRE(sfx.to_derived_copy("pie") == "pying");
	REQUIRE(sfx.to_root_copy("pying") == "pie");
}

TEST_CASE("Prefix_Multiset")
{
	auto set =
	    Prefix_Multiset<string>{"",     "a",   "",   "ab",   "abx", "as",
	                            "asdf", "axx", "as", "bqwe", "ba",  "rqwe"};
	auto expected = vector<string>{"", "", "a", "as", "as", "asdf"};
	auto out = vector<string>();
	set.copy_all_prefixes_of("asdfg", back_inserter(out));
	CHECK(out == expected);

	auto word = string("asdfg");
	auto it = set.iterate_prefixes_of(word);
	out.assign(begin(it), end(it));
	REQUIRE(out == expected);
}

TEST_CASE("Suffix_Multiset")
{
	auto set =
	    Suffix_Multiset<string>{"",   "",   "a",   "b",   "b",   "ab",
	                            "ub", "zb", "aub", "uub", "xub", "huub"};
	auto expected = vector<string>{"", "", "b", "b", "ub", "uub", "huub"};
	auto out = vector<string>();
	auto word = string("ahahuub");
	set.copy_all_prefixes_of(word, back_inserter(out));
	CHECK(out == expected);

	auto it = set.iterate_prefixes_of(word);
	out.assign(begin(it), end(it));
	REQUIRE(out == expected);
}

TEST_CASE("String_Pair")
{
	auto x = String_Pair();
	REQUIRE(x.str() == "");
	REQUIRE(x.idx() == 0);
	REQUIRE(x.first() == "");
	REQUIRE(x.second() == "");

	x.first("123qwe");
	REQUIRE(x.str() == "123qwe");
	REQUIRE(x.idx() == 6);
	REQUIRE(x.first() == "123qwe");
	REQUIRE(x.second() == "");

	x.second("456z");
	REQUIRE(x.str() == "123qwe456z");
	REQUIRE(x.idx() == 6);
	REQUIRE(x.first() == "123qwe");
	REQUIRE(x.second() == "456z");

	x = String_Pair("6789"s, "zxcvbnm"s);
	REQUIRE(x.str() == "6789zxcvbnm");
	REQUIRE(x.idx() == 4);
	REQUIRE(x.first() == "6789");
	REQUIRE(x.second() == "zxcvbnm");

	x = String_Pair("6789zxcvbnm", 4);
	REQUIRE(x.str() == "6789zxcvbnm");
	REQUIRE(x.idx() == 4);
	REQUIRE(x.first() == "6789");
	REQUIRE(x.second() == "zxcvbnm");

	REQUIRE_NOTHROW(String_Pair("6789", 4));
	REQUIRE_THROWS_AS(String_Pair("6789", 5), std::out_of_range);
	REQUIRE_THROWS_WITH(String_Pair("6789", 5), "word split is too long");
}

TEST_CASE("match_simple_regex()")
{
	REQUIRE(match_simple_regex("abdff"s, "abc?de*ff"s));
	REQUIRE(match_simple_regex("abcdff"s, "abc?de*ff"s));
	REQUIRE(match_simple_regex("abdeeff"s, "abc?de*ff"s));
	REQUIRE(match_simple_regex("abcdeff"s, "abc?de*ff"s));
	REQUIRE_FALSE(match_simple_regex("abcdeeeefff"s, "abc?de*ff"s));
	REQUIRE_FALSE(match_simple_regex("abccdeeeeff"s, "abc?de*ff"s));
	REQUIRE_FALSE(match_simple_regex("qwerty"s, "abc?de*ff"s));
}

TEST_CASE("Similarity_Group")
{
	auto sg = Similarity_Group("abc(AB)БШП(ghgh)");
	REQUIRE(sg.chars == "abcБШП");
	REQUIRE(sg.strings == vector{"AB"s, "ghgh"s});
}

TEST_CASE("utf32_to_utf8()")
{
	REQUIRE(utf32_to_utf8(U"") == "");
	REQUIRE(utf32_to_utf8(U"abcАбвг\uABCD\u1234\U0010ABCD") ==
	        "abcАбвг\uABCD\u1234\U0010ABCD");
}

TEST_CASE("is_all_ascii()")
{
	REQUIRE(is_all_ascii(""));
	REQUIRE(is_all_ascii("abcd\x7f"));
	REQUIRE_FALSE(is_all_ascii("abcd\x80"));
	REQUIRE_FALSE(is_all_ascii("abcd\xFF"));
}

TEST_CASE("latin1_to_ucs2()")
{
	REQUIRE(latin1_to_ucs2("abcd\x7F\x80\xFF") ==
	        u"abcd\u007F\u0080\u00FF");
}

TEST_CASE("is_all_bmp()")
{
	REQUIRE(is_all_bmp(u"abc\u00FF\uFFFF"));
	REQUIRE_FALSE(is_all_bmp(u"abc\u00FF\uFFFF\U00010000"));
	REQUIRE_FALSE(is_all_bmp(u"abc\U0010FFFF\u00FF\uFFFF"));
}

TEST_CASE("case conversion")
{
	// These are simple tests that only test if we wrap properly.
	// There is no need for extensive testing, ICU is well tested.
	auto in = "grüßEN";
	auto l = icu::Locale();
	CHECK(to_lower(in, l) == "grüßen");
	CHECK(to_upper(in, l) == "GRÜSSEN");
	CHECK(to_title(in, l) == "Grüßen");

	in = "isTAnbulI";
	CHECK(to_lower(in, l) == "istanbuli");
	CHECK(to_upper(in, l) == "ISTANBULI");
	CHECK(to_title(in, l) == "Istanbuli");

	l = {"tr_TR"};
	CHECK(to_lower(in, l) == "istanbulı");
	CHECK(to_upper(in, l) == "İSTANBULI");
	CHECK(to_title(in, l) == "İstanbulı");
}

TEST_CASE("classify_casing()")
{
	REQUIRE(classify_casing("") == Casing::SMALL);
	REQUIRE(classify_casing("здраво") == Casing::SMALL);
	REQUIRE(classify_casing("Здраво") == Casing::INIT_CAPITAL);
	REQUIRE(classify_casing("ЗДРАВО") == Casing::ALL_CAPITAL);
	REQUIRE(classify_casing("здРаВо") == Casing::CAMEL);
	REQUIRE(classify_casing("ЗдрАво") == Casing::PASCAL);
}

TEST_CASE("is_number()")
{
	REQUIRE_FALSE(is_number(""));
	REQUIRE(is_number("1234567890"));
	REQUIRE(is_number("-1234567890"));
	REQUIRE(is_number("123.456.78-9,0"));
	REQUIRE(is_number("-123.456.78-9,0"));
	REQUIRE_FALSE(is_number("123..456.78-9,0"));
	REQUIRE_FALSE(is_number("123.456.-78-9,0"));
	REQUIRE_FALSE(is_number("123..456.78-9-,0"));
}

TEST_CASE("Dict_Base::forgotten_char_suggest()")
{
	auto d = nuspell::Suggester();
	d.words.emplace("Забвгд", u"");
	d.words.emplace("абвШгд", u"");
	d.words.emplace("абвгдИ", u"");
	d.words.emplace("абвгдК", u"");
	d.try_chars = "шизШИЗ";
	auto in = "абвгд"s;
	auto sugs = vector<string>();
	d.forgotten_char_suggest(in, sugs);
	REQUIRE(sugs == vector{"абвШгд"s, "абвгдИ"s, "Забвгд"s});
}
