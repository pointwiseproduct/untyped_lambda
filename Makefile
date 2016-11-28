.PHONY: all

all:
	g++ -std=c++11 untyped_lambda.cpp -lboost_system -lboost_filesystem -O2 -o untyped_lambda
