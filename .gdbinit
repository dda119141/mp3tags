set pagination off
set logging file gdb.txt
set logging on
file out/writeTag
#b readTag.cpp:25
#b tagreader.hpp:18
#b id3v1.hpp:175
b tagwriter.hpp:61
b id3v2_common.hpp:221
b id3v2_common.hpp:241
#run -p -d files/test1.mp3
run -g "Rap" -d files/id3v2_test1.mp3
