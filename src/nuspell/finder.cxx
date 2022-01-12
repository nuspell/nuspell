/* Copyright 2016-2022 Dimitrij Mijoski
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

#include "finder.hxx"
#include "utils.hxx"

#include <algorithm>
#include <filesystem>
#include <set>

#if __has_include(<unistd.h>)
#include <unistd.h> // defines _POSIX_VERSION
#endif
#if _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace std;

namespace nuspell {
inline namespace v5 {

namespace fs = std::filesystem;

/**
 * @brief Append the paths of the default directories to be searched for
 * dictionaries.
 * @param paths vector of directory paths to append to
 */
auto append_default_dir_paths(std::vector<string>& paths) -> void
{
#ifdef _WIN32
	const auto PATHSEP = ';';
#else
	const auto PATHSEP = ':';
#endif
	auto dicpath = getenv("DICPATH");
	if (dicpath && *dicpath)
		split(dicpath, PATHSEP, paths);

#ifdef _POSIX_VERSION
	auto home = getenv("HOME");
	auto xdg_data_home = getenv("XDG_DATA_HOME");
	if (xdg_data_home && *xdg_data_home)
		paths.push_back(xdg_data_home + string("/hunspell"));
	else if (home)
		paths.push_back(home + string("/.local/share/hunspell"));

	auto xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs && *xdg_data_dirs) {
		auto data_dirs = string_view(xdg_data_dirs);

		auto i = paths.size();
		split(data_dirs, PATHSEP, paths);
		for (; i != paths.size(); ++i)
			paths[i] += "/hunspell";

		i = paths.size();
		split(data_dirs, PATHSEP, paths);
		for (; i != paths.size(); ++i)
			paths[i] += "/myspell";
	}
	else {
		paths.push_back("/usr/local/share/hunspell");
		paths.push_back("/usr/share/hunspell");
		paths.push_back("/usr/local/share/myspell");
		paths.push_back("/usr/share/myspell");
	}
#if defined(__APPLE__) && defined(__MACH__)
	auto osx = string("/Library/Spelling");
	if (home) {
		paths.push_back(home + osx);
	}
	paths.push_back(osx);
#endif
#endif
#ifdef _WIN32
	auto winpaths = {getenv("LOCALAPPDATA"), getenv("PROGRAMDATA")};
	for (auto& p : winpaths) {
		if (p) {
			paths.push_back(string(p) + "\\hunspell");
		}
	}
#endif
}

auto static append_lo_global(const fs::path& p, vector<string>& paths) -> void
{
	for (auto& de : fs::directory_iterator(p)) {
		if (!de.is_directory())
			continue;
		if (!begins_with(de.path().filename().string(), "dict-"))
			continue;
		paths.push_back(de.path().string());
	}
}

auto static append_lo_user(const fs::directory_entry& de1,
                           vector<string>& paths) -> void
{
	for (auto& de2 : fs::directory_iterator(de1)) {
		try {
			if (!de2.is_directory())
				continue;
			if (de2.path().extension() != ".oxt")
				continue;
			for (auto& de3 : fs::directory_iterator(de2)) {
				if (de3.is_directory() &&
				    begins_with(de3.path().filename().string(),
				                "dict")) {
					paths.push_back(de3.path().string());
				}
				else if (de3.is_regular_file() &&
				         de3.path().extension() == ".aff") {
					paths.push_back(de2.path().string());
					break;
				}
			}
		}
		catch (const fs::filesystem_error&) {
		}
	}
}

/**
 * @brief Append the paths of the LibreOffice's directories to be searched for
 * dictionaries.
 *
 * @warning This function shall not be called from LibreOffice or modules that
 * may end up being used by LibreOffice. It is mainly intended to be used by
 * the CLI tool.
 *
 * @param paths vector of directory paths to append to
 */
auto append_libreoffice_dir_paths(std::vector<std::string>& paths) -> void
{
	using namespace filesystem;
	auto p1 = path();
	// add LibreOffice global paths
	try {
#if _WIN32
		auto lo_ins_dir_sz = DWORD(260);
		auto lo_install_dir = string(lo_ins_dir_sz - 1, '*');
		auto subkey = "SOFTWARE\\LibreOffice\\UNO\\InstallPath";
		auto regerr = RegGetValueA(
		    HKEY_LOCAL_MACHINE, subkey, nullptr, RRF_RT_REG_SZ, nullptr,
		    data(lo_install_dir), &lo_ins_dir_sz);
		lo_install_dir.resize(lo_ins_dir_sz - 1);
		if (regerr == ERROR_MORE_DATA) {
			regerr = RegGetValueA(
			    HKEY_LOCAL_MACHINE, subkey, nullptr, RRF_RT_REG_SZ,
			    nullptr, data(lo_install_dir), &lo_ins_dir_sz);
		}
		if (regerr == ERROR_SUCCESS) {
			p1 = lo_install_dir;
			p1.replace_filename("share\\extensions");
			append_lo_global(p1, paths);
		}
#elif defined(__APPLE__) && defined(__MACH__)
		p1 = "/Applications/LibreOffice.app/Contents/Resources/"
		     "extensions";
		append_lo_global(p1, paths);
#elif _POSIX_VERSION
		p1 = "/opt";
		for (auto& de1 : directory_iterator(p1)) {
			try {
				if (!de1.is_directory())
					continue;
				if (!begins_with(de1.path().filename().string(),
				                 "libreoffice "))
					continue;
				append_lo_global(
				    de1.path() / "share/extensions", paths);
			}
			catch (const filesystem_error&) {
			}
		}
#endif
	}
	catch (const filesystem_error&) {
	}

	// add LibreOffice user paths
	p1.clear();
	try {
#if _WIN32
		auto appdata = getenv("APPDATA");
		if (appdata == nullptr)
			return;
		p1 = appdata;
#elif _POSIX_VERSION
		auto home = getenv("HOME");
		if (home == nullptr)
			return;
		p1 = home;
#if defined(__APPLE__) && defined(__MACH__)
		p1 /= "Library/Application Support";
#else
		p1 /= ".config";
#endif
#else
		return;
#endif
		p1 /= "libreoffice/4/user/uno_packages/cache/uno_packages";
		p1.make_preferred();
		for (auto& de1 : directory_iterator(p1)) {
			try {
				if (de1.is_directory())
					append_lo_user(de1, paths);
			}
			catch (const filesystem_error&) {
			}
		}
	}
	catch (const filesystem_error&) {
	}
}

/**
 * @brief Search a directory for dictionaries.
 *
 * This function searches the directory for files that represent a dictionary
 * and for each one found it appends the pair of dictionary name and filepath to
 * dictionary, both without the filename extension (.aff or .dic).
 *
 * For example for the files /dict/dir/en_US.dic and /dict/dir/en_US.aff the
 * following pair will be appended ("en_US", "/dict/dir/en_US").
 *
 * @todo At some point this API should be made to be more strongly typed.
 * Instead of using that pair of strings to represent the dictionary files, a
 * new class should be created with three public functions, getters, that would
 * return the name, the path to the .aff file (with filename extension to avoid
 * confusions) and the path to the .dic file. The C++ 17 std::filesystem::path
 * should probably be used. It is unspecified to the public what this class
 * holds privately, but it should probably hold only one path to the aff file.
 * For the directory paths, it is simple, just use the type
 * std::filesystem::path. When this API is created, the same function names
 * should be used, added as overloads. The old API should be marked as
 * deprecated. This should be done when we start requiring GCC 9 which supports
 * C++ 17 filesystem out of the box. GCC 8 has this too, but it is somewhat
 * experimental and requires manually linking to additional static library.
 *
 * @param dir_path path to directory
 * @param dict_list vector to append the found dictionaries to
 */
auto search_dir_for_dicts(const string& dir_path,
                          vector<pair<string, string>>& dict_list) -> void
try {
	using namespace filesystem;
	auto dp = path(dir_path);
	auto dics = set<path>();
	auto fp = path();
	for (auto& entry : directory_iterator(dp)) {
		fp = entry.path();
		auto name = fp.filename();
		auto ext = fp.extension();
		if (ext == ".dic") {
			dics.insert(name);
			name.replace_extension(".aff");
		}
		else if (ext == ".aff") {
			dics.insert(name);
			name.replace_extension(".dic");
		}
		else {
			continue;
		}
		if (dics.count(name)) {
			name.replace_extension();
			fp.replace_extension();
			dict_list.emplace_back(name.string(), fp.string());
		}
	}
}
catch (const fs::filesystem_error&) {
}

/**
 * @brief Search the directories for dictionaries.
 *
 * @see search_dir_for_dicts()
 *
 * @param dir_paths list of paths to directories
 * @param dict_list vector to append the found dictionaries to
 */
auto search_dirs_for_dicts(const std::vector<string>& dir_paths,
                           std::vector<std::pair<string, string>>& dict_list)
    -> void
{
	for (auto& p : dir_paths)
		search_dir_for_dicts(p, dict_list);
}

/**
 * @brief Search the default directories for dictionaries.
 *
 * @see append_default_dir_paths()
 * @see search_dirs_for_dicts()
 *
 * @param dict_list vector to append the found dictionaries to
 */
auto search_default_dirs_for_dicts(
    std::vector<std::pair<std::string, std::string>>& dict_list) -> void
{
	auto dir_paths = vector<string>();
	append_default_dir_paths(dir_paths);
	search_dirs_for_dicts(dir_paths, dict_list);
}

/**
 * @brief Find dictionary path given the name.
 *
 * Find the first dictionary whose name matches @p dict_name.
 *
 * @param dict_list vector of pairs with name and paths
 * @param dict_name dictionary name
 * @return iterator of @p dict_list that points to the found dictionary or end
 * if not found.
 */
auto find_dictionary(
    const std::vector<std::pair<std::string, std::string>>& dict_list,
    const std::string& dict_name)
    -> std::vector<std::pair<std::string, std::string>>::const_iterator
{
	return find_if(begin(dict_list), end(dict_list),
	               [&](auto& e) { return e.first == dict_name; });
}

Dict_Finder_For_CLI_Tool::Dict_Finder_For_CLI_Tool()
{
	append_default_dir_paths(dir_paths);
	append_libreoffice_dir_paths(dir_paths);
	dir_paths.push_back(".");
	search_dirs_for_dicts(dir_paths, dict_multimap);
	stable_sort(begin(dict_multimap), end(dict_multimap),
	            [](auto& a, auto& b) { return a.first < b.first; });
}

/**
 * @internal
 * @brief Gets the dictionary path.
 *
 * If path is given (contains slash) it returns the input argument,
 * otherwise searches the found dictionaries by their name and returns their
 * path.
 *
 * @param dict name or path of dictionary without the trailing .aff/.dic.
 * @return the path to dictionary or empty if does not exists.
 */
auto Dict_Finder_For_CLI_Tool::get_dictionary_path(
    const std::string& dict) const -> std::string
{
#ifdef _WIN32
	const auto SEPARATORS = "\\/";
#else
	const auto SEPARATORS = '/';
#endif
	// first check if it is a path
	if (dict.find_first_of(SEPARATORS) != dict.npos) {
		// a path
		return dict;
	}
	else {
		// search list
		auto x = find_dictionary(dict_multimap, dict);
		if (x != end(dict_multimap))
			return x->second;
	}
	return {};
}
} // namespace v5
} // namespace nuspell
