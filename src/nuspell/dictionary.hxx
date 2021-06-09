/* Copyright 2016-2020 Dimitrij Mijoski
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

/**
 * @file
 * @brief Dictionary spelling.
 */

#ifndef NUSPELL_DICTIONARY_HXX
#define NUSPELL_DICTIONARY_HXX

#include "checker.hxx"

namespace nuspell {
inline namespace v5 {

struct NUSPELL_EXPORT Suggester : public Checker {

	enum High_Quality_Sugs : bool {
		ALL_LOW_QUALITY_SUGS = false,
		HAS_HIGH_QUALITY_SUGS = true
	};

	auto suggest_priv(std::string_view input_word, List_Strings& out) const
	    -> void;

	auto suggest_low(std::string& word, List_Strings& out) const
	    -> High_Quality_Sugs;

	auto add_sug_if_correct(std::string& word, List_Strings& out) const
	    -> bool;

	auto uppercase_suggest(const std::string& word, List_Strings& out) const
	    -> void;

	auto rep_suggest(std::string& word, List_Strings& out) const -> void;

	auto try_rep_suggestion(std::string& word, List_Strings& out) const
	    -> void;

	auto max_attempts_for_long_alogs(std::string_view word) const -> size_t;

	auto map_suggest(std::string& word, List_Strings& out) const -> void;

	auto map_suggest(std::string& word, List_Strings& out, size_t i,
	                 size_t& remaining_attempts) const -> void;

	auto adjacent_swap_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto distant_swap_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto keyboard_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto extra_char_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto forgotten_char_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto move_char_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto bad_char_suggest(std::string& word, List_Strings& out) const
	    -> void;

	auto doubled_two_chars_suggest(std::string& word,
	                               List_Strings& out) const -> void;

	auto two_words_suggest(const std::string& word, List_Strings& out) const
	    -> void;

	auto ngram_suggest(const std::string& word_u8, List_Strings& out) const
	    -> void;

	auto expand_root_word_for_ngram(Word_List::const_reference root,
	                                std::string_view wrong,
	                                List_Strings& expanded_list,
	                                std::vector<bool>& cross_affix) const
	    -> void;
};

/**
 * @brief The only important public exception
 */
class Dictionary_Loading_Error : public std::runtime_error {
      public:
	using std::runtime_error::runtime_error;
};

/**
 * @brief The only important public class
 */
class NUSPELL_EXPORT Dictionary : private Suggester {
	Dictionary(std::istream& aff, std::istream& dic);

      public:
	Dictionary();
	auto static load_from_aff_dic(std::istream& aff, std::istream& dic)
	    -> Dictionary;
	auto static load_from_path(
	    const std::string& file_path_without_extension) -> Dictionary;
	auto spell(std::string_view word) const -> bool;
	auto suggest(std::string_view word, std::vector<std::string>& out) const
	    -> void;
};
} // namespace v5
} // namespace nuspell
#endif // NUSPELL_DICTIONARY_HXX
