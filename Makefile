CXX=g++
CXXFLAGS=-ggdb -std=c++14 -Wall -Werror -Wpedantic -pthread

CXXHDR= Scheduler.hpp \
	UVBSocket.hpp \
	uvc.hpp

CXXSRC= uvc.cpp \
	Scheduler.cpp \
	UVBSocket.cpp \
	util.cpp

CXXOBJ= $(CXXSRC:.cpp=.o)

uvc: $(CXXOBJ)
	$(CXX) $(CXXFLAGS) -o uvc $(CXXOBJ)

%.o: %.cpp $(CXXHDR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(CXXOBJ)
