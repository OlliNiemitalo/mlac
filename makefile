ampstatistics: research/ampstatistics.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o ampstatistics research/ampstatistics.cpp -lsndfile -Isrc -g -Wall --std=c++11

statistics: test/statistics.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o statistics test/statistics.cpp -lsndfile -Isrc -g -Wall --std=c++11

transcode: test/transcode.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o transcode test/transcode.cpp -Isrc --std=c++11 -lsndfile -O3 -ffast-math -march=native -funroll-all-loops

unittest: test/unittest.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o unittest test/unittest.cpp --std=c++11 -lrt -lsndfile -Isrc -O3 -ffast-math -march=native -funroll-all-loops

libmolo-encoder.o: src/libmolo-encoder.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o libmolo-encoder.o -c -O3 -ffast-math src/libmolo-encoder.cpp -std=c++11

libmolo-encoder.s: src/libmolo-encoder.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o libmolo-encoder.s -c -O3 -ffast-math src/libmolo-encoder.cpp -std=c++11 -g -S -fverbose-asm

libmolo-decoder.o: src/libmolo-decoder.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o libmolo-decoder.o -c -O3 -ffast-math src/libmolo-decoder.cpp -std=c++11

libmolo-decoder.s: src/libmolo-decoder.cpp src/molo-core.hpp src/molo-constants.h
	g++ -o libmolo-decoder.s -c -O3 -ffast-math src/libmolo-decoder.cpp -std=c++11 -g -S -fverbose-asm
