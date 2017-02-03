cc=g++
libs=
incs=-I.
cflags=-O2 -std=c++11 #-Wall
opts=-D ql_optimize

programs=$(wildcard ./programs/*.cc)
examples=$(patsubst %.cc,%,$(programs))

all: $(examples)

# compile examples

%: %.cc
	$(cc) -o $@ $< $(cflags) $(libs) $(incs) $(opts)

clean:
	rm -f programs/output/* $(examples)
