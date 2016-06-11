CXX=g++
CXXFLAGS=-ggdb -std=c++11 -Wall -Werror -pthread

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
