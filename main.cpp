#include <iostream>
#include <fstream>
#include <stack>
#include <queue>
#include <tuple>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <csignal>
#include <memory>
#include <algorithm>
using namespace std;

#ifdef _WIN32
#include <windows.h>
#endif

enum { CELL_PATH_N = 0x01, CELL_PATH_E = 0x02, CELL_PATH_S = 0x04, CELL_PATH_W = 0x08 };
enum { UNSEEN = 0, OPEN = 1, CLOSED = 2 };
enum { PX_BLACK = 0, PX_RED = 1, PX_GREEN = 2, PX_YELLOW = 3, PX_BLUE = 4, PX_MAGENTA = 5, PX_CYAN = 6, PX_WHITE = 7 };

const int INF = 1000000000;

struct MazeState {
    vector<int> maze;
    int w, h, start, goal;
    vector<int> state, parent, g;
    
    int head = 0, steps = 0, expanded = 0;
    bool solved = false, failed = false;

    int neighbour(int cur, int d) const {
        const int dx[4] = { 0, 1, 0, -1 };
        const int dy[4] = { -1, 0, 1, 0 };
        const int bit[4] = { CELL_PATH_N, CELL_PATH_E, CELL_PATH_S, CELL_PATH_W };
        if (!(maze[cur] & bit[d])) return -1;
        int nx = cur % w + dx[d], ny = cur / w + dy[d];
        if (nx < 0 || ny < 0 || nx >= w || ny >= h) return -1;
        return ny * w + nx;
    }
};

class ISolver {
public:
    virtual ~ISolver() = default;
    virtual void init(MazeState& s) = 0;
    virtual void step(MazeState& s) = 0;
};

// Include the algorithms directly
#include "dfs.cpp"
#include "bfs.cpp"
#include "dijkstra.cpp"
#include "astar.cpp"

static void restoreTerminal(int = 0) {
    cout << "\033[0m\033[?25h\n" << flush;
    exit(0);
}

static int parseAlgo(string s) {
    string t;
    for (char c : s) if (!isspace((unsigned char)c)) t += (char)tolower((unsigned char)c);
    if (t == "1" || t == "dfs") return 0;
    if (t == "2" || t == "bfs") return 1;
    if (t == "3" || t == "dijkstra" || t == "dij") return 2;
    if (t == "4" || t == "astar" || t == "a*" || t == "a-star" || t == "a") return 3;
    return -1;
}

static int chooseAlgo() {
    cout << "Choose an algorithm:\n  1) DFS\n  2) BFS\n  3) Dijkstra\n  4) A*\n> ";
    string line;
    while (getline(cin, line)) {
        int a = parseAlgo(line);
        if (a >= 0) return a;
        cout << "Please enter 1-4 or dfs / bfs / dijkstra / astar.\n> ";
    }
    return -1;
}

static bool loadMaze(const string& path, MazeState& s, int& sx, int& sy, int& ex, int& ey) {
    ifstream f(path);
    if (!f) return false;
    string magic; int version;
    if (!(f >> magic >> version) || magic != "MAZE" || version != 1) return false;
    if (!(f >> s.w >> s.h >> sx >> sy >> ex >> ey)) return false;
    s.maze.assign(s.w * s.h, 0);
    for (int y = 0; y < s.h; y++) {
        string row; f >> row;
        for (int x = 0; x < s.w; x++) {
            char c = row[x];
            int v = (c >= 'a') ? c - 'a' + 10 : ((c >= 'A') ? c - 'A' + 10 : c - '0');
            s.maze[y * s.w + x] = v;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    string file = "maze.txt";
    int algoIdx = -1, pw = 1, delayMs = 30;
    if (argc >= 2) file = argv[1];
    if (argc >= 3) algoIdx = parseAlgo(argv[2]);
    if (argc >= 4) pw = max(1, atoi(argv[3]));
    if (argc >= 5) delayMs = atoi(argv[4]);

    MazeState s;
    int sx, sy, ex, ey;
    if (!loadMaze(file, s, sx, sy, ex, ey)) { cerr << "Could not read '" << file << "'\n"; return 1; }
    
    if (algoIdx < 0) algoIdx = chooseAlgo();
    if (algoIdx < 0) return 1;
    
    s.start = sy * s.w + sx;
    s.goal = ey * s.w + ex;
    int N = s.w * s.h;
    
    s.state.assign(N, UNSEEN);
    s.parent.assign(N, -1);
    s.g.assign(N, INF);
    vector<char> onPath(N, 0);
    
    unique_ptr<ISolver> solver;
    const char* algoNames[] = { "DFS", "BFS", "Dijkstra", "A*" };
    switch (algoIdx) {
        case 0: solver = make_unique<DfsSolver>(); break;
        case 1: solver = make_unique<BfsSolver>(); break;
        case 2: solver = make_unique<DijkstraSolver>(); break;
        case 3: solver = make_unique<AStarSolver>(); break;
    }
    solver->init(s);

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | 0x0004);
#endif
    signal(SIGINT, restoreTerminal);

    int pathLen = 0;
    const int cw = s.w * (pw + 1), ch = s.h * (pw + 1);
    cout << "\033[2J\033[?25l";

    while (true) {
        if (!s.solved && !s.failed) {
            solver->step(s);
            if (s.solved) {
                for (int c = s.goal; c != -1; c = s.parent[c]) { onPath[c] = 1; pathLen++; }
            }
        }

        vector<unsigned char> canvas(cw * ch, PX_BLACK);
        auto draw = [&](int x, int y, unsigned char c) { canvas[y * cw + x] = c; };
        auto block = [&](int cell, unsigned char c) {
            int bx = (cell % s.w) * (pw + 1), by = (cell / s.w) * (pw + 1);
            for (int py = 0; py < pw; py++) for (int px = 0; px < pw; px++) draw(bx + px, by + py, c);
        };
        auto gap = [&](int a, int b, unsigned char c) {
            int ax = a % s.w, ay = a / s.w, bx = b % s.w, by = b / s.w;
            if (ax == bx) { int gy = min(ay, by) * (pw + 1) + pw; for (int p = 0; p < pw; p++) draw(ax * (pw + 1) + p, gy, c); }
            else { int gx = min(ax, bx) * (pw + 1) + pw; for (int p = 0; p < pw; p++) draw(gx, ay * (pw + 1) + p, c); }
        };

        for (int y = 0; y < s.h; y++) {
            for (int x = 0; x < s.w; x++) {
                int cell = y * s.w + x;
                block(cell, PX_WHITE);
                if (s.maze[cell] & CELL_PATH_S) gap(cell, cell + s.w, PX_WHITE);
                if (s.maze[cell] & CELL_PATH_E) gap(cell, cell + 1, PX_WHITE);
            }
        }

        for (int cell = 0; cell < N; cell++) {
            if (s.state[cell] == UNSEEN) continue;
            unsigned char c = (s.state[cell] == OPEN) ? PX_YELLOW : PX_BLUE;
            block(cell, c);
            if (s.parent[cell] >= 0) gap(cell, s.parent[cell], c);
        }

        if (s.solved) for (int cell = 0; cell < N; cell++) if (onPath[cell]) { block(cell, PX_GREEN); if (s.parent[cell] >= 0) gap(cell, s.parent[cell], PX_GREEN); }
        if (!s.solved && !s.failed) block(s.head, PX_RED);
        block(s.start, PX_MAGENTA); block(s.goal, PX_MAGENTA);

        string out = "\033[H";
        int last = -1;
        for (int y = 0; y < ch; y++) {
            for (int x = 0; x < cw; x++) {
                int c = canvas[y * cw + x];
                if (c != last) { out += "\033[" + to_string(40 + c) + "m"; last = c; }
                out += "  ";
            }
            out += "\033[0m\n"; last = -1;
        }
        out += string(algoNames[algoIdx]) + "  steps: " + to_string(s.steps) + "  expanded: " + to_string(s.expanded)
             + (s.solved ? "  path: " + to_string(pathLen) + "  SOLVED" : s.failed ? "  NO PATH" : "") + "\033[K\n";
        cout << out << flush;

        if (s.solved || s.failed) break;
        this_thread::sleep_for(chrono::milliseconds(delayMs));
    }

    if (s.solved) cout << algoNames[algoIdx] << ": path " << pathLen << " cells, expanded " << s.expanded << " of " << N << " cells in " << s.steps << " steps\n";
    restoreTerminal();
    return 0;
}
