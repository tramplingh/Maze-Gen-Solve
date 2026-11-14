class BfsSolver : public ISolver{
	std::queue<int> bfsQueue;
public:
	void init(MazeState& s) override{
		s.g[s.start] = 0;
		s.state[s.start] = OPEN;
		bfsQueue.push(s.start);
	}
	
	void step(MazeState& s) override{
		s.steps++;
		if(bfsQueue.empty()){
			s.failed = true;
			return;
		}
		
		int cur = bfsQueue.front();
		bfsQueue.pop();
		
		s.state[cur] = CLOSED;
		s.head = cur;
		s.expanded++;
		
		if(cur == s.goal){
			s.solved = true;
			return;
		}
		
		for(int d = 0; d < 4; d++){
			int n = s.neighbour(cur, d);
			if(n < 0 || s.state[n] != UNSEEN) 
				continue;
			s.parent[n] = cur;
			s.g[n] = s.g[cur] + 1;
			s.state[n] = OPEN;
			bfsQueue.push(n);
		}
	}
};
