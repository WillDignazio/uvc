CXX=clang++
CXXFLAGS=-O3 -std=c++14 -Wall -Werror -Wpedantic -pthread

CXXHDR= Scheduler.hpp \
	UVBSocket.hpp \
	UVBSocketSpawner.hpp \
	uvc.hpp

CXXSRC= uvc.cpp \
	Scheduler.cpp \
	UVBSocket.cpp \
	UVBSocketSpawner.cpp \
	util.cpp

CXXOBJ= $(CXXSRC:.cpp=.o)

uvc: $(CXXOBJ)
	$(CXX) $(CXXFLAGS) -o uvc $(CXXOBJ)

%.o: %.cpp $(CXXHDR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(CXXOBJ)
