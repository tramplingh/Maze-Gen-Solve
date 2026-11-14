class DijkstraSolver : public ISolver {
protected:
    typedef std::tuple<int, int, int> PQItem; 
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;
    
public:
    void init(MazeState& s) override {
        s.g[s.start] = 0;
        s.state[s.start] = OPEN;
        pq.push(PQItem(0, 0, s.start));
    }

    void step(MazeState& s) override {
        s.steps++;
        int cur = -1;
        while (!pq.empty()) {
            int c = std::get<2>(pq.top()); 
            pq.pop();
            if (s.state[c] == CLOSED) continue;
            cur = c; 
            break;
        }
        
        if (cur < 0) { s.failed = true; return; }
        
        s.state[cur] = CLOSED; 
        s.head = cur; 
        s.expanded++;
        if (cur == s.goal) { s.solved = true; return; }
        
        for (int d = 0; d < 4; d++) {
            int n = s.neighbour(cur, d);
            if (n < 0 || s.state[n] == CLOSED) continue;
            
            int ng = s.g[cur] + 1;
            if (ng < s.g[n]) {
                s.g[n] = ng; 
                s.parent[n] = cur; 
                s.state[n] = OPEN;
                pq.push(PQItem(ng, 0, n));
            }
        }
    }
};
