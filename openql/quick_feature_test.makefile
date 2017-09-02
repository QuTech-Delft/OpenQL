# This makefile can be used to quickly test a feature under development
# using quick_feature_test.cc or quick_feature_test.py file as test.
# this can be done by:
#	make -f quick_feature_test.makefile clean wrapper lib ctest
#
# This requires compilation and installation of lemon library
# and setting up LEMON_ROOT variable accordingly

SWIG=swig
LEMON_ROOT=/opt/lemon
INCS=-I .. -I . -I $(LEMON_ROOT)/include
CFLAGS=-fpic $(INCS) -std=c++11 -Wfatal-errors `python3-config --cflags`
LDFLAGS=`python3-config --ldflags`

PYTHON=python3

wrapper:
	$(SWIG) -c++ -python -o openql_wrap.cxx openql.i

lib:
	g++ $(CFLAGS) -c openql_wrap.cxx
	g++ -shared openql_wrap.o -o _openql.so

ptest:
	$(PYTHON) quick_feature_test.py

ctest:
	mkdir -p output
	g++ $(INCS) -std=c++11 -o quick_feature_test quick_feature_test.cc _openql.so $(LDFLAGS)
	./quick_feature_test

clean:
	rm -rf *.o *.pyc *.so *~ openql.py openql_wrap.cxx __pycache__ quick_feature_test output
