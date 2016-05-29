CXX=g++
CXXFLAGS=-ggdb -std=c++11 -Wall -Werror -pthread

CXXSRC= uvc.cpp \
	sched.cpp \
	socket.cpp \
	util.cpp

CXXOBJ= $(CXXSRC:.cpp=.o)

uvc: $(CXXOBJ)
	$(CXX) $(CXXFLAGS) -o uvc $(CXXOBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(CXXOBJ)
