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

#include "dic_data.hxx"

#include "locale_utils.hxx"
#include "string_utils.hxx"
#include <algorithm>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>

#include <boost/locale.hpp>

namespace nuspell {

using namespace std;

/**
 * Parses an input stream offering dictionary information.
 *
 * @param in input stream to read from.
 * @param aff affix data to retrive locale and flag settings from.
 * @return The boolean indication reacing the end of stream after parsing.
 */
auto Dic_Data::parse(istream& in, const Aff_Data& aff) -> bool
{
	size_t line_number = 1;
	size_t approximate_size;
	istringstream ss;
	string line;

	// locale must be without thousands separator.
	// boost::locale::generator locale_generator;
	// auto loc = locale_generator("en_US.us-ascii");
	auto loc = locale::classic();
	in.imbue(loc);
	ss.imbue(loc);
	if (!getline(in, line)) {
		return false;
	}
	if (aff.encoding.is_utf8() && !validate_utf8(line)) {
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

	regex morph_field(R"(\s+[a-z][a-z]:)");
	smatch mf_match;

	while (getline(in, line)) {
		line_number++;
		ss.str(line);
		ss.clear();
		word.clear();
		morph.clear();
		flags.clear();
		morphs.clear();

		if (aff.encoding.is_utf8() && !validate_utf8(line)) {
			cerr << "Invalid utf in dic file" << endl;
		}
		if (line.find('/') != line.npos) {
			// slash found, word untill slash
			getline(ss, word, '/');
			if (aff.flag_aliases.empty()) {
				flags = aff.decode_flags(ss);
			}
			else {
				size_t flag_alias_idx;
				ss >> flag_alias_idx;
				if (ss.fail() ||
				    flag_alias_idx > aff.flag_aliases.size()) {
					continue;
				}
				flags = aff.flag_aliases[flag_alias_idx - 1];
			}
		}
		else if (line.find('\t') != line.npos) {
			// Tab found, word until tab. No flags.
			// After tab follow morphological fields
			getline(ss, word, '\t');
		}
		else if (regex_search(line, mf_match, morph_field)) {
			word = mf_match.prefix();
			ss.ignore(mf_match.position());
		}
		else {
			// No slash or tab or morph field "aa:".
			// Treat word until end.
			getline(ss, word);
		}
		if (word.empty()) {
			continue;
		}
		parse_morhological_fields(ss, morphs);
		words[word] += flags;
		if (morphs.size()) {
			auto& vec = morph_data[word];
			vec.insert(vec.end(), morphs.begin(), morphs.end());
		}
	}
	return in.eof(); // success if we reached eof
}

auto Dic_Data::find(const wstring& word) -> iterator
{
	return find(boost::locale::conv::utf_to_utf<char>(word));
}

auto Dic_Data::find(const wstring& word) const -> const_iterator
{
	return find(boost::locale::conv::utf_to_utf<char>(word));
}
auto Dic_Data::equal_range(const std::wstring& word)
    -> std::pair<iterator, iterator>
{
	return equal_range(boost::locale::conv::utf_to_utf<char>(word));
}
auto Dic_Data::equal_range(const std::wstring& word) const
    -> std::pair<const_iterator, const_iterator>
{
	return equal_range(boost::locale::conv::utf_to_utf<char>(word));
}

/**
 * Looks up a word in the unordered map words.
 *
 * @param word string to look up.
 * @return The flag set belonging to the word found or a null pointer when
 * nothing has been found.
 */
auto Dic_Data::lookup(const string& word) -> Flag_Set*
{
	auto kv = words.find(word);
	if (kv != end())
		return &kv->second;
	return nullptr;
}

auto Dic_Data::lookup(const string& word) const -> const Flag_Set*
{
	auto kv = words.find(word);
	if (kv != end())
		return &kv->second;
	return nullptr;
}

auto Dic_Data::lookup(const wstring& word) -> Flag_Set*
{
	return lookup(boost::locale::conv::utf_to_utf<char>(word));
}

auto Dic_Data::lookup(const wstring& word) const -> const Flag_Set*
{
	return lookup(boost::locale::conv::utf_to_utf<char>(word));
}
}
