#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <algorithm>

using namespace std;

enum Mode {
    NORMAL,
    INSERT,
    COMMAND
};

// Buffer class - represents a single file in memory
class Buffer {
public:
    vector<string> lines;
    string filename;
    bool modified;
    int cursorX;
    int cursorY;
    
    // Undo/Redo state for this buffer
    struct BufferState {
        vector<string> lines;
        int cursorX;
        int cursorY;
    };
    
    vector<BufferState> undoStack;
    vector<BufferState> redoStack;
    const size_t MAX_UNDO_HISTORY = 100;
    
    Buffer(string fname = "untitled.txt") : 
        filename(fname), modified(false), cursorX(0), cursorY(0) {
        loadFile();
    }
    
    void loadFile() {
        ifstream file(filename);
        lines.clear();
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                lines.push_back(line);
            }
            file.close();
        }
        
        // Always ensure at least one empty line
        if (lines.empty()) {
            lines.push_back("");
        }
        
        // Reset cursor to safe position
        cursorX = 0;
        cursorY = 0;
    }
    
    bool save() {
        ofstream file(filename);
        if (file.is_open()) {
            for (size_t i = 0; i < lines.size(); i++) {
                file << lines[i];
                if (i < lines.size() - 1) file << "\n";
            }
            file.close();
            modified = false;
            return true;
        }
        return false;
    }
    
    void saveState() {
        BufferState state;
        state.lines = lines;
        state.cursorX = cursorX;
        state.cursorY = cursorY;
        
        undoStack.push_back(state);
        
        if (undoStack.size() > MAX_UNDO_HISTORY) {
            undoStack.erase(undoStack.begin());
        }
        
        redoStack.clear();
    }
    
    bool undo() {
        if (undoStack.empty()) {
            return false;
        }
        
        BufferState currentState;
        currentState.lines = lines;
        currentState.cursorX = cursorX;
        currentState.cursorY = cursorY;
        redoStack.push_back(currentState);
        
        BufferState prevState = undoStack.back();
        undoStack.pop_back();
        
        lines = prevState.lines;
        cursorX = prevState.cursorX;
        cursorY = prevState.cursorY;
        
        return true;
    }
    
    bool redo() {
        if (redoStack.empty()) {
            return false;
        }
        
        BufferState currentState;
        currentState.lines = lines;
        currentState.cursorX = cursorX;
        currentState.cursorY = cursorY;
        undoStack.push_back(currentState);
        
        BufferState nextState = redoStack.back();
        redoStack.pop_back();
        
        lines = nextState.lines;
        cursorX = nextState.cursorX;
        cursorY = nextState.cursorY;
        
        return true;
    }
};

class TextEditor {
private:
    vector<Buffer*> buffers;
    int currentBufferIndex;
    Mode currentMode;
    string commandBuffer;
    string statusMessage;
    struct termios orig_termios;
    bool syntaxHighlightEnabled;
    
    // Syntax highlighting colors
    const string COLOR_KEYWORD = "\033[38;5;205m";
    const string COLOR_STRING = "\033[38;5;180m";
    const string COLOR_COMMENT = "\033[38;5;244m";
    const string COLOR_NUMBER = "\033[38;5;141m";
    const string COLOR_RESET = "\033[0m";
    
    vector<string> cppKeywords = {
        "int", "char", "bool", "void", "string", "float", "double",
        "if", "else", "for", "while", "return", "class", "public",
        "private", "protected", "namespace", "using", "include",
        "const", "static", "virtual", "override", "template",
        "auto", "vector", "size_t", "true", "false", "nullptr"
    };

    void disableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }

    void enableRawMode() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_oflag &= ~(OPOST);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
        
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    void clearScreen() {
        write(STDOUT_FILENO, "\033[2J", 4);
        write(STDOUT_FILENO, "\033[H", 3);
    }

    bool isKeyword(const string& word) {
        for (const auto& kw : cppKeywords) {
            if (word == kw) return true;
        }
        return false;
    }
    
    string applySyntaxHighlighting(const string& line) {
        if (!syntaxHighlightEnabled) return line;
        
        string result;
        string currentWord;
        bool inString = false;
        bool inComment = false;
        char stringChar = '\0';
        
        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];
            
            if (!inString && i < line.length() - 1 && line[i] == '/' && line[i+1] == '/') {
                if (!currentWord.empty()) {
                    if (isKeyword(currentWord)) {
                        result += COLOR_KEYWORD + currentWord + COLOR_RESET;
                    } else {
                        result += currentWord;
                    }
                    currentWord.clear();
                }
                result += COLOR_COMMENT + line.substr(i) + COLOR_RESET;
                break;
            }
            
            if ((c == '"' || c == '\'') && !inComment) {
                if (!inString) {
                    if (!currentWord.empty()) {
                        if (isKeyword(currentWord)) {
                            result += COLOR_KEYWORD + currentWord + COLOR_RESET;
                        } else {
                            result += currentWord;
                        }
                        currentWord.clear();
                    }
                    inString = true;
                    stringChar = c;
                    result += COLOR_STRING;
                    result += c;
                } else if (c == stringChar && (i == 0 || line[i-1] != '\\')) {
                    inString = false;
                    result += c;
                    result += COLOR_RESET;
                } else {
                    result += c;
                }
                continue;
            }
            
            if (inString) {
                result += c;
                continue;
            }
            
            if (isdigit(c)) {
                if (!currentWord.empty()) {
                    if (isKeyword(currentWord)) {
                        result += COLOR_KEYWORD + currentWord + COLOR_RESET;
                    } else {
                        result += currentWord;
                    }
                    currentWord.clear();
                }
                string number;
                number += c;
                while (i + 1 < line.length() && (isdigit(line[i+1]) || line[i+1] == '.')) {
                    number += line[++i];
                }
                result += COLOR_NUMBER + number + COLOR_RESET;
                continue;
            }
            
            if (isalnum(c) || c == '_') {
                currentWord += c;
            } else {
                if (!currentWord.empty()) {
                    if (isKeyword(currentWord)) {
                        result += COLOR_KEYWORD + currentWord + COLOR_RESET;
                    } else {
                        result += currentWord;
                    }
                    currentWord.clear();
                }
                result += c;
            }
        }
        
        if (!currentWord.empty()) {
            if (isKeyword(currentWord)) {
                result += COLOR_KEYWORD + currentWord + COLOR_RESET;
            } else {
                result += currentWord;
            }
        }
        
        return result;
    }

    string getModeString() {
        switch(currentMode) {
            case NORMAL: return "NORMAL";
            case INSERT: return "INSERT";
            case COMMAND: return "COMMAND";
            default: return "UNKNOWN";
        }
    }
    
    Buffer* getCurrentBuffer() {
        if (buffers.empty()) return nullptr;
        return buffers[currentBufferIndex];
    }

public:
    TextEditor() : 
        currentBufferIndex(0), currentMode(NORMAL), 
        commandBuffer(""), statusMessage(""), syntaxHighlightEnabled(true) {
    }
    
    ~TextEditor() {
        for (auto buf : buffers) {
            delete buf;
        }
    }
    
    void addBuffer(string filename) {
        Buffer* newBuffer = new Buffer(filename);
        buffers.push_back(newBuffer);
        currentBufferIndex = buffers.size() - 1;
        statusMessage = "Opened: " + filename;
    }
    
    void nextBuffer() {
        if (buffers.size() > 1) {
            currentBufferIndex = (currentBufferIndex + 1) % buffers.size();
            statusMessage = "Switched to: " + getCurrentBuffer()->filename;
        }
    }
    
    void prevBuffer() {
        if (buffers.size() > 1) {
            currentBufferIndex = (currentBufferIndex - 1 + buffers.size()) % buffers.size();
            statusMessage = "Switched to: " + getCurrentBuffer()->filename;
        }
    }
    
    void closeBuffer() {
        if (buffers.size() == 1) {
            statusMessage = "Cannot close last buffer";
            return;
        }
        
        Buffer* buf = getCurrentBuffer();
        if (buf->modified) {
            statusMessage = "Unsaved changes! Use :q! to force close";
            return;
        }
        
        delete buf;
        buffers.erase(buffers.begin() + currentBufferIndex);
        
        if (currentBufferIndex >= (int)buffers.size()) {
            currentBufferIndex = buffers.size() - 1;
        }
        
        statusMessage = "Buffer closed";
    }

    void display() {
        Buffer* buf = getCurrentBuffer();
        if (!buf) return;
        
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        int termHeight = w.ws_row;
        
        write(STDOUT_FILENO, "\033[2J", 4);
        write(STDOUT_FILENO, "\033[H", 3);
        
        // Tab bar - shows all open buffers
        write(STDOUT_FILENO, "\033[44m", 5); // Blue background
        for (size_t i = 0; i < buffers.size(); i++) {
            if (i == (size_t)currentBufferIndex) {
                write(STDOUT_FILENO, "\033[1;37m", 7); // Bold white for active
            } else {
                write(STDOUT_FILENO, "\033[0;37m", 7); // Normal white
            }
            
            string tab = " " + buffers[i]->filename;
            if (buffers[i]->modified) tab += "[+]";
            tab += " ";
            write(STDOUT_FILENO, tab.c_str(), tab.length());
            write(STDOUT_FILENO, "\033[44m", 5); // Reset to blue bg
        }
        
        // Fill rest of tab bar
        string padding(80, ' ');
        write(STDOUT_FILENO, padding.c_str(), padding.length());
        write(STDOUT_FILENO, "\033[0m\r\n", 7);
        
        // Header - line 2
        string header = " Mode: " + getModeString();
        if (currentMode == NORMAL) {
            header += " | i=insert :=cmd Tab=next Shift+Tab=prev";
        } else if (currentMode == INSERT) {
            header += " | ESC=normal";
        }
        header += " | Buffers: " + to_string(buffers.size()) + " ";
        
        write(STDOUT_FILENO, "\033[7m", 4);
        write(STDOUT_FILENO, header.c_str(), header.length());
        string pad(80 - header.length(), ' ');
        write(STDOUT_FILENO, pad.c_str(), pad.length());
        write(STDOUT_FILENO, "\033[0m\r\n", 7);

        // Display lines
        for (size_t i = 0; i < buf->lines.size(); i++) {
            string displayLine = applySyntaxHighlighting(buf->lines[i]);
            write(STDOUT_FILENO, displayLine.c_str(), displayLine.length());
            write(STDOUT_FILENO, "\033[K\r\n", 5);
        }

        // Status bar
        char statusPos[20];
        snprintf(statusPos, sizeof(statusPos), "\033[%d;1H", termHeight);
        write(STDOUT_FILENO, statusPos, strlen(statusPos));
        
        write(STDOUT_FILENO, "\033[7m", 4);
        if (currentMode == COMMAND) {
            write(STDOUT_FILENO, ":", 1);
            write(STDOUT_FILENO, commandBuffer.c_str(), commandBuffer.length());
            string cmdPad(79 - commandBuffer.length(), ' ');
            write(STDOUT_FILENO, cmdPad.c_str(), cmdPad.length());
        } else if (!statusMessage.empty()) {
            write(STDOUT_FILENO, statusMessage.c_str(), statusMessage.length());
            string msgPad(80 - statusMessage.length(), ' ');
            write(STDOUT_FILENO, msgPad.c_str(), msgPad.length());
        } else {
            string pos = to_string(buf->cursorY + 1) + "," + to_string(buf->cursorX + 1);
            string posPad(80 - pos.length(), ' ');
            write(STDOUT_FILENO, posPad.c_str(), posPad.length());
            write(STDOUT_FILENO, pos.c_str(), pos.length());
        }
        write(STDOUT_FILENO, "\033[0m", 4);

        // Position cursor
        // Line 1 = tab bar, Line 2 = header, Line 3+ = content
        char cursorPos[20];
        if (currentMode == COMMAND) {
            snprintf(cursorPos, sizeof(cursorPos), "\033[%d;%dH", termHeight, (int)commandBuffer.length() + 2);
        } else {
            // cursorY is 0-indexed, display starts at line 3 (after tab bar and header)
            int displayLine = buf->cursorY + 3;
            int displayCol = buf->cursorX + 1;
            snprintf(cursorPos, sizeof(cursorPos), "\033[%d;%dH", displayLine, displayCol);
        }
        write(STDOUT_FILENO, cursorPos, strlen(cursorPos));
    }

    void executeCommand() {
        statusMessage = "";
        Buffer* buf = getCurrentBuffer();
        
        if (commandBuffer == "w") {
            if (buf->save()) {
                statusMessage = "Saved: " + buf->filename;
            } else {
                statusMessage = "Error saving!";
            }
        } else if (commandBuffer == "q") {
            if (buf->modified) {
                statusMessage = "Unsaved changes! Use :q! or :wq";
            } else {
                closeBuffer();
                if (buffers.empty()) {
                    disableRawMode();
                    clearScreen();
                    exit(0);
                }
            }
        } else if (commandBuffer == "q!") {
            delete buf;
            buffers.erase(buffers.begin() + currentBufferIndex);
            if (currentBufferIndex >= (int)buffers.size()) {
                currentBufferIndex = buffers.size() - 1;
            }
            if (buffers.empty()) {
                disableRawMode();
                clearScreen();
                exit(0);
            }
        } else if (commandBuffer == "wq") {
            if (buf->save()) {
                closeBuffer();
                if (buffers.empty()) {
                    disableRawMode();
                    clearScreen();
                    exit(0);
                }
            }
        } else if (commandBuffer == "wa") {
            // Write all buffers
            int saved = 0;
            for (auto b : buffers) {
                if (b->modified && b->save()) {
                    saved++;
                }
            }
            statusMessage = "Saved " + to_string(saved) + " buffers";
        } else if (commandBuffer == "qa") {
            // Quit all
            bool hasModified = false;
            for (auto b : buffers) {
                if (b->modified) {
                    hasModified = true;
                    break;
                }
            }
            if (hasModified) {
                statusMessage = "Unsaved changes! Use :qa! or :wqa";
            } else {
                disableRawMode();
                clearScreen();
                exit(0);
            }
        } else if (commandBuffer == "qa!") {
            disableRawMode();
            clearScreen();
            exit(0);
        } else if (commandBuffer == "wqa") {
            for (auto b : buffers) {
                if (b->modified) {
                    b->save();
                }
            }
            disableRawMode();
            clearScreen();
            exit(0);
        } else if (commandBuffer.substr(0, 2) == "e ") {
            // Open new file: :e filename
            string filename = commandBuffer.substr(2);
            addBuffer(filename);
        } else if (commandBuffer == "bn") {
            nextBuffer();
        } else if (commandBuffer == "bp") {
            prevBuffer();
        } else if (commandBuffer == "ls") {
            // List all buffers
            statusMessage = "Buffers: ";
            for (size_t i = 0; i < buffers.size(); i++) {
                statusMessage += buffers[i]->filename;
                if (i < buffers.size() - 1) statusMessage += ", ";
            }
        } else {
            statusMessage = "Unknown: " + commandBuffer;
        }
        
        commandBuffer = "";
        currentMode = NORMAL;
    }

    void insertChar(char c) {
        Buffer* buf = getCurrentBuffer();
        if (!buf) return;
        
        // Ensure cursor is in valid position
        if (buf->cursorY >= (int)buf->lines.size()) {
            buf->cursorY = buf->lines.size() - 1;
        }
        if (buf->cursorY < 0) buf->cursorY = 0;
        if (buf->cursorX > (int)buf->lines[buf->cursorY].length()) {
            buf->cursorX = buf->lines[buf->cursorY].length();
        }
        if (buf->cursorX < 0) buf->cursorX = 0;
        
        buf->saveState();
        
        if (c == '\n' || c == '\r') {
            string rest = buf->lines[buf->cursorY].substr(buf->cursorX);
            buf->lines[buf->cursorY] = buf->lines[buf->cursorY].substr(0, buf->cursorX);
            buf->lines.insert(buf->lines.begin() + buf->cursorY + 1, rest);
            buf->cursorY++;
            buf->cursorX = 0;
        } else if (c == 127 || c == 8) {
            if (buf->cursorX > 0) {
                buf->lines[buf->cursorY].erase(buf->cursorX - 1, 1);
                buf->cursorX--;
            } else if (buf->cursorY > 0) {
                buf->cursorX = buf->lines[buf->cursorY - 1].length();
                buf->lines[buf->cursorY - 1] += buf->lines[buf->cursorY];
                buf->lines.erase(buf->lines.begin() + buf->cursorY);
                buf->cursorY--;
            }
        } else if (c >= 32 && c < 127) {
            buf->lines[buf->cursorY].insert(buf->cursorX, 1, c);
            buf->cursorX++;
        }
        buf->modified = true;
        statusMessage = "";
    }

    void handleArrowKey(char c) {
        Buffer* buf = getCurrentBuffer();
        if (!buf) return;
        
        // Ensure lines vector is not empty
        if (buf->lines.empty()) {
            buf->lines.push_back("");
            buf->cursorY = 0;
            buf->cursorX = 0;
            return;
        }
        
        if (c == 'A' && buf->cursorY > 0) {
            buf->cursorY--;
            if (buf->cursorX > (int)buf->lines[buf->cursorY].length())
                buf->cursorX = buf->lines[buf->cursorY].length();
        } else if (c == 'B' && buf->cursorY < (int)buf->lines.size() - 1) {
            buf->cursorY++;
            if (buf->cursorX > (int)buf->lines[buf->cursorY].length())
                buf->cursorX = buf->lines[buf->cursorY].length();
        } else if (c == 'C' && buf->cursorX < (int)buf->lines[buf->cursorY].length()) {
            buf->cursorX++;
        } else if (c == 'D' && buf->cursorX > 0) {
            buf->cursorX--;
        }
    }

    void run() {
        enableRawMode();
        display();

        char c;
        while (true) {
            if (read(STDIN_FILENO, &c, 1) != 1) continue;
            
            if (currentMode == COMMAND) {
                if (c == '\n' || c == '\r') {
                    executeCommand();
                } else if (c == 27) {
                    commandBuffer = "";
                    currentMode = NORMAL;
                    statusMessage = "";
                } else if (c == 127 || c == 8) {
                    if (!commandBuffer.empty()) {
                        commandBuffer.pop_back();
                    } else {
                        currentMode = NORMAL;
                    }
                } else if (c >= 32 && c < 127) {
                    commandBuffer += c;
                }
            } else if (currentMode == INSERT) {
                if (c == 27) {
                    currentMode = NORMAL;
                    statusMessage = "";
                } else {
                    insertChar(c);
                }
            } else { // NORMAL mode
                if (c == 9) { // Tab key
                    nextBuffer();
                } else if (c == 'Z') { // Shift+Tab (sent as capital Z in some terminals)
                    prevBuffer();
                } else if (c == 'i') {
                    currentMode = INSERT;
                    statusMessage = "";
                } else if (c == 'u') {
                    if (getCurrentBuffer()->undo()) {
                        statusMessage = "Undo";
                    } else {
                        statusMessage = "Nothing to undo";
                    }
                } else if (c == 'r') {
                    if (getCurrentBuffer()->redo()) {
                        statusMessage = "Redo";
                    } else {
                        statusMessage = "Nothing to redo";
                    }
                } else if (c == ':') {
                    currentMode = COMMAND;
                    commandBuffer = "";
                } else if (c == 27) {
                    char seq[3];
                    if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
                        if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                            handleArrowKey(seq[1]);
                        }
                    }
                }
            }
            
            display();
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <file1> [file2] [file3] ..." << endl;
        cout << "Example: " << argv[0] << " main.cpp header.h utils.cpp" << endl;
        return 1;
    }
    
    TextEditor editor;
    
    // Open all files from command line
    for (int i = 1; i < argc; i++) {
        editor.addBuffer(argv[i]);
    }
    
    editor.run();
    return 0;
}