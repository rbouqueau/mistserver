g++ -funsigned-char -DOUTPUTTYPE="\"output_progressive_mp4.h\"" -DTS_BASECLASS=Output -DSHM_ENABLED=1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DDEBUG=4 -DPACKAGE_VERSION="\"2.3-27-g3ff1ba6\"" -DNOT_MAIN -DINPUTTYPE="\"input_mp3.h\"" -I../../.. main.cpp ../../../src/io.cpp ../../../src/input/input.cpp ../../../src/input/input_mp3.cpp ../../../src/input/mist_in.cpp ../../../src/output/mist_out.cpp ../../../src/output/output.cpp ../../../src/output/output_http.cpp  ../../../src/output/output_progressive_mp4.cpp -o mp32mp4 -L../../msys2 -lmist -lws2_32
