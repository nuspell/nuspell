cd $(dirname "$0")
clang-format -style=file -i src/nuspell/*.[ch]xx tests/*.[ch]xx
