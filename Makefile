CXX = c++
CXXFLAGS = -I/opt/homebrew/include -std=c++11
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL

graphics: graphics.cpp
	$(CXX) $(CXXFLAGS) graphics.cpp -o graphics $(LDFLAGS)

terminal: terminal_glfw.cpp
	$(CXX) $(CXXFLAGS) terminal_glfw.cpp -o terminal_glfw $(LDFLAGS)

clean:
	rm -f graphics terminal_glfw

.PHONY: clean

