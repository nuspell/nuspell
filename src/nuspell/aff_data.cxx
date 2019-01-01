/* Copyright 2016-2018 Dimitrij Mijoski
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
#include "string_utils.hxx"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <clocale>

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) ||               \
                         (defined(__APPLE__) && defined(__MACH__)))
#include <unistd.h>
#endif

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/locale.hpp>
#include <boost/range/adaptors.hpp>

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

struct Affix {
	char16_t flag;
	bool cross_product;
	std::string stripping;
	std::string appending;
	std::u16string new_flags;
	std::string condition;
	std::vector<std::string> morphological_fields;
};

struct Compound_Check_Pattern {
	std::string first_word_end;
	std::string second_word_begin;
	std::string replacement;
	char16_t first_word_flag;
	char16_t second_word_flag;
};

auto Word_List::equal_range(const std::wstring& word) const
    -> std::pair<Word_List_Base::local_const_iterator,
                 Word_List_Base::local_const_iterator>
{
	auto static thread_local u8buf = string();
	wide_to_utf8(word, u8buf);
	return equal_range(u8buf);
}

void reset_failbit_istream(std::istream& in)
{
	in.clear(in.rdstate() & ~in.failbit);
}

/**
 * Parses vector of class T from an input stream.
 *
 * @param in input stream to decode from.
 * @param line_num
 * @param command
 * @param[in,out] counts
 * @param[in,out] vec
 * @param parseLineFunc
 */
template <class T, class Func>
auto parse_vector_of_T(istream& in, size_t line_num, const string& command,
                       unordered_map<string, int>& counts, vector<T>& vec,
                       Func parseLineFunc) -> void
{
	auto dat = counts.find(command);
	if (dat == counts.end()) {
		// first line
		size_t a;
		in >> a;
		if (in.fail()) {
			a = 0; // err
			cerr << "Nuspell error: a vector command (series of "
			        "of similar commands) has no count. Ignoring "
			        "all of them."
			     << endl;
		}
		counts[command] = a;
	}
	else if (dat->second) {
		vec.emplace_back();
		parseLineFunc(in, vec.back());
		if (in.fail()) {
			vec.pop_back();
			cerr << "Nuspell error: single entry of a vector "
			        "command (series of "
			        "of similar commands) is invalid"
			     << endl;
		}
		dat->second--;
	}
	else {
		cerr << "Nuspell warning: extra entries of " << command << "\n";
		cerr << "Nuspell warning in line " << line_num << endl;
	}
}

enum class Flag_Parsing_Error {
	NONUTF8_FLAGS_ABOVE_127_WARNING = -1,
	NO_ERROR = 0,
	MISSING_FLAGS,
	UNPAIRED_LONG_FLAG,
	INVALID_NUMERIC_FLAG,
	// FLAGS_ARE_UTF8_BUT_FILE_NOT,
	INVALID_UTF8,
	FLAG_ABOVE_65535,
	INVALID_NUMERIC_ALIAS,
	COMPOUND_RULE_INVALID_FORMAT
};

auto decode_flags(const string& s, Flag_Type t, const Encoding& enc,
                  u16string& out) -> Flag_Parsing_Error
{
	using Err = Flag_Parsing_Error;
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
		while (p2 != &s[s.size()] && *p2 == ',') {
			p = p2 + 1;
			flag = strtoul(p, &p2, 10);
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
                                 u16string& out) -> Flag_Parsing_Error
{
	if (flag_aliases.empty())
		return decode_flags(s, t, enc, out);

	char* p;
	errno = 0;
	out.clear();
	auto i = strtoul(s.c_str(), &p, 10);
	if (p == s.c_str())
		return Flag_Parsing_Error::INVALID_NUMERIC_ALIAS;

	if (i == numeric_limits<decltype(i)>::max() && errno == ERANGE)
		return Flag_Parsing_Error::INVALID_NUMERIC_ALIAS;

	if (0 < i && i <= flag_aliases.size()) {
		out = flag_aliases[i - 1];
		return {};
	}
	return Flag_Parsing_Error::INVALID_NUMERIC_ALIAS;
}

auto report_flag_parsing_error(Flag_Parsing_Error err, size_t line_num)
{
	using Err = Flag_Parsing_Error;
	switch (err) {
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
	case Err::COMPOUND_RULE_INVALID_FORMAT:
		cerr << "Nuspell error: Compound rule is in invalid format in "
		        "line "
		     << line_num << '\n';
		break;
	}
}

/**
 * Decodes flags.
 *
 * Expects that there are flags in the stream.
 * If there are no flags in the stream (eg, stream is at eof)
 * or if the format of the flags is incorrect the stream failbit will be set.
 */
auto decode_flags(istream& in, size_t line_num, Flag_Type t,
                  const Encoding& enc, u16string& out) -> istream&
{
	string s;
	in >> s;
	auto err = decode_flags(s, t, enc, out);
	if (static_cast<int>(err) > 0)
		in.setstate(in.failbit);
	report_flag_parsing_error(err, line_num);
	return in;
}

auto decode_flags_possible_alias(istream& in, size_t line_num, Flag_Type t,
                                 const Encoding& enc,
                                 const vector<Flag_Set>& flag_aliases,
                                 u16string& out) -> istream&
{
	string s;
	in >> s;
	auto err = decode_flags_possible_alias(s, t, enc, flag_aliases, out);
	if (static_cast<int>(err) > 0)
		in.setstate(in.failbit);
	report_flag_parsing_error(err, line_num);
	return in;
}

/**
 * Decodes a single flag from an input stream.
 *
 * @param in input stream to decode from.
 * @param line_num
 * @param t
 * @param enc encoding of the stream.
 * @return The value of the first decoded flag or 0 when no flag was decoded.
 */
auto decode_single_flag(istream& in, size_t line_num, Flag_Type t,
                        const Encoding& enc) -> char16_t
{
	auto flags = u16string();
	decode_flags(in, line_num, t, enc, flags);
	if (!flags.empty()) {
		return flags.front();
	}
	return 0;
}

auto parse_word_slash_flags(istream& in, size_t line_num, Flag_Type t,
                            const Encoding& enc,
                            const vector<Flag_Set>& flag_aliases, string& word,
                            u16string& flags) -> istream&
{
	in >> word;
	auto slash_pos = word.find('/');
	if (slash_pos == word.npos) {
		flags.clear();
		return in;
	}

	auto flag_str = word.substr(slash_pos + 1);
	word.erase(slash_pos);
	auto err =
	    decode_flags_possible_alias(flag_str, t, enc, flag_aliases, flags);
	if (static_cast<int>(err) > 0)
		in.setstate(in.failbit);
	report_flag_parsing_error(err, line_num);
	return in;
}

auto parse_word_slash_single_flag(istream& in, size_t line_num, Flag_Type t,
                                  const Encoding& enc, string& word,
                                  char16_t& flag) -> istream&
{
	in >> word;
	auto slash_pos = word.find('/');
	if (slash_pos == word.npos) {
		flag = 0;
		return in;
	}

	auto flags = u16string();
	auto flag_str = word.substr(slash_pos + 1);
	word.erase(slash_pos);
	auto err = decode_flags(flag_str, t, enc, flags);
	if (static_cast<int>(err) > 0)
		in.setstate(in.failbit);
	report_flag_parsing_error(err, line_num);
	if (flags.empty())
		flag = 0;
	else
		flag = flags.front();
	return in;
}

/**
 * Parses morhological fields.
 *
 * @param in input stream to parse from.
 * @param[in,out] vecOut
 */
auto parse_morhological_fields(istream& in, vector<string>& vecOut) -> void
{
	if (!in.good()) {
		return;
	}

	string morph;
	while (in >> morph) {
		vecOut.push_back(morph);
	}
	reset_failbit_istream(in);
}

/**
 * Parses an affix from an input stream.
 *
 * @param in input stream to parse from.
 * @param line_num
 * @param[in,out] command
 * @param t
 * @param enc
 * @param flag_aliases
 * @param[in,out] vec
 * @param[in,out] cmd_affix
 */
auto parse_affix(istream& in, size_t line_num, string& command, Flag_Type t,
                 const Encoding& enc, const vector<Flag_Set>& flag_aliases,
                 vector<Affix>& vec,
                 unordered_map<string, pair<bool, int>>& cmd_affix) -> void
{
	char16_t f = decode_single_flag(in, line_num, t, enc);
	if (f == 0) {
		// err
		return;
	}
	char f1 = f & 0xff;
	char f2 = (f >> 8) & 0xff;
	command.push_back(f1);
	command.push_back(f2);
	auto dat = cmd_affix.find(command);
	// note: the current affix parser does not allow the same flag
	// to be used once with cross product and again witohut
	// one flag is tied to one cross product value
	if (dat == cmd_affix.end()) {
		char cross_char; // 'Y' or 'N'
		size_t cnt;
		in >> cross_char >> cnt;
		bool cross = cross_char == 'Y';
		if (in.fail()) {
			cnt = 0; // err
			cerr << "Nuspell error: a SFX/PFX header command is "
			        "invalid, missing count or cross product in "
			        "line "
			     << line_num << endl;
		}
		cmd_affix[command] = make_pair(cross, cnt);
	}
	else if (dat->second.second) {
		vec.emplace_back();
		auto& elem = vec.back();
		elem.flag = f;
		elem.cross_product = dat->second.first;
		in >> elem.stripping;
		if (elem.stripping == "0")
			elem.stripping = "";
		parse_word_slash_flags(in, line_num, t, enc, flag_aliases,
		                       elem.appending, elem.new_flags);
		if (elem.appending == "0")
			elem.appending = "";
		if (in.fail()) {
			vec.pop_back();
			return;
		}
		in >> elem.condition;
		if (elem.condition.empty())
			elem.condition = '.';
		if (in.fail())
			reset_failbit_istream(in);
		else
			parse_morhological_fields(in,
			                          elem.morphological_fields);
		dat->second.second--;
	}
	else {
		cerr << "Nuspell warning: extra entries of "
		     << command.substr(0, 3) << "\n"
		     << "Nuspell warning in line " << line_num << endl;
	}
}

/**
 * Parses flag type.
 *
 * @param in input stream to parse from.
 * @param line_num
 * @param[out] flag_type
 */
auto parse_flag_type(istream& in, size_t line_num, Flag_Type& flag_type) -> void
{
	using Ft = Flag_Type;
	(void)line_num;
	string p;
	in >> p;
	boost::algorithm::to_upper(p, in.getloc());
	if (p == "LONG")
		flag_type = Ft::DOUBLE_CHAR;
	else if (p == "NUM")
		flag_type = Ft::NUMBER;
	else if (p == "UTF-8")
		flag_type = Ft::UTF8;
	else
		cerr << "Nuspell error: unknown FLAG type" << endl;
}

auto parse_compound_rule(const string& s, Flag_Type t, const Encoding& enc,
                         u16string& out) -> Flag_Parsing_Error
{
	using Ft = Flag_Type;
	using Err = Flag_Parsing_Error;
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
			flag = strtoul(p, &p2, 10);
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

auto parse_compound_rule(istream& in, size_t line_num, Flag_Type t,
                         const Encoding& enc, u16string& out) -> istream&
{
	string s;
	in >> s;
	auto err = parse_compound_rule(s, t, enc, out);
	if (static_cast<int>(err) > 0)
		in.setstate(in.failbit);
	report_flag_parsing_error(err, line_num);
	return in;
}

auto strip_utf8_bom(std::istream& in) -> void
{
	if (!in.good())
		return;
	string bom(3, '\0');
	in.read(&bom[0], 3);
	auto cnt = in.gcount();
	if (cnt == 3 && bom == "\xEF\xBB\xBF")
		return;
	if (cnt < 3)
		reset_failbit_istream(in);
	for (auto i = cnt - 1; i >= 0; --i) {
		in.putback(bom[i]);
	}
}

//#if _POSIX_VERSION >= 200809L
#ifdef _POSIX_VERSION
class Setlocale_To_C_In_Scope {
	locale_t old_loc = nullptr;

      public:
	Setlocale_To_C_In_Scope()
	    : old_loc{uselocale(newlocale(0, "C", nullptr))}
	{
	}
	~Setlocale_To_C_In_Scope()
	{
		auto new_loc = uselocale(old_loc);
		if (new_loc != old_loc)
			freelocale(new_loc);
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#else
class Setlocale_To_C_In_Scope {
	std::string old_name;
#ifdef _WIN32
	int old_per_thread;
#endif
      public:
	Setlocale_To_C_In_Scope() : old_name(setlocale(LC_ALL, nullptr))
	{
#ifdef _WIN32
		old_per_thread = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
#endif
		auto x = setlocale(LC_ALL, "C");
		if (!x)
			old_name.clear();
	}
	~Setlocale_To_C_In_Scope()
	{
#ifdef _WIN32
		_configthreadlocale(old_per_thread);
		if (old_per_thread == _ENABLE_PER_THREAD_LOCALE)
#endif
		{
			if (!old_name.empty())
				setlocale(LC_ALL, old_name.c_str());
		}
	}
	Setlocale_To_C_In_Scope(const Setlocale_To_C_In_Scope&) = delete;
};
#endif

/**
 * @brief Sets the internal encoding, and optionally, language.
 *
 * Sets the encoding of the strings inside the dictionary. This function
 * should not be called manually because it is called by parse_aff(). Should
 * be only called if you are manually filling this object via C++ code,
 * and not from file.
 *
 * @param enc
 * @param lang
 */
auto Aff_Data::set_encoding_and_language(const string& enc, const string& lang)
    -> void
{
	boost::locale::generator locale_generator;
	auto name = lang;
	if (name.empty())
		name = "en_US";
	if (!enc.empty())
		name += '.' + enc;
	internal_locale = locale_generator(name);
	install_ctype_facets_inplace(internal_locale);
}

/**
 * Parses an input stream offering affix information.
 *
 * @param in input stream to parse from.
 * @return true on success.
 */
auto Aff_Data::parse_aff(istream& in) -> bool
{
	Encoding encoding;
	string language_code;
	string ignore_chars;
	string keyboard_layout;
	string try_chars;
	vector<Affix> prefixes;
	vector<Affix> suffixes;
	vector<string> break_patterns;
	bool break_exists = false;
	vector<pair<string, string>> input_conversion;
	vector<pair<string, string>> output_conversion;
	vector<vector<string>> morphological_aliases;
	vector<Compound_Check_Pattern> compound_check_patterns;
	vector<u16string> rules;
	vector<pair<string, string>> replacements;
	vector<string> map_related_chars;
	vector<pair<string, string>> phonetic_replacements;
	auto flags = u16string();

	flag_type = Flag_Type::SINGLE_CHAR;

	unordered_map<string, string*> command_strings = {
	    {"LANG", &language_code},
	    {"IGNORE", &ignore_chars},

	    {"KEY", &keyboard_layout},
	    {"TRY", &try_chars},

	    {"WORDCHARS", &wordchars}};

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

	unordered_map<string, vector<string>*> command_vec_str = {
	    {"MAP", &map_related_chars}};

	unordered_map<string, unsigned short*> command_shorts = {
	    {"MAXCPDSUGS", &max_compound_suggestions},
	    {"MAXNGRAMSUGS", &max_ngram_suggestions},
	    {"MAXDIFF", &max_diff_factor},

	    {"COMPOUNDMIN", &compound_min_length},
	    {"COMPOUNDWORDMAX", &compound_max_word_count}};

	unordered_map<string, vector<pair<string, string>>*> command_vec_pair =
	    {{"REP", &replacements},
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

	// keeps count for each vector
	auto cmd_with_vec_cnt = unordered_map<string, int>();
	auto cmd_affix = unordered_map<string, pair<bool, int>>();
	auto line = string();
	auto command = string();
	size_t line_num = 0;
	auto ss = istringstream();
	auto loc = locale::classic();
	Setlocale_To_C_In_Scope setlocale_to_C;
	// while parsing, the streams must have plain ascii locale without
	// any special number separator otherwise istream >> int might fail
	// due to thousands separator.
	// "C" locale can be used assuming it is US-ASCII
	in.imbue(loc);
	ss.imbue(loc);
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
		ss >> ws;
		if (ss.eof() || ss.peek() == '#') {
			continue; // skip comment or empty lines
		}
		ss >> command;
		boost::algorithm::to_upper(command, ss.getloc());
		ss >> ws;
		if (command == "PFX" || command == "SFX") {
			auto& vec = command[0] == 'P' ? prefixes : suffixes;
			parse_affix(ss, line_num, command, flag_type, encoding,
			            flag_aliases, vec, cmd_affix);
		}
		else if (command_strings.count(command)) {
			auto& str = *command_strings[command];
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
			*command_flag[command] = decode_single_flag(
			    ss, line_num, flag_type, encoding);
		}
		else if (command_vec_str.count(command)) {
			auto& vec = *command_vec_str[command];
			auto func = [&](istream& in, string& p) { in >> p; };
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command_vec_pair.count(command)) {
			auto& vec = *command_vec_pair[command];
			auto func = [&](istream& in, pair<string, string>& p) {
				in >> p.first >> p.second;
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "SET") {
			if (encoding.empty()) {
				auto str = string();
				ss >> str;
				encoding = str;
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
			parse_flag_type(ss, line_num, flag_type);
		}
		else if (command == "AF") {
			auto& vec = flag_aliases;
			auto func = [&](istream& inn, Flag_Set& p) {
				decode_flags(inn, line_num, flag_type, encoding,
				             flags);
				p = flags;
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "AM") {
			auto& vec = morphological_aliases;
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec,
			                  parse_morhological_fields);
		}
		else if (command == "BREAK") {
			auto& vec = break_patterns;
			auto func = [&](istream& in, string& p) { in >> p; };
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
			break_exists = true;
		}
		else if (command == "CHECKCOMPOUNDPATTERN") {
			auto& vec = compound_check_patterns;
			auto func = [&](istream& in,
			                Compound_Check_Pattern& p) {
				parse_word_slash_single_flag(
				    in, line_num, flag_type, encoding,
				    p.first_word_end, p.first_word_flag);
				parse_word_slash_single_flag(
				    in, line_num, flag_type, encoding,
				    p.second_word_begin, p.second_word_flag);
				if (in.fail()) {
					return;
				}
				in >> p.replacement;
				reset_failbit_istream(in);
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, vec, func);
		}
		else if (command == "COMPOUNDRULE") {
			auto func = [&](istream& in, u16string& rule) {
				parse_compound_rule(in, line_num, flag_type,
				                    encoding, rule);
			};
			parse_vector_of_T(ss, line_num, command,
			                  cmd_with_vec_cnt, rules, func);
		}
		else if (command == "COMPOUNDSYLLABLE") {
			ss >> compound_syllable_max >> compound_syllable_vowels;
		}
		else if (command == "SYLLABLENUM") {
			decode_flags(ss, line_num, flag_type, encoding, flags);
			compound_syllable_num = flags;
		}
		if (ss.fail()) {
			cerr
			    << "Nuspell error: could not parse affix file line "
			    << line_num << ": " << line << endl;
		}
	}
	// default BREAK definition
	if (!break_exists) {
		break_patterns = {"-", "^-", "-$"};
	}
	for (auto& r : replacements) {
		auto& s = r.second;
		replace_char(s, '_', ' ');
	}

	// now fill data structures from temporary data
	set_encoding_and_language(encoding.value_or_default(), language_code);
	compound_rules = move(rules);
	if (encoding.is_utf8()) {
		auto u8_to_w = [](auto& x) { return utf8_to_wide(x); };
		auto u8_to_w_pair = [](auto& x) {
			return make_pair(utf8_to_wide(x.first),
			                 utf8_to_wide(x.second));
		};
		auto iconv =
		    boost::adaptors::transform(input_conversion, u8_to_w_pair);
		wide_structures.input_substr_replacer = iconv;
		auto oconv =
		    boost::adaptors::transform(output_conversion, u8_to_w_pair);
		wide_structures.output_substr_replacer = oconv;

		auto break_pat =
		    boost::adaptors::transform(break_patterns, u8_to_w);
		wide_structures.break_table = break_pat;
		wide_structures.ignored_chars = u8_to_w(ignore_chars);

		for (auto& x : prefixes) {
			auto appending = u8_to_w(x.appending);
			erase_chars(appending, wide_structures.ignored_chars);
			wide_structures.prefixes.emplace(
			    x.flag, x.cross_product, u8_to_w(x.stripping),
			    appending, x.new_flags, u8_to_w(x.condition));
		}
		for (auto& x : suffixes) {
			auto appending = u8_to_w(x.appending);
			erase_chars(appending, wide_structures.ignored_chars);
			wide_structures.suffixes.emplace(
			    x.flag, x.cross_product, u8_to_w(x.stripping),
			    appending, x.new_flags, u8_to_w(x.condition));
		}
		for (auto& x : compound_check_patterns) {
			auto forbid_unaffixed = x.first_word_end == "0";
			if (forbid_unaffixed)
				x.first_word_end.clear();
			wide_structures.compound_patterns.push_back(
			    {{u8_to_w(x.first_word_end),
			      u8_to_w(x.second_word_begin)},
			     u8_to_w(x.replacement),
			     x.first_word_flag,
			     x.second_word_flag,
			     forbid_unaffixed});
		}
		auto reps =
		    boost::adaptors::transform(replacements, u8_to_w_pair);
		wide_structures.replacements = reps;

		auto maps =
		    boost::adaptors::transform(map_related_chars, u8_to_w);
		wide_structures.similarities.assign(begin(maps), end(maps));
		wide_structures.keyboard_closeness = u8_to_w(keyboard_layout);
		wide_structures.try_chars = u8_to_w(try_chars);
		auto phone = boost::adaptors::transform(phonetic_replacements,
		                                        u8_to_w_pair);
		wide_structures.phonetic_table = phone;
	}
	else {
		// convert non-unicode dicts to unicode
		auto n_to_w = [&](auto& x) {
			return to_wide(x, this->internal_locale);
		};
		auto n_to_w_pair = [&](auto& x) {
			return make_pair(
			    to_wide(x.first, this->internal_locale),
			    to_wide(x.second, this->internal_locale));
		};
		auto iconv =
		    boost::adaptors::transform(input_conversion, n_to_w_pair);
		wide_structures.input_substr_replacer = iconv;
		auto oconv =
		    boost::adaptors::transform(output_conversion, n_to_w_pair);
		wide_structures.output_substr_replacer = oconv;

		auto break_pat =
		    boost::adaptors::transform(break_patterns, n_to_w);
		wide_structures.break_table = break_pat;
		wide_structures.ignored_chars = n_to_w(ignore_chars);

		for (auto& x : prefixes) {
			auto appending = n_to_w(x.appending);
			erase_chars(appending, wide_structures.ignored_chars);
			wide_structures.prefixes.emplace(
			    x.flag, x.cross_product, n_to_w(x.stripping),
			    appending, x.new_flags, n_to_w(x.condition));
		}
		for (auto& x : suffixes) {
			auto appending = n_to_w(x.appending);
			erase_chars(appending, wide_structures.ignored_chars);
			wide_structures.suffixes.emplace(
			    x.flag, x.cross_product, n_to_w(x.stripping),
			    appending, x.new_flags, n_to_w(x.condition));
		}
		for (auto& x : compound_check_patterns) {
			auto forbid_unaffixed = x.first_word_end == "0";
			if (forbid_unaffixed)
				x.first_word_end.clear();
			wide_structures.compound_patterns.push_back(
			    {{n_to_w(x.first_word_end),
			      n_to_w(x.second_word_begin)},
			     n_to_w(x.replacement),
			     x.first_word_flag,
			     x.second_word_flag,
			     forbid_unaffixed});
		}
		auto reps =
		    boost::adaptors::transform(replacements, n_to_w_pair);
		wide_structures.replacements = reps;

		auto maps =
		    boost::adaptors::transform(map_related_chars, n_to_w);
		wide_structures.similarities.assign(begin(maps), end(maps));
		wide_structures.keyboard_closeness = n_to_w(keyboard_layout);
		wide_structures.try_chars = n_to_w(try_chars);
		auto phone = boost::adaptors::transform(phonetic_replacements,
		                                        n_to_w_pair);
		wide_structures.phonetic_table = phone;

		// set_encoding_and_language("UTF-8", language_code);
		// No need to set the internal locale to utf-8, keep it to
		// non-unicode because:
		// 1) We still need it as is in parse_dic().
		// 2) We will later use only the wide facets which are Unicode
		//    anyway.
	}

	cerr.flush();
	return in.eof(); // success when eof is reached
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
	istringstream ss;
	string line;

	// locale must be without thousands separator.
	auto loc = locale::classic();
	Setlocale_To_C_In_Scope setlocale_to_C;
	in.imbue(loc);
	ss.imbue(loc);
	strip_utf8_bom(in);
	if (!getline(in, line)) {
		return false;
	}
	auto encoding = Encoding(
	    use_facet<boost::locale::info>(internal_locale).encoding());
	if (encoding.is_utf8() && !validate_utf8(line)) {
		cerr << "Invalid utf in dic file" << endl;
	}
	ss.str(line);
	if (ss >> approximate_size) {
		words.reserve(approximate_size);
	}
	else {
		return false;
	}

	string word;
	string morph;
	vector<string> morphs;
	u16string flags;
	wstring wide_word;

	while (getline(in, line)) {
		line_number++;
		ss.str(line);
		ss.clear();
		word.clear();
		morph.clear();
		flags.clear();
		morphs.clear();

		if (encoding.is_utf8() && !validate_utf8(line)) {
			cerr << "Invalid utf in dic file" << endl;
		}
		size_t slash_pos = 0;
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
			ss.ignore(slash_pos + 1);
			decode_flags_possible_alias(ss, line_number, flag_type,
			                            encoding, flag_aliases,
			                            flags);
			if (ss.fail())
				continue;
		}
		else if (line.find('\t') != line.npos) {
			// Tab found, word until tab. No flags.
			// After tab follow morphological fields
			getline(ss, word, '\t');
		}
		else {
			auto end = dic_find_end_of_word_heuristics(line);
			word.assign(line, 0, end);
			ss.ignore(end);
		}
		if (word.empty()) {
			continue;
		}
		// parse_morhological_fields(ss, morphs);

		auto casing = Casing();
		auto ok = false;
		if (encoding.is_utf8()) {
			ok = utf8_to_wide(word, wide_word);
		}
		else {
			ok = to_wide(word, internal_locale, wide_word);
			wide_to_utf8(wide_word, word);
		}
		if (!ok)
			continue;
		if (!wide_structures.ignored_chars.empty()) {
			erase_chars(wide_word, wide_structures.ignored_chars);
			wide_to_utf8(wide_word, word);
		}
		casing = classify_casing(wide_word, internal_locale);

		const char16_t HIDDEN_HOMONYM_FLAG = -1;
		switch (casing) {
		case Casing::ALL_CAPITAL: {
			// check for hidden homonym
			auto hom = words.equal_range_nonconst_unsafe(word);
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
				words.emplace(word, flags);
			}
			break;
		}
		case Casing::PASCAL:
		case Casing::CAMEL: {
			words.emplace(word, flags);

			// add the hidden homonym directly in uppercase
			auto up_wide =
			    boost::locale::to_upper(wide_word, internal_locale);
			wide_to_utf8(wide_word, word);
			auto& up = word;
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
			words.emplace(word, flags);
			break;
		}
	}
	return in.eof(); // success if we reached eof
}
} // namespace nuspell
