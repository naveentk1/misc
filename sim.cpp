#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <string>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std;

// Simulation parameters
const int WIDTH = 120;
const int HEIGHT = 35;
const double FIRE_INTENSITY = 0.85;
const double WIND_STRENGTH = 0.3;
const int MAX_EMBERS = 800;

// ANSI Color codes
namespace Color {
    const string WHITE = "\033[97m";
    const string YELLOW = "\033[93m";
    const string ORANGE = "\033[33m";
    const string RED = "\033[91m";
    const string DARK_RED = "\033[31m";
    const string GRAY = "\033[90m";
    const string BLACK = "\033[30m";
    const string RESET = "\033[0m";
    const string BOLD = "\033[1m";
}

// Random float between min and max
double randomFloat(double min, double max) {
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

// Random int between min and max
int randomInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

class Ember {
public:
    double x, y;
    double vx, vy;
    double heat;
    double life;
    double flicker;
    
    Ember(double x, double y, double heat = 1.0) 
        : x(x), y(y), heat(heat) {
        vx = randomFloat(-0.5, 0.5);
        vy = randomFloat(-2.0, -4.0);
        life = randomFloat(0.8, 1.0);
        flicker = randomFloat(0.0, 1.0);
    }
    
    void update(double wind) {
        // Heat rises
        vy -= 0.15;
        vx += wind * 0.5;
        
        // Add turbulence
        vx += randomFloat(-0.3, 0.3);
        vy += randomFloat(-0.2, 0.1);
        
        // Apply velocity
        x += vx;
        y += vy;
        
        // Cool down and fade
        heat *= 0.96;
        life -= randomFloat(0.01, 0.03);
        
        // Flicker
        flicker += randomFloat(-0.3, 0.3);
        if (flicker < 0) flicker = 0;
        if (flicker > 1) flicker = 1;
        
        // Damping
        vx *= 0.95;
        vy *= 0.98;
    }
    
    bool isAlive() const {
        return life > 0 && x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
    }
};

class FireSimulation {
private:
    vector<Ember> embers;
    vector<vector<double>> heatMap;
    int frame;
    double wind;
    double windChange;
    string mode;
    double intensity;
    
public:
    FireSimulation() : frame(0), wind(0), windChange(0), 
                       mode("campfire"), intensity(1.0) {
        heatMap.resize(HEIGHT, vector<double>(WIDTH, 0.0));
        srand(time(NULL));
    }
    
    void spawnFire() {
        int spawnCount = 0;
        
        if (mode == "campfire") {
            int centerX = WIDTH / 2;
            int fireWidth = 20;
            spawnCount = (int)(15 * intensity);
            
            for (int i = 0; i < spawnCount; i++) {
                int x = centerX + randomInt(-fireWidth, fireWidth);
                double heat = randomFloat(0.8, 1.0) * 
                             (1.0 - abs(x - centerX) / (double)fireWidth * 0.5);
                embers.push_back(Ember(x, HEIGHT - 3, heat));
            }
        }
        else if (mode == "inferno") {
            spawnCount = (int)(40 * intensity);
            for (int i = 0; i < spawnCount; i++) {
                int x = randomInt(10, WIDTH - 10);
                double heat = randomFloat(0.9, 1.0);
                embers.push_back(Ember(x, HEIGHT - 3, heat));
            }
        }
        else if (mode == "forge") {
            int centerX = WIDTH / 2;
            spawnCount = (int)(25 * intensity);
            for (int i = 0; i < spawnCount; i++) {
                int x = centerX + randomInt(-10, 10);
                double heat = randomFloat(1.0, 1.2);
                embers.push_back(Ember(x, HEIGHT - 3, heat));
            }
        }
        else if (mode == "dragon") {
            int breathX = 20 + (frame * 2) % (WIDTH - 40);
            spawnCount = (int)(30 * intensity);
            for (int i = 0; i < spawnCount; i++) {
                int x = breathX + randomInt(-5, 15);
                int y = HEIGHT / 2 + randomInt(-3, 3);
                Ember ember(x, y, randomFloat(0.9, 1.1));
                ember.vx = randomFloat(2.0, 4.0);
                ember.vy = randomFloat(-0.5, 0.5);
                embers.push_back(ember);
            }
        }
        else if (mode == "wildfire") {
            int spread = (int)(20 + 30 * sin(frame * 0.05));
            spawnCount = (int)(35 * intensity);
            for (int i = 0; i < spawnCount; i++) {
                int x = WIDTH / 3 + spread + randomInt(-25, 25);
                double heat = randomFloat(0.7, 1.0);
                embers.push_back(Ember(x, HEIGHT - 3, heat));
            }
        }
    }
    
    void update() {
        // Update wind
        windChange += randomFloat(-0.1, 0.1);
        windChange = max(-0.5, min(0.5, windChange));
        wind += windChange;
        wind = max(-WIND_STRENGTH * 2, min(WIND_STRENGTH * 2, wind));
        wind *= 0.98;
        
        // Spawn new embers
        spawnFire();
        
        // Update all embers
        for (auto& ember : embers) {
            ember.update(wind);
        }
        
        // Remove dead embers
        embers.erase(
            remove_if(embers.begin(), embers.end(), 
                     [](const Ember& e) { return !e.isAlive(); }),
            embers.end()
        );
        
        // Limit ember count
        if (embers.size() > MAX_EMBERS) {
            embers.erase(embers.begin(), 
                        embers.begin() + (embers.size() - MAX_EMBERS));
        }
        
        // Build heat map
        for (auto& row : heatMap) {
            fill(row.begin(), row.end(), 0.0);
        }
        
        for (const auto& ember : embers) {
            int x = (int)ember.x;
            int y = (int)ember.y;
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                heatMap[y][x] += ember.heat * ember.flicker;
            }
        }
        
        frame++;
    }
    
    pair<string, string> getFireCharAndColor(double heat, double flicker) {
        heat = heat * (0.8 + flicker * 0.4);
        
        if (heat > 1.5) return {"█", Color::WHITE + Color::BOLD};
        else if (heat > 1.2) return {"▓", Color::YELLOW + Color::BOLD};
        else if (heat > 0.9) return {"▒", Color::YELLOW};
        else if (heat > 0.6) return {"░", Color::ORANGE};
        else if (heat > 0.4) return {"▒", Color::RED};
        else if (heat > 0.2) return {"░", Color::DARK_RED};
        else if (heat > 0.1) return {"·", Color::GRAY};
        else return {"˙", Color::BLACK};
    }
    
    void render() {
        // Clear screen
        cout << "\033[2J\033[H";
        
        // Top border
        cout << Color::RED;
        for (int i = 0; i < WIDTH; i++) cout << "═";
        cout << Color::RESET << '\n';
        
        // Render fire
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                double heat = heatMap[y][x];
                
                if (heat > 0.05) {
                    double flicker = 0.5;
                    // Find ember for flicker
                    for (const auto& ember : embers) {
                        if ((int)ember.x == x && (int)ember.y == y) {
                            flicker = ember.flicker;
                            break;
                        }
                    }
                    
                    auto [ch, color] = getFireCharAndColor(heat, flicker);
                    cout << color << ch << Color::RESET;
                } else {
                    if (randomFloat(0, 1) > 0.98) {
                        cout << Color::BLACK << "·" << Color::RESET;
                    } else {
                        cout << ' ';
                    }
                }
            }
            cout << '\n';
        }
        
        // Bottom border with fire base
        for (int x = 0; x < WIDTH; x++) {
            double bottomHeat = 0;
            for (const auto& ember : embers) {
                if ((int)ember.x == x && ember.y > HEIGHT - 5) {
                    bottomHeat += ember.heat;
                }
            }
            
            if (bottomHeat > 0.5) {
                cout << Color::ORANGE << Color::BOLD << "▀" << Color::RESET;
            } else if (bottomHeat > 0.2) {
                cout << Color::RED << "▀" << Color::RESET;
            } else {
                cout << Color::DARK_RED << "═" << Color::RESET;
            }
        }
        cout << '\n';
        
        // Stats
        double avgHeat = 0;
        if (!embers.empty()) {
            for (const auto& e : embers) avgHeat += e.heat;
            avgHeat /= embers.size();
        }
        
        string windArrow = wind > 0 ? "→" : (wind < 0 ? "←" : "↕");
        
        cout << '\n' << Color::ORANGE 
             << "╔═══════════════════════════════════════════════════════════════════════╗\n"
             << "║" << Color::RESET << " 🔥 Mode: " << Color::BOLD << Color::YELLOW 
             << mode << Color::RESET << "  |  Embers: " << Color::BOLD << embers.size() 
             << Color::RESET << "  |  Temp: " << Color::BOLD << Color::RED;
        printf("%.2f", avgHeat);
        cout << Color::RESET << "  |  Wind: " << windArrow << " ";
        printf("%.1f", abs(wind));
        cout << "  |  Frame: " << frame << " " << Color::ORANGE << "║\n"
             << "╚═══════════════════════════════════════════════════════════════════════╝"
             << Color::RESET << "\n\n";
        
        cout << Color::YELLOW 
             << "[1] Campfire  [2] Inferno  [3] Forge  [4] Dragon  [5] Wildfire\n"
             << Color::RESET << Color::GRAY 
             << "[+/-] Intensity  [W] Wind  [Q] Quit" << Color::RESET << '\n';
    }
    
    void handleInput(char key) {
        switch(key) {
            case '1': mode = "campfire"; break;
            case '2': mode = "inferno"; break;
            case '3': mode = "forge"; break;
            case '4': mode = "dragon"; break;
            case '5': mode = "wildfire"; break;
            case '+': case '=': 
                intensity = min(2.0, intensity + 0.2); 
                break;
            case '-': 
                intensity = max(0.2, intensity - 0.2); 
                break;
            case 'w': case 'W': 
                wind += randomFloat(-1.0, 1.0); 
                break;
        }
    }
    
    int getFrame() { return frame; }
};

// macOS compatible non-blocking input
struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char getKeyPress() {
    char c = 0;
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
        read(STDIN_FILENO, &c, 1);
    }
    
    return c;
}

int main() {
    // Title screen
    cout << Color::ORANGE << Color::BOLD;
    cout << "╔═══════════════════════════════════════════╗\n";
    cout << "║                                           ║\n";
    cout << "║       🔥 FIRE SIMULATION 2.0 🔥          ║\n";
    cout << "║                                           ║\n";
    cout << "║    Realistic Physics & Particle FX       ║\n";
    cout << "║          C++ Implementation              ║\n";
    cout << "║              macOS Build                 ║\n";
    cout << "║                                           ║\n";
    cout << "╚═══════════════════════════════════════════╝\n";
    cout << Color::RESET;
    cout << "Igniting in 2 seconds...\n";
    
    this_thread::sleep_for(chrono::seconds(2));
    
    enableRawMode();
    
    FireSimulation sim;
    
    try {
        while (true) {
            // Check for input
            char ch = getKeyPress();
            if (ch == 'q' || ch == 'Q') break;
            if (ch != 0) sim.handleInput(ch);
            
            sim.update();
            sim.render();
            
            this_thread::sleep_for(chrono::milliseconds(40));  // 25 FPS
        }
    } catch (...) {
        disableRawMode();
        throw;
    }
    
    disableRawMode();
    
    cout << "\n\n" << Color::RED << "🔥 Fire extinguished! 🔥" << Color::RESET << '\n';
    cout << "Total frames: " << sim.getFrame() << "\n\n";
    
    return 0;
}