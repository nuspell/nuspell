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

#include "dictionary.hxx"
#include "utils.hxx"

#include <fstream>
#include <sstream>

/**
 * @mainpage
 *
 * You should be looking at:
 *
 * - Class nuspell::Dictionary
 * - Exception class nuspell::Dictionary_Loading_Error
 * - Auxiliary free functions for locating dictionaries on a file-system, see
 *   finder.hxx
 */

using namespace std;

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

Dictionary::Dictionary(std::istream& aff, std::istream& dic)
{
	load_aff_dic(aff, dic);
}

Dictionary::Dictionary() = default;

/**
 * @brief Load the dictionary from opened files as iostreams
 * @pre Before calling this the dictionary object must be empty e.g
 *      default-constructed or assigned with another empty object.
 * @param aff The iostream of the .aff file
 * @param dic The iostream of the .dic file
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_aff_dic(std::istream& aff, std::istream& dic) -> void
{
	auto err_msg = ostringstream();
	if (!parse_aff_dic(aff, dic, err_msg))
		throw Dictionary_Loading_Error(std::move(err_msg).str());
}

static auto open_aff_dic(const filesystem::path& aff_path)
    -> pair<ifstream, ifstream>
{
	auto aff_file = ifstream(aff_path);
	if (aff_file.fail()) {
		auto err = "Aff file " + aff_path.string() + " not found.";
		throw Dictionary_Loading_Error(err);
	}
	auto dic_path = aff_path;
	dic_path.replace_extension(".dic");
	auto dic_file = ifstream(dic_path);
	if (dic_file.fail()) {
		auto err = "Dic file " + dic_path.string() + " not found.";
		throw Dictionary_Loading_Error(err);
	}
	return {std::move(aff_file), std::move(dic_file)};
}

/**
 * @brief Load the dictionary from file on filesystem
 * @pre Before calling this the dictionary object must be empty e.g
 *      default-constructed or assigned with another empty object.
 * @param aff_path path to .aff file. The path of .dic is inffered from this.
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_aff_dic(const std::filesystem::path& aff_path) -> void
{
	auto [aff, dic] = open_aff_dic(aff_path);
	load_aff_dic(aff, dic);
}

auto Dictionary::load_aff_dic_internal(const std::filesystem::path& aff_path,
                                       std::ostream& err_msg) -> void
{
	auto [aff, dic] = open_aff_dic(aff_path);
	if (!parse_aff_dic(aff, dic, err_msg))
		throw Dictionary_Loading_Error("Parsing error.");
}

/**
 * @brief Create a dictionary from opened files as iostreams
 *
 * Prefer using load_from_path(). Use this if you have a specific use case,
 * like when .aff and .dic are in-memory buffers istringstream.
 *
 * @deprecated Use new non-static member function
 *             load_aff_dic(std::istream&,std::istream&).
 * @param aff The iostream of the .aff file
 * @param dic The iostream of the .dic file
 * @return Dictionary object
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_from_aff_dic(std::istream& aff, std::istream& dic)
    -> Dictionary
{
	auto d = Dictionary();
	d.load_aff_dic(aff, dic);
	return d;
}

/**
 * @brief Create a dictionary from files
 * @deprecated Use new non-static member function
 * load_aff_dic(const std::filesystem::path&). Note that the new function takes
 * path **WITH .aff extension**.
 * @param file_path_without_extension path *without* extensions (without .dic or
 * .aff)
 * @return Dictionary object
 * @throws Dictionary_Loading_Error on error
 */
auto Dictionary::load_from_path(const std::string& file_path_without_extension)
    -> Dictionary
{
	auto d = Dictionary();
	d.load_aff_dic(file_path_without_extension + ".aff");
	return d;
}

/**
 * @brief Checks if a given word is correct
 * @param word any word
 * @return true if correct, false otherwise
 */
auto Dictionary::spell(std::string_view word) const -> bool
{
	auto ok_enc = validate_utf8(word);
	if (unlikely(word.size() > 360))
		return false;
	if (unlikely(!ok_enc))
		return false;
	auto word_buf = string(word);
	return spell_priv(word_buf);
}

/**
 * @brief Suggests correct words for a given incorrect word
 * @param[in] word incorrect word
 * @param[out] out this object will be populated with the suggestions
 */
auto Dictionary::suggest(std::string_view word,
                         std::vector<std::string>& out) const -> void
{
	out.clear();
	auto ok_enc = validate_utf8(word);
	if (unlikely(word.size() > 360))
		return;
	if (unlikely(!ok_enc))
		return;
	suggest_priv(word, out);
}
NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
