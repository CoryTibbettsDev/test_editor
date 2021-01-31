editor: editor.o
	$(CXX) editor.o -o editor -Wall -Wextra -pedantic

editor.o: editor.cpp
	$(CXX) -c editor.cpp

clean:
	rm *.o editor
