#!/bin/sh
sql:
	g++ mysql_test.cpp -o testmysql -I ../mysql/include -L ../mysql/lib/ -lmysqlclient 
memc:
	g++ memcache_test.cpp -o testmemcache -lmemcached -std=c++0x
