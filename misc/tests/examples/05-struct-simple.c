/* Simple struct */

struct S { int x; } sa = {12} ;

int main(){
    struct S sb = sa;
    return sb.x;
}
