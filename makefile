all:
	g++ -o cl-json-pull-test -I include/cl-json-pull -I test test/test*.cpp src/cl-json-pull.cpp
