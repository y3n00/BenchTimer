cc := g++
src := *.cpp
exe := -o Timer
libs := -lfmt
flags := -std=c++2b $(libs)

release:
	$(cc) $(src) $(exe) $(flags) -O3
debug:
	$(cc) $(src) $(exe)_dbg $(flags) -Og
