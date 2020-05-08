#include <sys/stat.h>
#include <stdio.h>

int not_exist(char *f)
{
	struct stat info;
	return (stat(f, &info) == -1);
}

int main(int argc, char const *argv[])
{
	char *f = "./blurImg.html";
	printf("%d\n", not_exist(f));
	return 0;
}