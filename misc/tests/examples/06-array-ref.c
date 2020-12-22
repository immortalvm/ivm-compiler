/* References relative to labels are 
   printed using prefix notation*/

int a[] = {1, 3, 4, 2, 0};

int main(int c, char** v)
{
	a[4] = a[1];
	return a[4];
}
