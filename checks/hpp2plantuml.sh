#sudo pip3 install hpp2plantuml
#FIXME hpp2plantuml -i "../src/nuspell/*.hxx" -o nuspell.puml
hpp2plantuml \
-i ../src/nuspell/condition.hxx \
-i ../src/nuspell/finder.hxx \
-i ../src/nuspell/string_utils.hxx \
-i ../src/nuspell/aff_data.hxx \
-o hpp2plantuml.puml
#FIXME-i ../src/nuspell/structures.hxx \
#FIXME-i ../src/nuspell/locale_utils.hxx \
#FIXME-i ../src/nuspell/dictionary.hxx \
plantuml -tsvg hpp2plantuml.puml
plantuml -tpng hpp2plantuml.puml
