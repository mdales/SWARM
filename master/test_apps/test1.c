#define SIZE 10

int data[SIZE];

int main()
{
#if 0
	int i, j, temp, n;

	i = 1;
	j = 0;

	for (n = 0; n < SIZE; n++)
	{
		data[n] = i;
		temp = i + j;
		j = i;
		i = temp;
	}
#endif
  volatile unsigned long long int a, b, c;
  a = 5;
  b = 3;
  c = a % b;
	
	return 0;
}
