CPPFLAGS=-std=c++11

wrapper : wrapper.cpp
	g++ ${CPPFLAGS} $< -o $@

.PHONY : clean
clean :
	rm -rf wrapper *.o
