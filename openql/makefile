SWIG=swig

INCS= -I. -I/data/repositories/quantRepos/openql -I/opt/lemon/include -I/usr/include/python3.5
PYTHON=python3

wrapper:
	$(SWIG) -c++ -python example.i

lib: wrapper
	g++ -fpic $(INCS) -std=c++11 -c example_wrap.cxx
	g++ -shared example_wrap.o -o _example.so

ptest: lib
	$(PYTHON) test.py

ctest: lib
	g++ $(INCS) -std=c++11 -L/data/repositories/quantRepos/openql/swig _example.so -o test test.cxx _example.so -lpython3.5m
	./test

clean:
	rm -rf *.o *.pyc *.so *~ example.py example_wrap.cxx __pycache__ test
