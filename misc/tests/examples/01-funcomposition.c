/* One function calls other functions as arguments */

int f(){ return 1; }
int g(int x){ return x; }
int h(){ return 11; }

int z(int x, int y, int z) { return z+y+z; }

int main(){
  return  z(f(), g(12), g(h()));
}
