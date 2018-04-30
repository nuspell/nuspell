nuspell(1) -- spell checker
===========================

## SYNOPSIS

```
nuspell [-d dict_NAME] [-i enc] [file_name]...
nuspell -l|-G [-L] [-d dict_NAME] [-i enc] [file_name]...
nuspell -D|-h|--help|-v|--version
```

## DESCRIPTION

Check spelling of each FILE. Without FILE, check standard 
input.

## OPTIONS

  - `-d <di_CT>`:
    use di_CT dictionary. Only one dictionary is currently supported.
  - `-D`:
    show available dictionaries and exit
  - `-i <enc>`:
    input encoding, default is active locale
  - `-l`:
    print only misspelled words or lines
  - `-G`:
    print only correct words or lines
  - `-L`:
    lines mode
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

Bug reports: <https://github.com/hunspell/nuspell/issues>

## EXAMPLES

    nuspell -d en_US file.txt

## AUTHOR

## COPYRIGHT
    
## SEE ALSO

Full documentation: <https://github.com/hunspell/hunspell/wiki>

Home page: <http://hunspell.github.io/>
