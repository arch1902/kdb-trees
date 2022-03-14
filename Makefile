sampleobjects = buffer_manager.o file_manager.o sample_run.o
kdbobjects = buffer_manager.o file_manager.o kdbtree.o

kdbtree : $(kdbobjects)
	     g++ -std=c++11 -o kdbtree $(kdbobjects)

sample_run : $(sampleobjects)
	     g++ -std=c++11 -o sample_run $(sampleobjects)

kdbtree.o : kdbtree.cpp
	g++ -std=c++11 -c kdbtree.cpp

sample_run.o : sample_run.cpp
	g++ -std=c++11 -c sample_run.cpp

buffer_manager.o : buffer_manager.cpp
	g++ -std=c++11 -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ -std=c++11 -c file_manager.cpp

clean :
	rm -f *.o
	rm -f sample_run
