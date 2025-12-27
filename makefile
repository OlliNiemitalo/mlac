all:: ampstatistics statistics transcode unittest libmlac-encoder.o libmlac-decoder.o

clean::
	-rm libmlac-*.o ampstatistics statistics transcode unittest
	-rm -r **/*~

ampstatistics: research/ampstatistics.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o ampstatistics research/ampstatistics.cpp -lsndfile -Isrc -g -Wall --std=c++20

statistics: test/statistics.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o statistics test/statistics.cpp -lsndfile -Isrc -g -Wall --std=c++20

transcode: test/transcode.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o transcode test/transcode.cpp -Isrc -g --std=c++20 -lsndfile -O3 -ffast-math -march=native -funroll-all-loops

unittest: test/unittest.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o unittest test/unittest.cpp -g --std=c++20 -lrt -lsndfile -Isrc -O3 -ffast-math -march=native -funroll-all-loops

libmlac-encoder.o: src/libmlac-encoder.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o libmlac-encoder.o -c -O3 -ffast-math -march=native -funroll-all-loops src/libmlac-encoder.cpp -g -std=c++20

libmlac-encoder.s: src/libmlac-encoder.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o libmlac-encoder.s -c -O3 -ffast-math -march=native -funroll-all-loops src/libmlac-encoder.cpp -std=c++20 -g -S -fverbose-asm

libmlac-decoder.o: src/libmlac-decoder.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o libmlac-decoder.o -c -O3 -ffast-math -march=native -funroll-all-loops src/libmlac-decoder.cpp -g -std=c++20

libmlac-decoder.s: src/libmlac-decoder.cpp src/mlac-core.hpp src/mlac-constants.h
	g++ -o libmlac-decoder.s -c -O3 -ffast-math -march=native -funroll-all-loops src/libmlac-decoder.cpp -std=c++20 -g -S -fverbose-asm
