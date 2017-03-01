LEMON=/opt/lemon
cc=g++
libs= -L $(LEMON)/lib
incs=-I. -I $(LEMON)/include
cflags=-O2 -std=c++11 -Wall -Wfatal-errors
# opts=-D ql_optimize

programs=$(wildcard ./programs/*.cc)
examples=$(patsubst %.cc,%,$(programs))

test_programs=$(wildcard ./tests/*.cc)
tests=$(patsubst %.cc,%,$(test_programs))

all: $(examples) $(tests)

# compile examples

%: %.cc
	$(cc) -o $@ $< $(cflags) $(libs) $(incs) $(opts)

run:
	# ./programs/circuit7
	./tests/t_8_32

clean:
	rm -f 	programs/output/* $(examples) $(tests) *~ *.dot *.dat \
			scheduled*.qc
