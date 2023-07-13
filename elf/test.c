extern int fun(int, int);

int fun3()
{
 asm("nop");
}

__attribute__ ((weak))
int fun2()
{
 asm("nop");
}

int main()
{
 volatile int s = fun(0, 1);
 fun2();
 fun3();
}
