nuspell(5) -- format of spell checker dictionaries and affix files
==================================================================

## DESCRIPTION

Nuspell(5)  Nuspell needs at least two files to do spell checking for a
certain language.  One is a dictionary file containing words with optional
flags.  The other is an affix file containing some general settings and
definitions on how the flags affect the spell checking.  An optional file
is the personal dictionary file.  These file formats are described below.

## PERSONAL DICTIONARY FILE

.

## DICTIONARY FILE

.

Note: do not use a slash character `/` at the end of a word when that
word does not have any flags. This interferes with the parsing of the flags
and results in an error.

## AFFIX FILE

.

## MORPHOLOGICAL ANALYSIS

.

Note: in the dictionary file, do not use a slash character `/` in the values
of the morphological fields. This interferes with the parsing of the flags
and results in error.

## BUGS

Bug reports: <https://github.com/nuspell/nuspell/issues>

## SEE ALSO

nuspell(1), hunspell(1), hunspell(5), ispell(1), ispell(5)

Full documentation: <https://github.com/nuspell/nuspell/wiki>

Home page: <http://nuspell.github.io/>
