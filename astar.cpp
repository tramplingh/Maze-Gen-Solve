class AStarSolver : public DijkstraSolver {
    int heuristic(const MazeState& s, int c) const {
        int ex = s.goal % s.w;
        int ey = s.goal / s.w;
        return std::abs(c % s.w - ex) + std::abs(c / s.w - ey);
    }
	public:
	    void init(MazeState& s) override {
		s.g[s.start] = 0;
		s.state[s.start] = OPEN;
		int h = heuristic(s, s.start);
		pq.push(PQItem(h, h, s.start));
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
		        int hh = heuristic(s, n);
		        pq.push(PQItem(ng + hh, hh, n));
		    }
		}
	    }
	};
