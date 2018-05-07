#include <stdio.h>
float func1(float a, double b)
{
    double y[3] = {1, 2, 3};
    float c = a + b;
    // double d = a + b;
    double d = a + b - c;
    c = a - b;
    return c / d;
	// return a * (a + b);
}

double func4(double a, double b)
{
    double t1 = 0;
    double t2 = 42;
    double t3 = a + b - (t1 + a) * t2;
    double t4 = t3 / 57;
    return t4;
}

int main()
{
	// printf() displays the string inside quotation
    float x = func1(5, 6);
    float y;
    y = 10;

    if (x > 15)
    {
        printf(">15\n");
    }
    else
    {
        printf("<=15\n");
    }

    if (x)
        printf("test\n");

	printf("Hello, World!\n");
    printf("x: %f\n", x);
    
    do
    {
        printf("x: %f\n", x);
    } while (x-- < 15);

    for (double i = 0; i < 5; ++i)
        i++;

	return 0;
}