
static char text[] = "String test";
static int tab[16];

int main(int argc, char *argv[])
{
	text[6] = '+';
	int a = 1, b = 3;
	a += b;
	tab[3] = text[8];
	return 0;
}
