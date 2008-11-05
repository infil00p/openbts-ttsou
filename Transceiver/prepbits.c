#include <stdio.h>


int main(int argc, char *argv[])
{
	int ch = getchar();
	while (ch!=EOF) {
		if (ch=='0') putchar('0');
		if (ch=='1') putchar('1');
		ch = getchar();
	}
}
