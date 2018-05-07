float func2(float x, double y)
{
	int a = 7;
	if (a % 2)
	{
		float k = x - y;
	}
	else
	{
		double j = x * y;
	}
	return x + y;
}

void func3(float *arr, int x)
{
	for (int i = 0; i < x; ++i)
	{
		arr[i] = arr[i+1];
	}
}

