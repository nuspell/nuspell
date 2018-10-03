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

/**
 * @file dictionary.hxx
 * Dictionary spelling.
 */

#ifndef NUSPELL_DICTIONARY_HXX
#define NUSPELL_DICTIONARY_HXX

#include "aff_data.hxx"
#include "locale_utils.hxx"

#include <fstream>
#include <locale>

namespace nuspell {

enum Affixing_Mode {
	FULL_WORD,
	AT_COMPOUND_BEGIN,
	AT_COMPOUND_END,
	AT_COMPOUND_MIDDLE
};

template <class... Affixes>
struct Affixing_Result
    : public std::tuple<Word_List::const_pointer, const Affixes*...> {
	using base_type =
	    std::tuple<Word_List::const_pointer, const Affixes*...>;
	Affixing_Result() = default;
	Affixing_Result(Word_List::const_reference word_entry,
	                const Affixes&... affixes)
	    : base_type(&word_entry, &affixes...)
	{
	}
	// operator bool() const { return std::get<0>(*this); }
	operator Word_List::const_pointer() const { return std::get<0>(*this); }
	auto& operator*() const { return *std::get<0>(*this); }
	auto operator-> () const { return std::get<0>(*this); }
};

struct Compounding_Result {
	Word_List::const_pointer word_entry = {};
	bool affixed_and_modified = {}; /**< non-zero affix */
	operator Word_List::const_pointer() const { return word_entry; }
	auto& operator*() const { return *word_entry; }
	auto operator-> () const { return word_entry; }
};

struct Dict_Base : public Aff_Data {

	template <class CharT>
	auto spell_priv(std::basic_string<CharT>& s) const -> bool;
	template <class CharT>
	auto spell_break(std::basic_string<CharT>& s, size_t depth = 0) const
	    -> bool;
	template <class CharT>
	auto spell_casing(std::basic_string<CharT>& s) const -> const Flag_Set*;
	template <class CharT>
	auto spell_casing_upper(std::basic_string<CharT>& s) const
	    -> const Flag_Set*;
	template <class CharT>
	auto spell_casing_title(std::basic_string<CharT>& s) const
	    -> const Flag_Set*;
	template <class CharT>
	auto spell_sharps(std::basic_string<CharT>& base, size_t n_pos = 0,
	                  size_t n = 0, size_t rep = 0) const
	    -> const Flag_Set*;
	template <class CharT>
	auto check_word(std::basic_string<CharT>& s) const -> const Flag_Set*;

	template <Affixing_Mode m, class CharT>
	auto affix_NOT_valid(const Prefix<CharT>& a) const;
	template <Affixing_Mode m, class CharT>
	auto affix_NOT_valid(const Suffix<CharT>& a) const;
	template <Affixing_Mode m, class AffixT>
	auto outer_affix_NOT_valid(const AffixT& a) const;
	template <class AffixT>
	auto is_circumfix(const AffixT& a) const;
	template <Affixing_Mode m>
	auto is_valid_inside_compound(const Flag_Set& flags) const;

	/**
	 * @brief strip_prefix_only
	 * @param s derived word with affixes
	 * @return if found, root word + prefix
	 */
	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_only(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Prefix<CharT>>;

	/**
	 * @brief strip_suffix_only
	 * @param s derived word with affixes
	 * @return if found, root word + suffix
	 */
	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_suffix_only(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Suffix<CharT>>;

	/**
	 * @brief strip_prefix_then_suffix
	 *
	 * This accepts a derived word that was formed first by adding
	 * suffix then prefix to the root. The stripping is in reverse.
	 *
	 * @param s derived word with affixes
	 * @return if found, root word + suffix + prefix
	 */
	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_then_suffix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Suffix<CharT>, Prefix<CharT>>;

	template <Affixing_Mode m, class CharT>
	auto strip_pfx_then_sfx_2(const Prefix<CharT>& pe,
	                          std::basic_string<CharT>& s) const
	    -> Affixing_Result<Suffix<CharT>, Prefix<CharT>>;

	/**
	 * @brief strip_suffix_then_prefix
	 *
	 * This accepts a derived word that was formed first by adding
	 * prefix then suffix to the root. The stripping is in reverse.
	 *
	 * @param s derived word with prefix and suffix
	 * @return if found, root word + prefix + suffix
	 */
	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_suffix_then_prefix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Prefix<CharT>, Suffix<CharT>>;

	template <Affixing_Mode m, class CharT>
	auto strip_sfx_then_pfx_2(const Suffix<CharT>& se,
	                          std::basic_string<CharT>& s) const
	    -> Affixing_Result<Prefix<CharT>, Suffix<CharT>>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_then_suffix_commutative(
	    std::basic_string<CharT>& word) const
	    -> Affixing_Result<Suffix<CharT>, Prefix<CharT>>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_pfx_then_sfx_comm_2(const Prefix<CharT>& pe,
	                               std::basic_string<CharT>& word) const
	    -> Affixing_Result<Suffix<CharT>, Prefix<CharT>>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_suffix_then_suffix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Suffix<CharT>, Suffix<CharT>>;

	template <Affixing_Mode m, class CharT>
	auto strip_sfx_then_sfx_2(const Suffix<CharT>& se1,
	                          std::basic_string<CharT>& s) const
	    -> Affixing_Result<Suffix<CharT>, Suffix<CharT>>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_then_prefix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<Prefix<CharT>, Prefix<CharT>>;

	template <Affixing_Mode m, class CharT>
	auto strip_pfx_then_pfx_2(const Prefix<CharT>& pe1,
	                          std::basic_string<CharT>& s) const
	    -> Affixing_Result<Prefix<CharT>, Prefix<CharT>>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_then_2_suffixes(std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m, class CharT>
	auto strip_pfx_2_sfx_3(const Prefix<CharT>& pe1,
	                       const Suffix<CharT>& se1,
	                       std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_suffix_prefix_suffix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m, class CharT>
	auto strip_s_p_s_3(const Suffix<CharT>& se1, const Prefix<CharT>& pe1,
	                   std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_2_suffixes_then_prefix(std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m, class CharT>
	auto strip_2_sfx_pfx_3(const Suffix<CharT>& se1,
	                       const Suffix<CharT>& se2,
	                       std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_suffix_then_2_prefixes(std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m, class CharT>
	auto strip_sfx_2_pfx_3(const Suffix<CharT>& se1,
	                       const Prefix<CharT>& pe1,
	                       std::basic_string<CharT>& s) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_prefix_suffix_prefix(std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m, class CharT>
	auto strip_p_s_p_3(const Prefix<CharT>& pe1, const Suffix<CharT>& se1,
	                   std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;

	template <Affixing_Mode m = FULL_WORD, class CharT>
	auto strip_2_prefixes_then_suffix(std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;
	template <Affixing_Mode m, class CharT>
	auto strip_2_pfx_sfx_3(const Prefix<CharT>& pe1,
	                       const Prefix<CharT>& pe2,
	                       std::basic_string<CharT>& word) const
	    -> Affixing_Result<>;

	template <class CharT>
	auto check_compound(std::basic_string<CharT>& word) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN, class CharT>
	auto check_compound(std::basic_string<CharT>& word, size_t start_pos,
	                    size_t num_part,
	                    std::basic_string<CharT>& part) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN, class CharT>
	auto check_compound_classic(std::basic_string<CharT>& word,
	                            size_t start_pos, size_t i, size_t num_part,
	                            std::basic_string<CharT>& part) const
	    -> Compounding_Result;

	template <Affixing_Mode m = AT_COMPOUND_BEGIN, class CharT>
	auto check_compound_with_pattern_replacements(
	    std::basic_string<CharT>& word, size_t start_pos, size_t i,
	    size_t num_part, std::basic_string<CharT>& part) const
	    -> Compounding_Result;

	template <class CharT>
	auto check_compound_with_rules(std::basic_string<CharT>& word,
	                               std::vector<const Flag_Set*>& words_data,
	                               size_t start_pos,
	                               std::basic_string<CharT>& part) const
	    -> Compounding_Result;

	template <Affixing_Mode m, class CharT>
	auto check_word_in_compound(std::basic_string<CharT>& s) const
	    -> Compounding_Result;

	template <class CharT>
	auto suggest_priv(std::basic_string<CharT>& word,
	                  List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto add_sug_if_correct(std::basic_string<CharT>& word,
	                        List_Strings<CharT>& out) const -> bool;

	template <class CharT>
	auto try_rep_suggestion(std::basic_string<CharT>& word,
	                        List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto rep_suggest(std::basic_string<CharT>& word,
	                 List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto extra_char_suggest(std::basic_string<CharT>& word,
	                        List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto map_suggest(std::basic_string<CharT>& word,
	                 List_Strings<CharT>& out, size_t i = 0) const -> void;

	template <class CharT>
	auto keyboard_suggest(std::basic_string<CharT>& word,
	                      List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto bad_char_suggest(std::basic_string<CharT>& word,
	                      List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto forgotten_char_suggest(std::basic_string<CharT>& word,
	                            List_Strings<CharT>& out) const -> void;

	template <class CharT>
	auto phonetic_suggest(std::basic_string<CharT>& word,
	                      List_Strings<CharT>& out) const -> void;

      public:
	Dict_Base()
	    : Aff_Data() // we explicity do value init so content is zeroed
	{
	}
};

class Basic_Dictionary : protected Dict_Base {
      protected:
	std::locale external_locale;
	Encoding_Details enc_details;

	friend struct List_Strings_Back_Inserter;

	Basic_Dictionary(std::istream& aff, std::istream& dic)
	{
		if (!parse_aff_dic(aff, dic))
			throw std::ios_base::failure("error parsing");
		enc_details =
		    analyze_encodings(external_locale, internal_locale);
	}
	auto external_to_internal_encoding(const std::string& in,
	                                   std::wstring& wide_out,
	                                   std::string& narrow_out) const
	    -> bool;

	auto internal_to_external_encoding(std::string& in_out,
	                                   std::wstring& wide_in_out) const
	    -> bool;

      public:
	Basic_Dictionary()
	{
		// ensures the internal locale is boost locale
		set_encoding_and_language("");
		enc_details =
		    analyze_encodings(external_locale, internal_locale);
	}
	auto static load_from_aff_dic(std::istream& aff, std::istream& dic)
	{
		return Basic_Dictionary(aff, dic);
	}
	auto static load_from_aff_dic(const string& file_path_without_extension)
	{
		auto& path = file_path_without_extension;
		std::ifstream aff_file(path + ".aff");
		if (aff_file.fail())
			throw std::ios_base::failure("aff file not found");
		std::ifstream dic_file(path + ".dic");
		if (dic_file.fail())
			throw std::ios_base::failure("dic file not found");
		return load_from_aff_dic(aff_file, dic_file);
	}
	auto imbue(const std::locale& loc) -> void;
	auto spell(const std::string& word) const -> bool;
	auto suggest(const std::string& word, List_Strings<char>& out) const
	    -> void;
};

using Dictionary = Basic_Dictionary;

} // namespace nuspell
#endif // NUSPELL_DICTIONARY_HXX
