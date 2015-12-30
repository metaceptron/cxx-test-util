all:
	clang++ example.cxx tools/*.cxx -o example.bin -std=c++11 -Wall -Wextra -pipe \
		-I. -lboost_program_options -O2
