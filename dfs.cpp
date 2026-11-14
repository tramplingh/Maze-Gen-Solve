class DfsSolver : public ISolver{
	std::stack<int> dfsStack;
public:
	void init(MazeState& s) override{
		s.g[s.start] = 0;
		s.state[s.start] = OPEN;
		dfsStack.push(s.start);
		s.expanded = 1;
	}
	
	void step(MazeState& s) override{
		s.steps++;
		if(dfsStack.empty()){
			s.failed = true;
			return;
		}
		
		int cur = dfsStack.top();
		s.head = cur;
		if(cur == s.goal){
			s.solved = true;
			return;
		}
		
		for(int d = 0; d < 4; d++){
			int n = s.neighbour(cur,d);
			if(n < 0 || s.state[n] != UNSEEN)
				continue;
			s.parent[n] = cur;
			s.g[n] = s.g[cur] + 1;
			s.state[n] = OPEN;
			dfsStack.push(n);
			s.expanded++;
			return;
		}
		s.state[cur] = CLOSED;
		dfsStack.pop();
		if(dfsStack.empty())
			s.failed = true;
	}
};
