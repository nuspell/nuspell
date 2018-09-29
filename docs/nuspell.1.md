nuspell(1) -- spell checker
===========================

## SYNOPSIS


`nuspell` [-d _dict_NAME_] [-i _ENCODING_] [_FILE_]...  
`nuspell` -l|-G [-L] [-d _dict_NAME_] [-i _ENCODING_] [_FILE_]...  
`nuspell` -D|-h|--help|-v|--version


## DESCRIPTION

Nuspell checks spelling of each _FILE_.
Without _FILE_, checks standard input.

## OPTIONS

  - `-d` _di\_CT_:
    use _di\_CT_ dictionary. Only one dictionary is currently supported.
  - `-D`:
    print search paths and available dictionaries and exit
  - `-i` _ENCODING_:
    input/output encoding, default is active locale
  - `-l`:
    print only misspelled words or lines
  - `-G`:
    print only correct words or lines
  - `-L`:
    lines mode
  - `-S`:
    use Unicode text segmentation to extract words
  - `-U`:
    do not suggest, increases performance
  - `-h, --help`:
    display this help and exit
  - `-v, --version`:
    print version number and exit

## ENVIRONMENT

  - DICPATH:
    Dictionary path.
    
## RETURN VALUES

Returns error if the argument syntax is invalid or some file can not be opened.
Otherwise, spell checking has occured and returns success.
    
## BUGS

Bug reports: <https://github.com/nuspell/nuspell/issues>

## EXAMPLES

    nuspell -d en_US file.txt

## AUTHOR

## COPYRIGHT
    
## SEE ALSO

Full documentation: <https://github.com/nuspell/nuspell/wiki>

Home page: <http://nuspell.github.io/>
