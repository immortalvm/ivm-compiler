/* Inline assembly using local variables and arguments (stack)*/

// Compile this code only for the ivm64 cpu
#ifdef __ivm64__
	int add_2(int a, int b){
		int c=100, r;
		
		// Strategy
		long aa[1] = {a};
		long bb[1] = {b};
		long cc[1] = {c};
		long rr[1] = {r};
		
        // Let's compute a+b+c

		// SP unbalance = 0
		asm volatile("load8! %0":: "m"(aa[0])); 
		// SP unbalance = 1
		asm volatile("load8! %0":: "m"(bb[1]));
		// SP unbalance = 2
		asm volatile("load8! %0":: "m"(cc[2]));

		// SP unbalance = 3
		asm volatile("add");
		// SP unbalance = 2    
		asm volatile("add");

		// SP unbalance = 1
		asm volatile("store8! %0": "=m"(rr[1]));

		r = rr[0]; // Restore r
		return r;
	}

	int main(){
		return add_2(2,3);
	}
#else
int main(){} // Do nothing if not ivm64
#endif
