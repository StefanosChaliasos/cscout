struct sa {
	int a, b;
} k;

struct sa *fun(int a, int b);

struct sa *fun2(int a, int b)
{
	static struct sa r;

	r.a = r.b = 12;
	return &r;
}

foo()
{
	fun(1, 2)->a = fun(1, 2)->b = 0;
	fun2(3, 4)->a = fun2(1, 2)->b = 12;
}
