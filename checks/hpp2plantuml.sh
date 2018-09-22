#!/usr/bin/env sh

# description: generates UML class diagram from header files
# license: https://github.com/hunspell/nuspell/blob/master/LICENSES
# author: Sander van Geloven

hpp2plantuml -i "../src/nuspell/*.hxx" -o hpp2plantuml.puml
sed -i -e 's/@startuml/@startuml\ntitle Nuspell/' hpp2plantuml.puml
if [ -e ../../misc-nuspell/uml/plantuml/skin.iuml ]; then
    sed -i -e 's/@startuml/@startuml\n!include ..\/..\/misc-nuspell\/uml\/plantuml\/skin.iuml/' hpp2plantuml.puml
fi
plantuml -tpng hpp2plantuml.puml
plantuml -tsvg hpp2plantuml.puml
