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

#ifndef HUNSPELL_DIC_MANAGER_HXX
#define HUNSPELL_DIC_MANAGER_HXX

#include "aff_data.hxx"
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace nuspell {

class Dic_Data {
	// word and flag vector
	// efficient for short flag vectors
	// for long flag vectors like in Korean dict
	// we should keep pointers to the string in the affix aliases vector
	// for now we will leave it like this
	using MapT = std::unordered_map<std::string, Flag_Set>;
	MapT words;

	// word and morphological data
	// we keep them separate because morph data is generally absent
	std::unordered_map<std::string, std::vector<std::string>> morph_data;

      public:
	using iterator = MapT::iterator;
	using const_iterator = MapT::const_iterator;

	// methods
	// parses the dic data to hashtable
	auto parse(std::istream& in, const Aff_Data& aff) -> bool;

	auto& data() const { return words; }
	auto begin() { return words.begin(); }
	auto begin() const { return words.begin(); }
	auto end() { return words.end(); }
	auto end() const { return words.end(); }

	auto find(const std::string& word) { return words.find(word); }
	auto find(const std::string& word) const { return words.find(word); }
	auto find(const std::wstring& word) -> iterator;
	auto find(const std::wstring& word) const -> const_iterator;
	auto equal_range(const std::string& word)
	{
		return words.equal_range(word);
	}
	auto equal_range(const std::string& word) const
	{
		return words.equal_range(word);
	}
	auto equal_range(const std::wstring& word)
	    -> std::pair<iterator, iterator>;
	auto equal_range(const std::wstring& word) const
	    -> std::pair<const_iterator, const_iterator>;

	auto lookup(const std::string& word) -> Flag_Set*;
	auto lookup(const std::string& word) const -> const Flag_Set*;
	auto lookup(const std::wstring& word) -> Flag_Set*;
	auto lookup(const std::wstring& word) const -> const Flag_Set*;
};
}

#endif
