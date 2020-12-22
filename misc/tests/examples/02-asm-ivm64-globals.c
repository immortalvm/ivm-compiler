/* Inline assembly using global variables*/

int a=3, b=4, c; 

// Compile this code only for the ivm64 cpu
#ifdef __ivm64__
	void add_2(){ // Operating on globals: c = a + b
		asm volatile("load8! %0": "=m"(a));
		asm volatile("load8! %0": "=m"(b));
		asm volatile("add");
		asm volatile("store8! %0": "=m"(c));
	}

	int main(){
		add_2();
		return c;
	}
#else
int main(){} // Do nothing if not ivm64
#endif

