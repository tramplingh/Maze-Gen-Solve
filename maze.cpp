#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <csignal>
using namespace std;

#ifdef _WIN32
#include <windows.h>
#endif

enum
{
    CELL_PATH_N  = 0x01,
    CELL_PATH_E  = 0x02,
    CELL_PATH_S  = 0x04,
    CELL_PATH_W  = 0x08,
    CELL_VISITED = 0x10,
};

enum { PX_BLACK = 0, PX_BLUE = 1, PX_WHITE = 2, PX_GREEN = 3 };

static void restoreTerminal(int = 0)
{
    cout << "\033[0m\033[?25h\n" << flush;
    exit(0);
}

static bool saveMaze(const string& path, const vector<int>& maze, int w, int h,
                     int sx, int sy, int ex, int ey)
{
    ofstream f(path);
    if (!f) return false;
    f << "MAZE 1\n" << w << " " << h << "\n"
      << sx << " " << sy << "\n" << ex << " " << ey << "\n";
    static const char* hex = "0123456789ABCDEF";
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
            f << hex[maze[y * w + x] & 0x0F];
        f << "\n";
    }
    return (bool)f;
}

int main(int argc, char** argv)
{
    int mazeWidth  = 20;
    int mazeHeight = 10;
    int pathWidth  = 1;
    int delayMs    = 30;

    if (argc >= 3) { mazeWidth = atoi(argv[1]); mazeHeight = atoi(argv[2]); }
    if (argc >= 4) pathWidth = atoi(argv[3]);
    if (argc >= 5) delayMs = atoi(argv[4]);
    
    string outFile;
    if (argc >= 6) 
    {
        outFile = argv[5];
    } 
    else 
    {
        cout << "Enter output file name to save as (default: maze.txt): ";
        string input;
        getline(cin, input);
        if (input.empty()) outFile = "maze.txt";
        else outFile = input;
    }

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | 0x0004);
#endif

    signal(SIGINT, restoreTerminal);
    srand((unsigned)time(nullptr));

    vector<int> maze(mazeWidth * mazeHeight, 0);
    stack<pair<int, int>> st;

    int sx = 0;
    int sy = 0;
    st.push({ sx, sy });
    maze[sy * mazeWidth + sx] = CELL_VISITED;
    int visitedCells = 1;

    int cw = mazeWidth  * (pathWidth + 1);
    int ch = mazeHeight * (pathWidth + 1);

    cout << "\033[2J\033[?25l";

    bool done = false;
    while (true)
    {
        auto offset = [&](int x, int y)
        {
            return (st.top().second + y) * mazeWidth + (st.top().first + x);
        };

        if (visitedCells < mazeWidth * mazeHeight)
        {
            vector<int> neighbours;

            if (st.top().second > 0 && (maze[offset(0, -1)] & CELL_VISITED) == 0)
                neighbours.push_back(0);
            if (st.top().first < mazeWidth - 1 && (maze[offset(1, 0)] & CELL_VISITED) == 0)
                neighbours.push_back(1);
            if (st.top().second < mazeHeight - 1 && (maze[offset(0, 1)] & CELL_VISITED) == 0)
                neighbours.push_back(2);
            if (st.top().first > 0 && (maze[offset(-1, 0)] & CELL_VISITED) == 0)
                neighbours.push_back(3);

            if (!neighbours.empty())
            {
                int dir = neighbours[rand() % neighbours.size()];
                int cx = st.top().first, cy = st.top().second;

                switch (dir)
                {
                case 0: maze[offset(0, -1)] |= CELL_VISITED | CELL_PATH_S;
                        maze[offset(0,  0)] |= CELL_PATH_N;
                        st.push({ cx, cy - 1 }); break;
                case 1: maze[offset(1, 0)]  |= CELL_VISITED | CELL_PATH_W;
                        maze[offset(0, 0)]  |= CELL_PATH_E;
                        st.push({ cx + 1, cy }); break;
                case 2: maze[offset(0, 1)]  |= CELL_VISITED | CELL_PATH_N;
                        maze[offset(0, 0)]  |= CELL_PATH_S;
                        st.push({ cx, cy + 1 }); break;
                case 3: maze[offset(-1, 0)] |= CELL_VISITED | CELL_PATH_E;
                        maze[offset( 0, 0)] |= CELL_PATH_W;
                        st.push({ cx - 1, cy }); break;
                }
                visitedCells++;
            }
            else
            {
                st.pop();
            }
        }
        else
        {
            done = true;
        }

        vector<unsigned char> canvas(cw * ch, PX_BLACK);
        auto draw = [&](int x, int y, unsigned char c) { canvas[y * cw + x] = c; };

        for (int x = 0; x < mazeWidth; x++)
        {
            for (int y = 0; y < mazeHeight; y++)
            {
                int cell = maze[y * mazeWidth + x];
                int bx = x * (pathWidth + 1);
                int by = y * (pathWidth + 1);

                for (int py = 0; py < pathWidth; py++)
                    for (int px = 0; px < pathWidth; px++)
                        draw(bx + px, by + py, (cell & CELL_VISITED) ? PX_WHITE : PX_BLUE);

                for (int p = 0; p < pathWidth; p++)
                {
                    if (cell & CELL_PATH_S) draw(bx + p, by + pathWidth, PX_WHITE);
                    if (cell & CELL_PATH_E) draw(bx + pathWidth, by + p, PX_WHITE);
                }
            }
        }

        if (!done)
            for (int py = 0; py < pathWidth; py++)
                for (int px = 0; px < pathWidth; px++)
                    draw(st.top().first * (pathWidth + 1) + px,
                         st.top().second * (pathWidth + 1) + py, PX_GREEN);

        string out = "\033[H";
        static const char* codes[] = { "\033[40m", "\033[44m", "\033[47m", "\033[42m" };
        int last = -1;
        for (int y = 0; y < ch; y++)
        {
            for (int x = 0; x < cw; x++)
            {
                int c = canvas[y * cw + x];
                if (c != last) { out += codes[c]; last = c; }
                out += "  ";
            }
            out += "\033[0m\n";
            last = -1;
        }
        cout << out << flush;

        if (done) break;
        this_thread::sleep_for(chrono::milliseconds(delayMs));
    }

    bool ok = saveMaze(outFile, maze, mazeWidth, mazeHeight, 0, 0, mazeWidth - 1, mazeHeight - 1);
    cout << "\033[0m" << (ok ? "Saved maze to " : "FAILED to save maze to ") << outFile;

    restoreTerminal();
    return 0;
}
