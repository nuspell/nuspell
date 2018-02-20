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

#include "dictionary.hxx"

#include <boost/range/iterator_range_core.hpp>

namespace hunspell {

using namespace std;

/**
 * Checks prefix.
 *
 * @param[in] dic dictionary to use.
 * @param[in] affix_table table with prefixes.
 * @param[in] word to check.
 * @return The TODO.
 */
auto prefix_check(const Dic_Data& dic, const Prefix_Table& affix_table,
                  string word)
{
	for (size_t aff_len = 1; 0 < aff_len && aff_len <= word.size();
	     ++aff_len) {

		auto affix = word.substr(0, aff_len);
		auto entries = affix_table.equal_range(affix);
		for (auto& e : boost::make_iterator_range(entries)) {

			e.to_root(word);
			auto matched = e.check_condition(word);
			if (matched) {
				auto flags = dic.lookup(word);
				if (flags && flags->exists(e.flag)) {
					// success
					return true;
				}
			}
			e.to_derived(word);
		}
	}
	return false;
}
}
