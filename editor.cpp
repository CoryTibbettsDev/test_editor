#include <iostream>
#include <termios.h>
#include <unistd.h>
// #include <stdio.h> // <cstdio>
// #include <stdlib.h> // <cstdlib>
#include <fstream>
#include <string>
#include <vector>

std::vector<std::string> lineVector;
std::vector<std::string>::iterator it;

const char *currentFile;

struct Cursor {
	int x = 0;
	int y = 0;
};
Cursor cursor;

// IMPORTANT define allows us to easily detect control key presses
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	BACKSPACE = 127,
};

struct termios original_termios;
void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) {
		throw;
	}
}
void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
		throw;
	}
	atexit(disableRawMode);

	struct termios raw = original_termios;
	// https://man7.org/linux/man-pages/man3/termios.3.html
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		throw;
	}
}


void readFile(const char *fileName) {
	std::ifstream file(fileName);
	std::string line;
	if (file.is_open()) {
		while (getline(file, line)) {
			lineVector.push_back(line);
		}
		file.close();
	} else {
		std::cout << "File did not open" << std::flush;
	}
}
void saveFile(const char *fileName) {
	std::ofstream file(fileName);
	if (file.is_open()) {
		for (int i = 0; i < lineVector.size(); i++) {
			file << lineVector[i] << '\n';
		}
		file.close();
	} else {
		std::cout << "Could not write to file" << std::flush;
	}
}

void moveCursor(int key) {
	switch (key) {
		case ARROW_LEFT:
			if (cursor.x != 0) {
				cursor.x--;
			} else if (cursor.y > 0) {
				cursor.y--;
				cursor.x = lineVector[cursor.y].size();
			}
			break;
		case ARROW_RIGHT:
			if (cursor.x < lineVector[cursor.y].size()) {
				cursor.x++;
			} else if (cursor.x >= lineVector[cursor.y].size() &&
			cursor.y + 1 < lineVector.size()) {
				cursor.y++;
				cursor.x = 0;
			}
			break;
		case ARROW_UP:
			if (cursor.y != 0) {
				cursor.y--;
				if (cursor.x > lineVector[cursor.y].size()) {
					cursor.x = lineVector[cursor.y].size();
				}
			}
			break;
		case ARROW_DOWN:
			if (cursor.y + 1 < lineVector.size()) {
				cursor.y++;
			}
			break;
	}
}

int input() {
	char c;
	std::cin.get(c);
	// Needs to come before other stuff so there are not leftover chars in
	// the input stream from multi byte keypress
	if (c == '\x1b') {
		char seq[3];
		std::cin.get(seq[0]);
		std::cin.get(seq[1]);
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				std::cin.get(seq[2]);
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}
void processInput() {
	int c = input();
	switch (c) {
		case '\r':
			// void insert (iterator position, size_type n, const value_type& val);
			it = lineVector.begin();
			for (int i = 0; i < cursor.y + 1; i++) {
				it++;
			}
			lineVector.insert(it, 1, "");
			cursor.y++;
			cursor.x = 0;
			break;
		case CTRL_KEY('q'):
			std::cout << "\x1b[2J\x1b[H" << std::flush;
			exit(0);
			break;
		case CTRL_KEY('s'):
			saveFile(currentFile);
			break;
		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_UP:
		case ARROW_DOWN:
			moveCursor(c);
			break;
		case DEL_KEY:
		case HOME_KEY:
		case END_KEY:
		case PAGE_UP:
		case PAGE_DOWN:
			break;
		case BACKSPACE:
			if (cursor.x != 0) {
				lineVector[cursor.y].erase(cursor.x - 1, 1);
				cursor.x--;
			}
			break;
		default:
			if (cursor.x > lineVector[cursor.y].size()) {
				lineVector[cursor.y].push_back(c);
				cursor.x++;
			} else {
				// insert(positon, # of char, char)
				lineVector[cursor.y].insert(cursor.x, 1, c);
				if (c == '\t') {
					cursor.x = cursor.x + 5;
				} else {
					cursor.x++;
				}
			}
	}
}

void drawText() {
	std::cout << "\x1b[2J\x1b[H" << std::flush;
	for (int i = 0; i < lineVector.size(); i++) {
		std::cout << lineVector[i] << "\r\n" << std::flush;
	}
	std::cout << "\x1b[" << cursor.y + 1 << ";" << cursor.x + 1 << "H" << std::flush;
}

int main(int argc, char const *argv[]) {
	if (argc > 1) {
		currentFile = argv[1];
		readFile(argv[1]);
	} else {
		std::cout << "Enter filename to write to:" << '\n';

		std::string givenFilename;
		std::getline(std::cin, givenFilename);

		const char *f = givenFilename.c_str();

		currentFile = f;
		lineVector.push_back("");
	}

	enableRawMode();

	std::cout << "\x1b[2J\x1b[H" << std::flush;

	drawText();
	while (1) {
		processInput();
		drawText();
	}
	return 0;
}
