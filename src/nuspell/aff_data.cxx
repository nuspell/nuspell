/* Copyright 2016-2019 Dimitrij Mijoski
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

#include "aff_data.hxx"
#include "locale_utils.hxx"
#include "string_utils.hxx"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <boost/algorithm/string/case_conv.hpp>

/*
 * Aff_Data class and the method parse() should be structured in the following
 * way. The data members of the class should be data structures that are
 * useful for reading and searching operations, but are not necessarely
 * good for dynamic updates. They should be designed toward their later use
 * while spellchecking.
 *
 * On the other hand, the method parse() should fill the data in intermediate
 * data structures that are good for dynamic updates.
 *
 * E.g. In the class: it can store the affixes in a sorted vector by the key
 * on which will be searched, the adding affix. (aka flat_map)
 *
 * In method parse(): it will fill the affixes in simple unsorted vector.
 *
 * For simple stiff like simple int, flag, or for data structures that are
 * equally good for updating and reading, just parse directly into the class
 * data members.
 *
 * class Aff_data {
 *	Affix_Table affix_table;     // wraps a vector and keeps invariant
 *	                                that the vector is always sorted
 *	Substring_Replace_Table rep_table;
 * };
 *
 * parse(istream in) {
 *	vector<aff_entry> affix_table_intermediate;
 *	vector<rep_entry> rep_table_intermediate;
 *	while (getline) {
 *	...
 *		affix_table_intermediate.push_back(entry)
 *	...
 *	}
 *	At the end
 *	affix_table = move(affix_table_intermediate);
 * }
 */

/**
 * @brief Library main namespace
 */
namespace nuspell {

using namespace std;

auto Encoding::normalize_name() -> void
{
	boost::algorithm::to_upper(name, locale::classic());
	if (name == "UTF8")
		name = "UTF-8";
	else if (name.compare(0, 10, "MICROSOFT-") == 0)
		name.erase(0, 10);
}

namespace {

void reset_failbit_istream(std::istream& in)
{
	in.clear(in.rdstate() & ~in.failbit);
}

enum class Parsing_Error_Code {
	NO_FLAGS_AFTER_SLASH_WARNING = -2,
	NONUTF8_FLAGS_ABOVE_127_WARNING = -1,
	NO_ERROR = 0,
	MISSING_FLAGS,
	UNPAIRED_LONG_FLAG,
	INVALID_NUMERIC_FLAG,
	// FLAGS_ARE_UTF8_BUT_FILE_NOT,
	INVALID_UTF8,
	FLAG_ABOVE_65535,
	INVALID_NUMERIC_ALIAS,
	AFX_CONDITION_INVALID_FORMAT,
	COMPOUND_RULE_INVALID_FORMAT
};

auto decode_flags(const string& s, Flag_Type t, const Encoding& enc,
                  u16string& out) -> Parsing_Error_Code
{
	using Err = Parsing_Error_Code;
	using Ft = Flag_Type;
	auto warn = Err();
	out.clear();
	if (s.empty())
		return Err::MISSING_FLAGS;
	switch (t) {
	case Ft::SINGLE_CHAR:
		if (enc.is_utf8() && !is_all_ascii(s)) {
			warn = Err::NONUTF8_FLAGS_ABOVE_127_WARNING;
			// This warning will be triggered in Hungarian.
			// Version 1 passed this, it just read a single byte
			// even if the stream utf-8. Hungarian dictionary
			// exploited this bug/feature, resulting it's file to be
			// mixed utf-8 and latin2. In v2 this will eventually
			// work, with a warning.
		}
		latin1_to_ucs2(s, out);
		break;
	case Ft::DOUBLE_CHAR: {
		if (enc.is_utf8() && !is_all_ascii(s))
			warn = Err::NONUTF8_FLAGS_ABOVE_127_WARNING;

		if (s.size() % 2 == 1)
			return Err::UNPAIRED_LONG_FLAG;

		auto i = s.begin();
		auto e = s.end();
		for (; i != e; i += 2) {
			auto c1 = *i;
			auto c2 = *(i + 1);
			out.push_back((c1 << 8) | c2);
		}
		break;
	}
	case Ft::NUMBER: {
		auto p = s.c_str();
		char* p2 = nullptr;
		errno = 0;
		for (;;) {
			auto flag = strtoul(p, &p2, 10);
			if (p2 == p)
				return Err::INVALID_NUMERIC_FLAG;
			if (flag == numeric_limits<decltype(flag)>::max() &&
			    errno == ERANGE) {
				errno = 0;
				return Err::FLAG_ABOVE_65535;
			}
			if (flag > 0xFFFF)
				return Err::FLAG_ABOVE_65535;
			out.push_back(flag);

			if (p2 == &s[s.size()] || *p2 != ',')
				break;

			p = p2 + 1;
		}
		break;
	}
	case Ft::UTF8: {
		// if (!enc.is_utf8())
		//	return Err::FLAGS_ARE_UTF8_BUT_FILE_NOT;

		auto ok = utf8_to_16(s, out);
		if (!ok) {
			out.clear();
			return Err::INVALID_UTF8;
		}

		if (!is_all_bmp(out)) {
			out.clear();
			return Err::FLAG_ABOVE_65535;
		}
		break;
	}
	}
	return warn;
}

auto decode_flags_possible_alias(const string& s, Flag_Type t,
                                 const Encoding& enc,
                                 const vector<Flag_Set>& flag_aliases,
                                 u16string& out) -> Parsing_Error_Code
{
	if (flag_aliases.empty())
		return decode_flags(s, t, enc, out);

	char* p;
	errno = 0;
	out.clear();
	auto i = strtoul(s.c_str(), &p, 10);
	if (p == s.c_str())
		return Parsing_Error_Code::INVALID_NUMERIC_ALIAS;

	if (i == numeric_limits<decltype(i)>::max() && errno == ERANGE)
		return Parsing_Error_Code::INVALID_NUMERIC_ALIAS;

	if (0 < i && i <= flag_aliases.size()) {
		out = flag_aliases[i - 1];
		return {};
	}
	return Parsing_Error_Code::INVALID_NUMERIC_ALIAS;
}

auto report_parsing_error(Parsing_Error_Code err, size_t line_num)
{
	using Err = Parsing_Error_Code;
	switch (err) {
	case Err::NO_FLAGS_AFTER_SLASH_WARNING:
		cerr << "Nuspell warning: no flags after slash in line "
		     << line_num << '\n';
		break;
	case Err::NONUTF8_FLAGS_ABOVE_127_WARNING:
		cerr << "Nuspell warning: bytes above 127 in flags in UTF-8 "
		        "file are treated as lone bytes for backward "
		        "compatibility. That means if in the flags you have "
		        "ONE character above ASCII, it may be interpreted as "
		        "2, 3, or 4 flags. Please update dictionary and affix "
		        "files to use FLAG UTF-8 and make the file valid "
		        "UTF-8 if it is not already. Warning in line "
		     << line_num << '\n';
		break;
	case Err::NO_ERROR:
		break;
	case Err::MISSING_FLAGS:
		cerr << "Nuspell error: missing flags in line " << line_num
		     << '\n';
		break;
	case Err::UNPAIRED_LONG_FLAG:
		cerr << "Nuspell error: the number of chars in string of long "
		        "flags is odd, should be even. Error in line "
		     << line_num << '\n';
		break;
	case Err::INVALID_NUMERIC_FLAG:
		cerr << "Nuspell error: invalid numerical flag in line"
		     << line_num << '\n';
		break;
	// case Err::FLAGS_ARE_UTF8_BUT_FILE_NOT:
	//	cerr << "Nuspell error: flags are UTF-8 but file is not\n";
	//	break;
	case Err::INVALID_UTF8:
		cerr << "Nuspell error: Invalid UTF-8 in flags in line "
		     << line_num << '\n';
		break;
	case Err::FLAG_ABOVE_65535:
		cerr << "Nuspell error: Flag above 65535 in line " << line_num
		     << '\n';
		break;
	case Err::INVALID_NUMERIC_ALIAS:
		cerr << "Nuspell error: Flag alias is invalid in line"
		     << line_num << '\n';
		break;
	case Err::AFX_CONDITION_INVALID_FORMAT:
		cerr << "Nuspell error: Affix condition is invalid in line "
		     << line_num << '\n';
		break;
	case Err::COMPOUND_RULE_INVALID_FORMAT:
		cerr << "Nuspell error: Compound rule is in invalid format in "
		        "line "
		     << line_num << '\n';
		break;
	}
}

auto decode_compound_rule(const string& s, Flag_Type t, const Encoding& enc,
                          u16string& out) -> Parsing_Error_Code
{
	using Ft = Flag_Type;
	using Err = Parsing_Error_Code;
	switch (t) {
	case Ft::SINGLE_CHAR:
	case Ft::UTF8:
		return decode_flags(s, t, enc, out);
		break;
	case Ft::DOUBLE_CHAR:
		out.clear();
		if (s.empty())
			return Err::MISSING_FLAGS;
		for (size_t i = 0;;) {
			if (s.size() - i < 4)
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			if (s[i] != '(' || s[i + 3] != ')')
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			auto c1 = s[i + 1];
			auto c2 = s[i + 2];
			out.push_back((c1 << 8) | c2);
			i += 4;
			if (i == s.size())
				break;
			if (s[i] == '?' || s[i] == '*') {
				out.push_back(s[i]);
				i += 1;
			}
		}
		break;
	case Ft::NUMBER:
		out.clear();
		if (s.empty())
			return Err::MISSING_FLAGS;
		errno = 0;
		for (auto p = s.c_str(); *p != 0;) {
			if (*p != '(')
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			++p;
			char* p2;
			auto flag = strtoul(p, &p2, 10);
			if (p2 == p)
				return Err::INVALID_NUMERIC_FLAG;
			if (flag == numeric_limits<decltype(flag)>::max() &&
			    errno == ERANGE) {
				errno = 0;
				return Err::FLAG_ABOVE_65535;
			}
			if (flag > 0xFFFF)
				return Err::FLAG_ABOVE_65535;
			p = p2;
			if (*p != ')')
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			out.push_back(flag);
			++p;
			if (*p == '?' || *p == '*') {
				out.push_back(*p);
				++p;
			}
		}
		break;
	}
	return {};
}

auto strip_utf8_bom(std::istream& in) -> void
{
	if (!in.good())
		return;
	auto bom = string(3, '\0');
	in.read(&bom[0], 3);
	if (in && bom == "\xEF\xBB\xBF")
		return;
	if (in.bad())
		return;
	reset_failbit_istream(in);
	for (auto i = size_t(in.gcount()); i-- != 0;)
		in.putback(bom[i]);
}

struct Compound_Rule_Ref_Wrapper {
	std::u16string& rule;
};

auto wrap_compound_rule(std::u16string& r) -> Compound_Rule_Ref_Wrapper
{
	return {r};
}

class Aff_Line_Stream : public std::istringstream {
	std::string str_buf;
	std::u16string flag_buffer;

	const Aff_Data* aff_data = nullptr;
	Encoding_Converter cvt;

	auto virtual dummy_func() -> void;

      public:
	Parsing_Error_Code err = {};

	auto set_aff_data(Aff_Data& a)
	{
		aff_data = &a;
		cvt = Encoding_Converter(a.encoding.value_or_default());
	}

	Aff_Line_Stream() = default;
	Aff_Line_Stream(Aff_Line_Stream&& other)
	    : istringstream{std::move(other)}, aff_data{other.aff_data},
	      cvt{std::move(other.cvt)}, err{other.err}
	{
		other.aff_data = nullptr;
		other.err = {};
	}

	auto& parse(Encoding& enc)
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		enc = str_buf;
		cvt = Encoding_Converter(enc.value_or_default());
		if (!cvt.valid()) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Invalif setting for encoding");
		}
		return in;
	}

	auto& parse(std::wstring& wstr)
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail()) // str_buf is unmodified on fail
			return in;
		auto ok = cvt.to_wide(str_buf, wstr);
		if (!ok) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("encoding conversion error");
		}
		return in;
	}

	auto& parse(Flag_Type& flag_type)
	{
		using Ft = Flag_Type;
		auto& in = *this;
		flag_type = {};
		in >> str_buf;
		if (in.fail())
			return in;
		boost::algorithm::to_upper(str_buf, in.getloc());
		if (str_buf == "LONG")
			flag_type = Ft::DOUBLE_CHAR;
		else if (str_buf == "NUM")
			flag_type = Ft::NUMBER;
		else if (str_buf == "UTF-8")
			flag_type = Ft::UTF8;
		else {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("unknown FLAG type");
		}
		return in;
	}

	auto& parse(icu::Locale& loc)
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		loc = icu::Locale(str_buf.c_str());
		if (loc.isBogus()) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("unknown Language in LANG");
		}
		return in;
	}

	auto& parse(std::u16string& flags)
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		err = decode_flags(str_buf, aff_data->flag_type,
		                   aff_data->encoding, flags);
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Flag parsing error");
		}
		return in;
	}

	auto& parse(char16_t& flag)
	{
		auto& in = *this;
		flag = 0;
		parse(flag_buffer);
		if (in)
			flag = flag_buffer[0];
		return in;
	}

	auto& parse(Flag_Set& flags)
	{
		auto& in = *this;
		parse(flag_buffer);
		if (in)
			flags = flag_buffer;
		return in;
	}

	auto& parse_word_slash_flags(wstring& word, Flag_Set& flags)
	{
		using Err = Parsing_Error_Code;
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		// err = {};
		auto slash_pos = str_buf.find('/');
		if (slash_pos != str_buf.npos) {
			auto flag_str =
			    str_buf.substr(slash_pos + 1); // temporary
			str_buf.erase(slash_pos);
			err = decode_flags_possible_alias(
			    flag_str, aff_data->flag_type, aff_data->encoding,
			    aff_data->flag_aliases, flag_buffer);
			if (err == Err::MISSING_FLAGS)
				err = Err::NO_FLAGS_AFTER_SLASH_WARNING;
			flags = flag_buffer;
		}
		auto ok = cvt.to_wide(str_buf, word);
		if (!ok) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Encoding error");
		}
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Flag parsing error");
		}
		return in;
	}

	auto& parse(tuple<wstring&, Flag_Set&> word_and_flags)
	{
		return parse_word_slash_flags(std::get<0>(word_and_flags),
		                              std::get<1>(word_and_flags));
	}

	auto& parse(Condition<wchar_t>& cond)
	{
		auto& in = *this;
		auto wstr = wstring();
		in.parse(wstr);
		if (in.fail())
			return in;
		try {
			cond = std::move(wstr);
		}
		catch (const Condition_Exception& ex) {
			err = Parsing_Error_Code::AFX_CONDITION_INVALID_FORMAT;
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Condition error, " +
				              string(ex.what()));
		}
		return in;
	}

	auto parse_word_slash_single_flag(wstring& word, char16_t& flag)
	    -> istream&
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		// err = {};
		auto slash_pos = str_buf.find('/');
		if (slash_pos != str_buf.npos) {
			auto flag_str =
			    str_buf.substr(slash_pos + 1); // temporary
			str_buf.erase(slash_pos);
			err = decode_flags(flag_str, aff_data->flag_type,
			                   aff_data->encoding, flag_buffer);
			if (!flag_buffer.empty())
				flag = flag_buffer[0];
		}
		auto ok = cvt.to_wide(str_buf, word);
		if (!ok) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Encoding error");
		}
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Flag parsing error");
		}
		return in;
	}

	auto& parse(tuple<wstring&, char16_t&> word_and_flag)
	{
		return parse_word_slash_single_flag(std::get<0>(word_and_flag),
		                                    std::get<1>(word_and_flag));
	}

	auto& parse_compound_rule(u16string& out)
	{
		auto& in = *this;
		in >> str_buf;
		if (in.fail())
			return in;
		err = decode_compound_rule(str_buf, aff_data->flag_type,
		                           aff_data->encoding, out);
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw failure("Compound rule parsing error");
		}
		return in;
	}

	auto& parse(Compound_Rule_Ref_Wrapper rule)
	{
		return parse_compound_rule(rule.rule);
	}

#if 0
	auto& parse_morhological_fields(vector<string>& out)
	{
		auto& in = *this;
		if (in.fail())
			return in;
		out.clear();
		auto old_mask = in.exceptions();
		in.exceptions(in.goodbit); // disable exceptions
		while (in >> str_buf) {
			out.push_back(str_buf);
		}
		if (in.fail() && !in.bad())
			reset_failbit_istream(in);
		in.exceptions(old_mask);
		return in;
	}

	auto& parse(vector<string>& out)
	{
		return parse_morhological_fields(out);
	}
#endif
};
auto Aff_Line_Stream::dummy_func() -> void {}

template <class T, class = decltype(std::declval<Aff_Line_Stream>().parse(
                       std::declval<T>()))>
auto& operator>>(Aff_Line_Stream& in, T&& x)
{
	return in.parse(std::forward<T>(x));
}

auto& operator>>(Aff_Line_Stream& in, pair<wstring, wstring>& out)
{
	return in >> out.first >> out.second;
}

auto& operator>>(Aff_Line_Stream& in, Compound_Pattern<wchar_t>& p)
{
	auto first_word_end = wstring();
	auto second_word_begin = wstring();
	p.match_first_only_unaffixed_or_zero_affixed = false;
	in >> std::tie(first_word_end, p.first_word_flag);
	in >> std::tie(second_word_begin, p.second_word_flag);
	if (in.fail())
		return in;
	if (first_word_end == L"0") {
		first_word_end.clear();
		p.match_first_only_unaffixed_or_zero_affixed = true;
	}
	p.begin_end_chars = {first_word_end, second_word_begin};
	auto old_mask = in.exceptions();
	in.exceptions(in.goodbit); // disable exceptions
	in >> p.replacement;       // optional
	if (in.fail() && in.eof() && !in.bad()) {
		reset_failbit_istream(in);
		p.replacement.clear();
	}
	in.exceptions(old_mask);
	return in;
}

template <class T, class Func = identity>
auto parse_vector_of_T(Aff_Line_Stream& in, const string& command,
                       unordered_map<string, size_t>& counts, vector<T>& vec,
                       Func modifier_wrapper = Func()) -> void
{
	auto dat = counts.find(command);
	if (dat == counts.end()) {
		// first line
		auto& cnt = counts[command]; // cnt == 0
		size_t a;
		in >> a;
		if (in)
			cnt = a;
		else
			cerr << "Nuspell error: a vector command (series of "
			        "of similar commands) has no count. Ignoring "
			        "all of them.\n";
	}
	else if (dat->second != 0) {
		dat->second--;
		vec.emplace_back();
		in >> modifier_wrapper(vec.back());
		if (in.fail())
			cerr << "Nuspell error: single entry of a vector "
			        "command (series of "
			        "of similar commands) is invalid.\n";
	}
	else {
		cerr << "Nuspell warning: extra entries of " << command << "\n";
		// cerr << "Nuspell warning in line " << line_num << endl;
	}
}

template <class AffixT>
auto parse_affix(Aff_Line_Stream& in, string& command, vector<AffixT>& vec,
                 unordered_map<string, pair<bool, size_t>>& cmd_affix) -> void
{
	char16_t f;
	in >> f;
	if (in.fail())
		return;
	command.append(reinterpret_cast<char*>(&f), sizeof(f));
	auto dat = cmd_affix.find(command);
	// note: the current affix parser does not allow the same flag
	// to be used once with cross product and again witohut
	// one flag is tied to one cross product value
	if (dat == cmd_affix.end()) {
		char cross_char; // 'Y' or 'N'
		size_t cnt;
		auto& cross_and_cnt = cmd_affix[command]; // == false, 0
		in >> cross_char >> cnt;
		if (in.fail())
			return;
		if (cross_char != 'Y' && cross_char != 'N') {
			in.setstate(in.failbit);
			if (in.exceptions() & in.failbit)
				throw istream::failure("Cross char invalid");
			return;
		}
		bool cross = cross_char == 'Y';
		cross_and_cnt = {cross, cnt};
	}
	else if (dat->second.second) {
		dat->second.second--;
		vec.emplace_back();
		auto& elem = vec.back();
		elem.flag = f;
		elem.cross_product = dat->second.first;
		in >> elem.stripping;
		if (elem.stripping == L"0")
			elem.stripping.clear();
		in >> std::tie(elem.appending, elem.cont_flags);
		if (elem.appending == L"0")
			elem.appending.clear();
		if (in.fail())
			return;
		auto old_mask = in.exceptions();
		in.exceptions(in.goodbit);
		in >> elem.condition; // optional
		if (in.fail() && in.eof() && !in.bad()) {
			elem.condition = L".";
			reset_failbit_istream(in);
		}
		in.exceptions(old_mask);

		// in >> elem.morphological_fields;
	}
	else {
		cerr << "Nuspell warning: extra entries of "
		     << command.substr(0, 3) << "\n";
	}
}

} // namespace

/**
 * Parses an input stream offering affix information.
 *
 * @param in input stream to parse from.
 * @return true on success.
 */
auto Aff_Data::parse_aff(istream& in) -> bool
{
	auto prefixes = vector<Prefix<wchar_t>>();
	auto suffixes = vector<Suffix<wchar_t>>();
	auto break_patterns = vector<wstring>();
	auto break_exists = false;
	auto input_conversion = vector<pair<wstring, wstring>>();
	auto output_conversion = vector<pair<wstring, wstring>>();
	// auto morphological_aliases = vector<vector<string>>();
	auto rules = vector<u16string>();
	auto replacements = vector<pair<wstring, wstring>>();
	auto map_related_chars = vector<wstring>();
	auto phonetic_replacements = vector<pair<wstring, wstring>>();

	flag_type = Flag_Type::SINGLE_CHAR;

	unordered_map<string, wstring*> command_wstrings = {
	    {"IGNORE", &ignored_chars},

	    {"KEY", &keyboard_closeness},
	    {"TRY", &this->try_chars}};

	unordered_map<string, bool*> command_bools = {
	    {"COMPLEXPREFIXES", &complex_prefixes},

	    {"ONLYMAXDIFF", &only_max_diff},
	    {"NOSPLITSUGS", &no_split_suggestions},
	    {"SUGSWITHDOTS", &suggest_with_dots},
	    {"FORBIDWARN", &forbid_warn},

	    {"COMPOUNDMORESUFFIXES", &compound_more_suffixes},
	    {"CHECKCOMPOUNDDUP", &compound_check_duplicate},
	    {"CHECKCOMPOUNDREP", &compound_check_rep},
	    {"CHECKCOMPOUNDCASE", &compound_check_case},
	    {"CHECKCOMPOUNDTRIPLE", &compound_check_triple},
	    {"SIMPLIFIEDTRIPLE", &compound_simplified_triple},

	    {"FULLSTRIP", &fullstrip},
	    {"CHECKSHARPS", &checksharps}};

	unordered_map<string, unsigned short*> command_shorts = {
	    {"MAXCPDSUGS", &max_compound_suggestions},
	    {"MAXNGRAMSUGS", &max_ngram_suggestions},
	    {"MAXDIFF", &max_diff_factor},

	    {"COMPOUNDMIN", &compound_min_length},
	    {"COMPOUNDWORDMAX", &compound_max_word_count}};

	unordered_map<string, vector<pair<wstring, wstring>>*>
	    command_vec_pair = {{"REP", &replacements},
	                        {"PHONE", &phonetic_replacements},
	                        {"ICONV", &input_conversion},
	                        {"OCONV", &output_conversion}};

	unordered_map<string, char16_t*> command_flag = {
	    {"NOSUGGEST", &nosuggest_flag},
	    {"WARN", &warn_flag},

	    {"COMPOUNDFLAG", &compound_flag},
	    {"COMPOUNDBEGIN", &compound_begin_flag},
	    {"COMPOUNDEND", &compound_last_flag},
	    {"COMPOUNDMIDDLE", &compound_middle_flag},
	    {"ONLYINCOMPOUND", &compound_onlyin_flag},
	    {"COMPOUNDPERMITFLAG", &compound_permit_flag},
	    {"COMPOUNDFORBIDFLAG", &compound_forbid_flag},
	    {"COMPOUNDROOT", &compound_root_flag},
	    {"FORCEUCASE", &compound_force_uppercase},

	    {"CIRCUMFIX", &circumfix_flag},
	    {"FORBIDDENWORD", &forbiddenword_flag},
	    {"KEEPCASE", &keepcase_flag},
	    {"NEEDAFFIX", &need_affix_flag},
	    {"SUBSTANDARD", &substandard_flag}};

	// keeps count for each array-command
	auto cmd_with_vec_cnt = unordered_map<string, size_t>();
	auto cmd_affix = unordered_map<string, pair<bool, size_t>>();
	auto line = string();
	auto command = string();
	auto line_num = size_t(0);
	auto ss = Aff_Line_Stream();
	Setlocale_To_C_In_Scope setlocale_to_C;
	auto error_happened = false;
	// while parsing, the streams must have plain ascii locale without
	// any special number separator otherwise istream >> int might fail
	// due to thousands separator.
	// "C" locale can be used assuming it is US-ASCII
	in.imbue(locale::classic());
	ss.imbue(locale::classic());
	ss.set_aff_data(*this);
	strip_utf8_bom(in);
	while (getline(in, line)) {
		line_num++;

		if (encoding.is_utf8() && !validate_utf8(line)) {
			cerr << "Nuspell warning: invalid utf in aff file"
			     << endl;
			// Hungarian will triger this, contains mixed
			// utf-8 and latin2. See note in decode_flags().
		}

		ss.str(line);
		ss.clear();
		ss.err = {};
		ss >> ws;
		if (ss.eof() || ss.peek() == '#') {
			continue; // skip comment or empty lines
		}
		ss >> command;
		boost::algorithm::to_upper(command, ss.getloc());
		if (command == "SFX") {
			parse_affix(ss, command, suffixes, cmd_affix);
		}
		else if (command == "PFX") {
			parse_affix(ss, command, prefixes, cmd_affix);
		}
		else if (command_wstrings.count(command)) {
			auto& str = *command_wstrings[command];
			if (str.empty())
				ss >> str;
			else
				cerr << "Nuspell warning: "
				        "setting "
				     << command << " more than once, ignoring\n"
				     << "Nuspell warning in line " << line_num
				     << endl;
		}
		else if (command_bools.count(command)) {
			*command_bools[command] = true;
		}
		else if (command_shorts.count(command)) {
			auto ptr = command_shorts[command];
			ss >> *ptr;
			if (ptr == &compound_min_length && *ptr == 0) {
				compound_min_length = 1;
			}
		}
		else if (command_flag.count(command)) {
			ss >> *command_flag[command];
		}
		else if (command == "MAP") {
			parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			                  map_related_chars);
		}
		else if (command_vec_pair.count(command)) {
			auto& vec = *command_vec_pair[command];
			parse_vector_of_T(ss, command, cmd_with_vec_cnt, vec);
		}
		else if (command == "SET") {
			if (encoding.empty()) {
				ss >> encoding;
			}
			else {
				cerr << "Nuspell warning: "
				        "setting "
				     << command << " more than once, ignoring\n"
				     << "Nuspell warning in line " << line_num
				     << endl;
			}
		}
		else if (command == "FLAG") {
			ss >> flag_type;
		}
		else if (command == "LANG") {
			ss >> icu_locale;
		}
		else if (command == "AF") {
			parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			                  flag_aliases);
		}
		else if (command == "AM") {
			// parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			//                  morphological_aliases);
		}
		else if (command == "BREAK") {
			parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			                  break_patterns);
			break_exists = true;
		}
		else if (command == "CHECKCOMPOUNDPATTERN") {
			parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			                  compound_patterns);
		}
		else if (command == "COMPOUNDRULE") {
			parse_vector_of_T(ss, command, cmd_with_vec_cnt, rules,
			                  wrap_compound_rule);
		}
		else if (command == "COMPOUNDSYLLABLE") {
			ss >> compound_syllable_max >> compound_syllable_vowels;
		}
		else if (command == "SYLLABLENUM") {
			ss >> compound_syllable_num;
		}
		else if (command == "WORDCHARS") {
			ss >> wordchars;
		}
		if (ss.fail()) {
			error_happened = true;
			cerr
			    << "Nuspell error: could not parse affix file line "
			    << line_num << ": " << line << endl;
			report_parsing_error(ss.err, line_num);
		}
		else if (ss.err != Parsing_Error_Code::NO_ERROR) {
			cerr
			    << "Nuspell warning: while parsing affix file line "
			    << line_num << ": " << line << endl;
			report_parsing_error(ss.err, line_num);
		}
	}
	// default BREAK definition
	if (!break_exists) {
		break_patterns = {L"-", L"^-", L"-$"};
	}
	for (auto& r : replacements) {
		auto& s = r.second;
		replace_char(s, L'_', L' ');
	}

	// now fill data structures from temporary data
	compound_rules = std::move(rules);
	similarities.assign(begin(map_related_chars), end(map_related_chars));
	break_table = std::move(break_patterns);
	input_substr_replacer = std::move(input_conversion);
	output_substr_replacer = std::move(output_conversion);
	this->replacements = std::move(replacements);
	phonetic_table = std::move(phonetic_replacements);
	for (auto& x : prefixes) {
		erase_chars(x.appending, ignored_chars);
	}
	for (auto& x : suffixes) {
		erase_chars(x.appending, ignored_chars);
	}
	this->prefixes = std::move(prefixes);
	this->suffixes = std::move(suffixes);

	cerr.flush();
	return in.eof() && !error_happened; // true for success
}

/**
 * @brief Scans @p line for morphological field [a-z][a-z]:
 * @param line
 *
 * @returns the end of the word before the morph field, or npos
 */
auto dic_find_end_of_word_heuristics(const string& line)
{
	if (line.size() < 4)
		return line.npos;
	size_t a = 0;
	for (;;) {
		a = line.find(' ', a);
		if (a == line.npos)
			break;
		auto b = line.find_first_not_of(' ', a);
		if (b == line.npos)
			break;
		if (b > line.size() - 3)
			break;
		if (line[b] >= 'a' && line[b] <= 'z' && line[b + 1] >= 'a' &&
		    line[b + 1] <= 'z' && line[b + 2] == ':')
			return a;
		a = b;
	}
	return line.npos;
}

/**
 * Parses an input stream offering dictionary information.
 *
 * @param in input stream to read from.
 * @return true on success.
 */
auto Aff_Data::parse_dic(istream& in) -> bool
{
	size_t line_number = 1;
	size_t approximate_size;
	string line;
	string word;
	string flags_str;
	u16string flags;
	wstring wide_word;
	auto enc_conv = Encoding_Converter(encoding.value_or_default());

	// locale must be without thousands separator.
	auto& ctype = use_facet<std::ctype<char>>(locale::classic());
	in.imbue(locale::classic());
	Setlocale_To_C_In_Scope setlocale_to_C;

	strip_utf8_bom(in);
	if (in >> approximate_size)
		words.reserve(approximate_size);
	else
		return false;
	getline(in, line);

	while (getline(in, line)) {
		line_number++;
		word.clear();
		flags_str.clear();
		flags.clear();

		size_t slash_pos = 0;
		size_t tab_pos = 0;
		for (;;) {
			slash_pos = line.find('/', slash_pos);
			if (slash_pos == line.npos)
				break;
			if (slash_pos == 0)
				break;
			if (line[slash_pos - 1] != '\\')
				break;

			line.erase(slash_pos - 1, 1);
		}
		if (slash_pos != line.npos && slash_pos != 0) {
			// slash found, word until slash
			word.assign(line, 0, slash_pos);
			auto ptr = ctype.scan_is(ctype.space, &line[slash_pos],
			                         &line[line.size()]);
			auto end_flags_pos = ptr - &line[0];
			flags_str.assign(line, slash_pos + 1,
			                 end_flags_pos - (slash_pos + 1));
			auto err = decode_flags_possible_alias(
			    flags_str, flag_type, encoding, flag_aliases,
			    flags);
			report_parsing_error(err, line_number);
			if (static_cast<int>(err) > 0)
				continue;
		}
		else if ((tab_pos = line.find('\t')) != line.npos) {
			// Tab found, word until tab. No flags.
			// After tab follow morphological fields
			word.assign(line, 0, tab_pos);
		}
		else {
			auto end = dic_find_end_of_word_heuristics(line);
			word.assign(line, 0, end);
		}
		if (word.empty())
			continue;
		auto ok = enc_conv.to_wide(word, wide_word);
		if (!ok)
			continue;
		erase_chars(wide_word, ignored_chars);
		auto casing = classify_casing(wide_word);
		const char16_t HIDDEN_HOMONYM_FLAG = -1;
		switch (casing) {
		case Casing::ALL_CAPITAL: {
			// check for hidden homonym
			auto hom = words.equal_range_nonconst_unsafe(wide_word);
			auto h =
			    std::find_if(hom.first, hom.second, [&](auto& w) {
				    return w.second.contains(
				        HIDDEN_HOMONYM_FLAG);
			    });

			if (h != hom.second) {
				// replace if found
				h->second = flags;
			}
			else {
				words.emplace(wide_word, flags);
			}
			break;
		}
		case Casing::PASCAL:
		case Casing::CAMEL: {
			words.emplace(wide_word, flags);

			// add the hidden homonym directly in uppercase
			auto up_wide = to_upper(wide_word, icu_locale);
			auto& up = wide_word;
			auto hom = words.equal_range(up);
			auto h = none_of(hom.first, hom.second, [&](auto& w) {
				return w.second.contains(HIDDEN_HOMONYM_FLAG);
			});
			if (h) { // if not found
				flags += HIDDEN_HOMONYM_FLAG;
				words.emplace(up, flags);
			}
			break;
		}
		default:
			words.emplace(wide_word, flags);
			break;
		}
	}
	return in.eof(); // success if we reached eof
}
} // namespace nuspell
