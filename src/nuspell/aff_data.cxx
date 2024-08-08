/* Copyright 2016-2024 Dimitrij Mijoski
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
#include "utils.hxx"

#include <cassert>
#include <charconv>
#include <locale>
#include <sstream>
#include <unordered_map>

using namespace std;

/**
 * @brief Library main namespace
 */
namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

auto Encoding::normalize_name() -> void
{
	to_upper_ascii(name);
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
	NO_FLAGS_AFTER_SLASH_WARNING = -16,
	NONUTF8_FLAGS_ABOVE_127_WARNING,
	ARRAY_COMMAND_EXTRA_ENTRIES_WARNING,
	MULTIPLE_ENTRIES_WARNING,
	NO_ERROR = 0,
	ISTREAM_READING_ERROR,
	INVALID_ENCODING_IDENTIFIER,
	ENCODING_CONVERSION_ERROR,
	INVALID_FLAG_TYPE,
	INVALID_LANG_IDENTIFIER,
	MISSING_FLAGS,
	UNPAIRED_LONG_FLAG,
	INVALID_NUMERIC_FLAG,
	// FLAGS_ARE_UTF8_BUT_FILE_NOT,
	INVALID_UTF8,
	FLAG_ABOVE_65535,
	INVALID_NUMERIC_ALIAS,
	AFX_CROSS_CHAR_INVALID,
	AFX_CONDITION_INVALID_FORMAT,
	COMPOUND_RULE_INVALID_FORMAT,
	ARRAY_COMMAND_NO_COUNT
};

auto decode_flags(string_view s, Flag_Type t, const Encoding& enc,
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
			unsigned char c1 = *i;
			unsigned char c2 = *(i + 1);
			out.push_back((c1 << 8) | c2);
		}
		break;
	}
	case Ft::NUMBER:
		for (auto p = begin_ptr(s);;) {
			uint16_t flag;
			auto fc = from_chars(p, end_ptr(s), flag);
			if (fc.ec == errc::invalid_argument)
				return Err::INVALID_NUMERIC_FLAG;
			if (fc.ec == errc::result_out_of_range)
				return Err::FLAG_ABOVE_65535;
			out.push_back(flag);

			if (fc.ptr == end_ptr(s) || *fc.ptr != ',')
				break;

			p = fc.ptr + 1;
		}
		break;
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

auto decode_flags_possible_alias(string_view s, Flag_Type t,
                                 const Encoding& enc,
                                 const vector<Flag_Set>& flag_aliases,
                                 u16string& out) -> Parsing_Error_Code
{
	if (flag_aliases.empty())
		return decode_flags(s, t, enc, out);

	out.clear();
	size_t i;
	auto fc = from_chars(begin_ptr(s), end_ptr(s), i);
	if (fc.ec == errc::invalid_argument ||
	    fc.ec == errc::result_out_of_range)
		return Parsing_Error_Code::INVALID_NUMERIC_ALIAS;

	if (0 < i && i <= flag_aliases.size()) {
		out = flag_aliases[i - 1];
		return {};
	}
	return Parsing_Error_Code::INVALID_NUMERIC_ALIAS;
}

auto get_parsing_error_message(Parsing_Error_Code err)
{
	using Err = Parsing_Error_Code;
	switch (err) {
	case Err::NO_FLAGS_AFTER_SLASH_WARNING:
		return "Nuspell warning: no flags after slash.";
	case Err::NONUTF8_FLAGS_ABOVE_127_WARNING:
		return "Nuspell warning: bytes above 127 in flags in UTF-8 "
		       "file are treated as lone bytes for backward "
		       "compatibility. That means if in the flags you have "
		       "ONE character above ASCII, it may be interpreted as "
		       "2, 3, or 4 flags. Please update dictionary and affix "
		       "files to use FLAG UTF-8 and make the file valid "
		       "UTF-8 if it is not already.";
	case Err::ARRAY_COMMAND_EXTRA_ENTRIES_WARNING:
		return "Nuspell warning: extra entries of array command.";
	case Err::MULTIPLE_ENTRIES_WARNING:
		return "Nuspell warning: multiple entries the same command.";
	case Err::NO_ERROR:
		return "";
	case Err::ISTREAM_READING_ERROR:
		return "Nuspell error: problem reading number or string from "
		       "istream.";
	case Err::INVALID_ENCODING_IDENTIFIER:
		return "Nuspell error: Invalid identifier of encoding.";
	case Err::ENCODING_CONVERSION_ERROR:
		return "Nuspell error: encoding conversion error.";
	case Err::INVALID_FLAG_TYPE:
		return "Nuspell error: invalid identifier for the type of the "
		       "flags.";
	case Err::INVALID_LANG_IDENTIFIER:
		return "Nuspell error: invalid language code.";
	case Err::MISSING_FLAGS:
		return "Nuspell error: missing flags.";
	case Err::UNPAIRED_LONG_FLAG:
		return "Nuspell error: the number of chars in string of long "
		       "flags is odd, should be even.";
	case Err::INVALID_NUMERIC_FLAG:
		return "Nuspell error: invalid numerical flag.";
	// case Err::FLAGS_ARE_UTF8_BUT_FILE_NOT:
	//	return "Nuspell error: flags are UTF-8 but file is not\n";
	case Err::INVALID_UTF8:
		return "Nuspell error: Invalid UTF-8 in flags";
	case Err::FLAG_ABOVE_65535:
		return "Nuspell error: Flag above 65535 in line";
	case Err::INVALID_NUMERIC_ALIAS:
		return "Nuspell error: Flag alias is invalid.";
	case Err::AFX_CROSS_CHAR_INVALID:
		return "Nuspell error: Invalid cross char in affix entry. It "
		       "must be Y or N.";
	case Err::AFX_CONDITION_INVALID_FORMAT:
		return "Nuspell error: Affix condition is invalid.";
	case Err::COMPOUND_RULE_INVALID_FORMAT:
		return "Nuspell error: Compound rule is in invalid format.";
	case Err::ARRAY_COMMAND_NO_COUNT:
		return "Nuspell error: The first line of array command (series "
		       "of similar commands) has no count. Ignoring all of "
		       "them.";
	}
	return "Unknown error";
}

auto decode_compound_rule(string_view s, Flag_Type t, const Encoding& enc,
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
		for (auto p = begin_ptr(s); p != end_ptr(s);) {
			if (*p != '(')
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			++p;
			uint16_t flag;
			auto fc = from_chars(p, end_ptr(s), flag);
			if (fc.ec == errc::invalid_argument)
				return Err::INVALID_NUMERIC_FLAG;
			if (fc.ec == errc::result_out_of_range)
				return Err::FLAG_ABOVE_65535;
			p = fc.ptr;
			if (p == end_ptr(s) || *p != ')')
				return Err::COMPOUND_RULE_INVALID_FORMAT;
			out.push_back(flag);
			++p;
			if (p == end_ptr(s))
				break;
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
	in.read(bom.data(), 3);
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

class Aff_Line_Parser {
	std::string str_buf;
	std::u16string flag_buffer;

	const Aff_Data* aff_data = nullptr;
	Encoding_Converter cvt;

	using Err = Parsing_Error_Code;

      public:
	Parsing_Error_Code err = {};

	Aff_Line_Parser() = default;
	Aff_Line_Parser(Aff_Data& a)
	    : aff_data(&a), cvt(a.encoding.value_or_default())
	{
	}

	auto& parse(istream& in, Encoding& enc)
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		enc = str_buf;
		cvt = Encoding_Converter(enc.value_or_default());
		if (!cvt.valid()) {
			err = Err::INVALID_ENCODING_IDENTIFIER;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto& parse(istream& in, std::string& str)
	{
		in >> str_buf;
		if (in.fail()) { // str_buf is unmodified on fail
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		auto ok = cvt.to_utf8(str_buf, str);
		if (!ok) {
			err = Err::INVALID_ENCODING_IDENTIFIER;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto& parse(istream& in, Flag_Type& flag_type)
	{
		using Ft = Flag_Type;
		flag_type = {};
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		to_upper_ascii(str_buf);
		if (str_buf == "LONG")
			flag_type = Ft::DOUBLE_CHAR;
		else if (str_buf == "NUM")
			flag_type = Ft::NUMBER;
		else if (str_buf == "UTF-8")
			flag_type = Ft::UTF8;
		else {
			err = Err::INVALID_FLAG_TYPE;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto& parse(istream& in, icu::Locale& loc)
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		loc = icu::Locale(str_buf.c_str());
		if (loc.isBogus()) {
			err = Err::INVALID_LANG_IDENTIFIER;
			in.setstate(in.failbit);
		}
		return in;
	}

      private:
	auto& parse_flags(istream& in, std::u16string& flags)
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		err = decode_flags(str_buf, aff_data->flag_type,
		                   aff_data->encoding, flags);
		if (static_cast<int>(err) > 0)
			in.setstate(in.failbit);
		return in;
	}

      public:
	auto& parse(istream& in, char16_t& flag)
	{
		flag = 0;
		parse_flags(in, flag_buffer);
		if (in)
			flag = flag_buffer[0];
		return in;
	}

	auto& parse(istream& in, Flag_Set& flags)
	{
		parse_flags(in, flag_buffer);
		if (in)
			flags = flag_buffer;
		return in;
	}

	auto& parse_word_slash_flags(istream& in, string& word, Flag_Set& flags)
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
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
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			return in;
		}
		auto ok = cvt.to_utf8(str_buf, word);
		if (!ok) {
			err = Err::ENCODING_CONVERSION_ERROR;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto parse_word_slash_single_flag(istream& in, string& word,
	                                  char16_t& flag) -> istream&
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
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
		if (static_cast<int>(err) > 0) {
			in.setstate(in.failbit);
			return in;
		}
		auto ok = cvt.to_utf8(str_buf, word);
		if (!ok) {
			err = Err::ENCODING_CONVERSION_ERROR;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto& parse_compound_rule(istream& in, u16string& out)
	{
		in >> str_buf;
		if (in.fail()) {
			err = Err::ISTREAM_READING_ERROR;
			return in;
		}
		err = decode_compound_rule(str_buf, aff_data->flag_type,
		                           aff_data->encoding, out);
		if (static_cast<int>(err) > 0)
			in.setstate(in.failbit);
		return in;
	}

	auto& parse(istream& in, pair<string, string>& out)
	{
		parse(in, out.first);
		parse(in, out.second);
		return in;
	}

	auto& parse(istream& in, Condition& x)
	{
		auto str = string();
		parse(in, str);
		if (in.fail())
			return in;
		try {
			x = std::move(str);
		}
		catch (const Condition_Exception& ex) {
			err = Parsing_Error_Code::AFX_CONDITION_INVALID_FORMAT;
			in.setstate(in.failbit);
		}
		return in;
	}

	auto& parse(istream& in, Compound_Rule_Ref_Wrapper x)
	{
		return parse_compound_rule(in, x.rule);
	}

	auto& parse(istream& in, string& word, Flag_Set& flags)
	{
		return parse_word_slash_flags(in, word, flags);
	}

	auto& parse(istream& in, string& word, char16_t& flag)
	{
		return parse_word_slash_single_flag(in, word, flag);
	}

	auto& parse(istream& in, Compound_Pattern& p)
	{
		auto first_word_end = string();
		auto second_word_begin = string();
		p.match_first_only_unaffixed_or_zero_affixed = false;
		parse(in, first_word_end, p.first_word_flag);
		parse(in, second_word_begin, p.second_word_flag);
		if (in.fail())
			return in;
		if (first_word_end == "0") {
			first_word_end.clear();
			p.match_first_only_unaffixed_or_zero_affixed = true;
		}
		p.begin_end_chars = {first_word_end, second_word_begin};
		parse(in, p.replacement); // optional
		if (in.fail() && in.eof() && !in.bad()) {
			err = {};
			reset_failbit_istream(in);
			p.replacement.clear();
		}
		return in;
	}
};

template <class T, class Func = identity>
auto parse_vector_of_T(istream& in, Aff_Line_Parser& p, const string& command,
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
		else {
			// cnt aka counts[command] remains 0.
			p.err = Parsing_Error_Code::ARRAY_COMMAND_NO_COUNT;
			in.setstate(in.failbit);
		}
	}
	else if (dat->second != 0) {
		dat->second--;
		auto& elem = vec.emplace_back();
		p.parse(in, modifier_wrapper(elem));
	}
	else {
		p.err = Parsing_Error_Code::ARRAY_COMMAND_EXTRA_ENTRIES_WARNING;
	}
}

template <class AffixT>
auto parse_affix(istream& in, Aff_Line_Parser& p, string& command,
                 vector<AffixT>& vec,
                 unordered_map<string, pair<bool, size_t>>& cmd_affix) -> void
{
	using Err = Parsing_Error_Code;
	char16_t f;
	p.parse(in, f);
	if (in.fail())
		return;
	command.append(reinterpret_cast<char*>(&f), sizeof(f));
	auto dat = cmd_affix.find(command);
	// note: the current affix parser does not allow the same flag
	// to be used once with cross product and again without
	// one flag is tied to one cross product value
	if (dat == cmd_affix.end()) {
		char cross_char; // 'Y' or 'N'
		size_t cnt;
		auto& cross_and_cnt = cmd_affix[command]; // == false, 0
		in >> cross_char >> cnt;
		if (in.fail()) {
			p.err = Parsing_Error_Code::ISTREAM_READING_ERROR;
			return;
		}
		if (cross_char != 'Y' && cross_char != 'N') {
			p.err = Err::AFX_CROSS_CHAR_INVALID;
			in.setstate(in.failbit);
			return;
		}
		bool cross = cross_char == 'Y';
		cross_and_cnt = {cross, cnt};
	}
	else if (dat->second.second) {
		dat->second.second--;
		auto& elem = vec.emplace_back();
		elem.flag = f;
		elem.cross_product = dat->second.first;
		p.parse(in, elem.stripping);
		if (elem.stripping == "0")
			elem.stripping.clear();
		p.parse(in, elem.appending, elem.cont_flags);
		if (elem.appending == "0")
			elem.appending.clear();
		if (in.fail())
			return;
		p.parse(in, elem.condition); // optional
		if (in.fail() && in.eof() && !in.bad()) {
			elem.condition = ".";
			p.err = {};
			reset_failbit_istream(in);
		}

		// in >> elem.morphological_fields;
	}
	else {
		p.err = Parsing_Error_Code::ARRAY_COMMAND_EXTRA_ENTRIES_WARNING;
	}
}

} // namespace

auto Aff_Data::parse_aff(istream& in, ostream& err_msg) -> bool
{
	auto prefixes = vector<Prefix>();
	auto suffixes = vector<Suffix>();
	auto break_patterns = vector<string>();
	auto break_exists = false;
	auto input_conversion = vector<pair<string, string>>();
	auto output_conversion = vector<pair<string, string>>();
	// auto morphological_aliases = vector<vector<string>>();
	auto rules = vector<u16string>();
	auto replacements = vector<pair<string, string>>();
	auto map_related_chars = vector<string>();
	auto phonetic_replacements = vector<pair<string, string>>();

	max_compound_suggestions = 3;
	max_ngram_suggestions = 4;
	max_diff_factor = 5;
	flag_type = Flag_Type::SINGLE_CHAR;

	unordered_map<string, string*> command_strings = {
	    {"IGNORE", &ignored_chars},

	    {"KEY", &keyboard_closeness},
	    {"TRY", &try_chars}};

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
	    {"SYLLABLENUM", &compound_syllable_num},

	    {"FULLSTRIP", &fullstrip},
	    {"CHECKSHARPS", &checksharps}};

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

	// keeps count for each array-command
	auto cmd_with_vec_cnt = unordered_map<string, size_t>();
	auto cmd_affix = unordered_map<string, pair<bool, size_t>>();
	auto line = string();
	auto command = string();
	auto line_num = size_t(0);
	auto ss = istringstream();
	auto p = Aff_Line_Parser(*this);
	auto error_happened = false;
	// while parsing, the streams must have plain ascii locale without
	// any special number separator otherwise istream >> int might fail
	// due to thousands separator.
	// "C" locale can be used assuming it is US-ASCII
	in.imbue(locale::classic());
	ss.imbue(locale::classic());
	strip_utf8_bom(in);
	while (getline(in, line)) {
		line_num++;
		ss.str(line);
		ss.clear();
		p.err = {};
		ss >> ws;
		if (ss.eof() || ss.peek() == '#')
			continue; // skip comment or empty lines
		ss >> command;
		to_upper_ascii(command);
		if (command == "SFX") {
			parse_affix(ss, p, command, suffixes, cmd_affix);
		}
		else if (command == "PFX") {
			parse_affix(ss, p, command, prefixes, cmd_affix);
		}
		else if (command_strings.count(command)) {
			auto& str = *command_strings[command];
			if (str.empty())
				p.parse(ss, str);
			else
				p.err = Parsing_Error_Code::
				    MULTIPLE_ENTRIES_WARNING;
		}
		else if (command_bools.count(command)) {
			*command_bools[command] = true;
		}
		else if (command_shorts.count(command)) {
			auto ptr = command_shorts[command];
			ss >> *ptr;
			if (ptr == &compound_min_length && *ptr == 0)
				compound_min_length = 1;
			if (ptr == &max_diff_factor && max_diff_factor > 10)
				max_diff_factor = 5;
		}
		else if (command_flag.count(command)) {
			p.parse(ss, *command_flag[command]);
		}
		else if (command == "MAP") {
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  map_related_chars);
		}
		else if (command_vec_pair.count(command)) {
			auto& vec = *command_vec_pair[command];
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  vec);
		}
		else if (command == "SET") {
			if (encoding.empty())
				p.parse(ss, encoding);
			else
				p.err = Parsing_Error_Code::
				    MULTIPLE_ENTRIES_WARNING;
		}
		else if (command == "FLAG") {
			p.parse(ss, flag_type);
		}
		else if (command == "LANG") {
			p.parse(ss, icu_locale);
		}
		else if (command == "AF") {
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  flag_aliases);
		}
		else if (command == "AM") {
			// parse_vector_of_T(ss, command, cmd_with_vec_cnt,
			//                  morphological_aliases);
		}
		else if (command == "BREAK") {
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  break_patterns);
			break_exists = true;
		}
		else if (command == "CHECKCOMPOUNDPATTERN") {
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  compound_patterns);
		}
		else if (command == "COMPOUNDRULE") {
			parse_vector_of_T(ss, p, command, cmd_with_vec_cnt,
			                  rules, wrap_compound_rule);
		}
		else if (command == "COMPOUNDSYLLABLE") {
			ss >> compound_syllable_max;
			p.parse(ss, compound_syllable_vowels);
		}
		else if (command == "WORDCHARS") {
			p.parse(ss, wordchars);
		}
		if (ss.fail()) {
			assert(static_cast<int>(p.err) > 0);
			error_happened = true;
			err_msg << "Nuspell error: could not parse affix file. "
			        << line_num << ": " << line << '\n'
			        << get_parsing_error_message(p.err) << endl;
		}
		else if (p.err != Parsing_Error_Code::NO_ERROR) {
			assert(static_cast<int>(p.err) < 0);
			err_msg << "Nuspell warning: while parsing affix file. "
			        << line_num << ": " << line << '\n'
			        << get_parsing_error_message(p.err) << endl;
		}
	}
	// default BREAK definition
	if (!break_exists) {
		break_patterns = {"-", "^-", "-$"};
	}
	for (auto& r : replacements) {
		auto& s = r.second;
		replace_ascii_char(s, '_', ' ');
	}

	// now fill data structures from temporary data
	compound_rules = std::move(rules);
	similarities.assign(begin(map_related_chars), end(map_related_chars));
	break_table = std::move(break_patterns);
	input_substr_replacer = std::move(input_conversion);
	output_substr_replacer = std::move(output_conversion);
	this->replacements = std::move(replacements);
	// phonetic_table = std::move(phonetic_replacements);
	for (auto& x : prefixes) {
		erase_chars(x.appending, ignored_chars);
	}
	for (auto& x : suffixes) {
		erase_chars(x.appending, ignored_chars);
	}
	this->prefixes = std::move(prefixes);
	this->suffixes = std::move(suffixes);

	return in.eof() && !error_happened; // true for success
}

auto Aff_Data::parse_dic(istream& in, ostream& err_msg) -> bool
{
	size_t line_number = 1;
	size_t approximate_size;
	string line;
	string word;
	string flags_str;
	u16string flags;
	string u8word;
	auto enc_conv = Encoding_Converter(encoding.value_or_default());
	auto success = true;

	// locale must be without thousands separator.
	auto& ctype = use_facet<std::ctype<char>>(locale::classic());
	in.imbue(locale::classic());

	strip_utf8_bom(in);
	in >> approximate_size;
	if (in.fail()) {
		err_msg << "Nuspell error: while parsing first line of .dic "
		           "file. There is no number."
		        << endl;
		return false;
	}
	words.reserve(approximate_size);
	getline(in, line);

	while (getline(in, line)) {
		line_number++;
		word.clear();
		flags_str.clear();
		flags.clear();
		if (!empty(line) && line.back() == '\r')
			line.pop_back();

		auto end_word_pos = line.npos;
		for (size_t i = 0; i != size(line); ++i) {
			switch (line[i]) {
			case '/':
				if (i == 0)
					continue;
				if (line[i - 1] == '\\') {
					--i;
					line.erase(i, 1);
				}
				else {
					end_word_pos = i;
				}
				break;
			case '\t':
				end_word_pos = i;
				break;
			case ' ': {
				auto p = ctype.scan_not(
				    ctype.space, &line[i + 1], end_ptr(line));
				size_t k = p - begin_ptr(line);
				if (k == size(line) ||
				    (size(line) - k >= 3 &&
				     line[k + 2] == ':' &&
				     ctype.is(ctype.lower, line[k]) &&
				     ctype.is(ctype.lower, line[k + 1])))
					end_word_pos = i;
				break;
			}
			}
			if (end_word_pos != line.npos)
				break;
		}
		word.assign(line, 0, end_word_pos);
		if (end_word_pos != line.npos && line[end_word_pos] == '/') {
			// slash found, word until slash
			auto slash_pos = end_word_pos;
			auto ptr = ctype.scan_is(ctype.space, &line[slash_pos],
			                         end_ptr(line));
			auto end_flags_pos = ptr - begin_ptr(line);
			flags_str.assign(line, slash_pos + 1,
			                 end_flags_pos - (slash_pos + 1));
			auto err = decode_flags_possible_alias(
			    flags_str, flag_type, encoding, flag_aliases,
			    flags);
			if (err == Parsing_Error_Code::MISSING_FLAGS ||

			    (/* bug in eu.dic that we fix here. Remove this once
			        fixed there. */
			     err == Parsing_Error_Code::INVALID_NUMERIC_FLAG &&
			     flags_str == "None"))
				err = Parsing_Error_Code::
				    NO_FLAGS_AFTER_SLASH_WARNING;
			if (static_cast<int>(err) > 0) {
				err_msg << "Nuspell error: while parsing "
				           ".dic file. "
				        << line_number << ": " << line << '\n'
				        << get_parsing_error_message(err)
				        << endl;
				success = false;
				continue;
			}
			else if (static_cast<int>(err) < 0) {
				err_msg << "Nuspell warning: while parsing "
				           ".dic file. "
				        << line_number << ": " << line << '\n'
				        << get_parsing_error_message(err)
				        << endl;
			}
		}
		if (empty(word))
			continue;
		auto ok = enc_conv.to_utf8(word, u8word);
		if (!ok)
			continue;
		erase_chars(u8word, ignored_chars);
		auto casing = classify_casing(u8word);
		auto inserted = words.emplace(u8word, flags);
		switch (casing) {
		case Casing::ALL_CAPITAL:
			if (flags.empty())
				break;
			[[fallthrough]];
		case Casing::PASCAL:
		case Casing::CAMEL: {
			// This if is needed for the test allcaps2.dic.
			// Maybe it can be solved better by not checking the
			// forbiddenword_flag, but by keeping the hidden
			// homonym last in the multimap among the same-key
			// entries.
			if (inserted->second.contains(forbiddenword_flag))
				break;
			to_title(u8word, icu_locale, u8word);
			flags += HIDDEN_HOMONYM_FLAG;
			words.emplace(u8word, flags);
			break;
		}
		default:
			break;
		}
	}
	return in.eof() && success; // success if we reached eof
}
NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
