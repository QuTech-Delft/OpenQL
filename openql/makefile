SWIG=swig

INCS= -I. -I/data/repositories/quantRepos/openql -I/opt/lemon/include -I/usr/include/python3.5
PYTHON=python3

wrapper:
	$(SWIG) -c++ -python openql.i

lib: wrapper
	g++ -fpic $(INCS) -std=c++11 -c openql_wrap.cxx
	g++ -shared openql_wrap.o -o _openql.so

ptest: lib
	$(PYTHON) test.py

ctest: lib
	g++ $(INCS) -std=c++11 -L/data/repositories/quantRepos/openql/swig _openql.so -o test test.cc _openql.so -lpython3.5m
	./test

clean:
	rm -rf *.o *.pyc *.so *~ openql.py openql_wrap.cxx __pycache__ test
