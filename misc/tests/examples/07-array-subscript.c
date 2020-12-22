/* Subscripted subcripts */

int a[] = {1, 3, 4, 2, 0};

int main(int c, char** v)
{
	int idx=4;
	return a[a[a[a[idx]]]];
}
