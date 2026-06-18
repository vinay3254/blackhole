CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall

all: verify_physics

verify_physics: GeodesicTable.cpp verify_physics.cpp
	$(CXX) $(CXXFLAGS) GeodesicTable.cpp verify_physics.cpp -o verify_physics

clean:
	rm -f verify_physics verify_physics.exe
