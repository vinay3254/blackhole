CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall -fopenmp

all: verify_physics render_test

verify_physics: GeodesicTable.cpp verify_physics.cpp
	$(CXX) $(CXXFLAGS) GeodesicTable.cpp verify_physics.cpp -o verify_physics

render_test: BlackHoleRenderer.cpp render_test.cpp
	$(CXX) $(CXXFLAGS) BlackHoleRenderer.cpp render_test.cpp -o render_test

clean:
	rm -f verify_physics verify_physics.exe render_test render_test.exe blackhole.bmp
