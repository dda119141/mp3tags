set pagination off
set logging file gdb.txt
set logging on
file out/writeTag
#b readTag.cpp:25
#b tagreader.hpp:18
b tagwriter.hpp:60
b id3v2_common.hpp:221
b id3v1.hpp:168
b id3v1.hpp:206
b ape.hpp:166
b ape.hpp:413
b ape.hpp:447
#run -p -d files/test1.mp3
run -g "Rap" -d files/testToFix.mp3
