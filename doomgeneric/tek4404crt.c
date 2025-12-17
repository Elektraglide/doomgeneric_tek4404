#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/* missing runtime calls */
char *memmove(char *dst, char *src, int n)
{
	if (dst < src)
		return memcpy(dst,src,n);
	else
	{
		register char *dest,*source;
		dest = dst + n;
		source = src + n;
		while(n--)
		{
			*--dst = *--source;
		}
		return dst;
	}
}

#pragma GCC optimize ("O0")
int snprintf(char *buffer, int n, char *fmt, ...)
{
	va_list argptr;
	unsigned int val1;
	unsigned int val2;
	unsigned int val3;
	unsigned int val4;

	memset(buffer, 0, n);
	
	va_start(argptr, fmt);
    val1 = va_arg(argptr, unsigned int);
    val2 = va_arg(argptr, unsigned int);
    val3 = va_arg(argptr, unsigned int);
    val4 = va_arg(argptr, unsigned int);
	va_end(argptr);

	sprintf(buffer, fmt, val1,val2,val3,val4);
}

int strncasecmp(char *str1, char *str2, int n)
{
	char c,c1;
	
	while (n--)
	{
		c = *str1++;
		if (c == 0)
			break;
		if (isupper(c))
			c = _tolower(c);
			
		c1 = *str2++;
		if (c1 == 0)
			break;
		if (isupper(c1))
			c1 = _tolower(c1);
			
		if ((c) != (c1))
			return 1;
	}

	return 0;
}

int strcasecmp(char *str1, char *str2)
{
	return strncasecmp(str1, str2, 1024);
}

char *strdup(char *str)
{
	char *duplicate;
	
	duplicate = malloc(strlen(str)+1);
	strcpy(duplicate, str);
	return duplicate;
}

#undef putchar
int putchar(char c)
{
	return putc(c, stdout);
}

int remove(char *path)
{
	unlink(path);
	return 0;
}

int rename(char *old, char *new)
{
	int rc;
	
	rc = link(old, new);
	if (rc == 0)
	{
		unlink(old);
	}
	return 0;
}

int vfprintf(FILE *fp, char *fmt, va_list argptr)
{
	unsigned int val1;
	unsigned int val2;
	unsigned int val3;
	unsigned int val4;

	argptr -= 4;
    val1 = va_arg(argptr, unsigned int);
    val2 = va_arg(argptr, unsigned int);
    val3 = va_arg(argptr, unsigned int);
    val4 = va_arg(argptr, unsigned int);
	va_end(argptr);
	fprintf(fp, fmt, val1,val2,val3,val4);
}

#define MKNOD_OWNER 0x0007
#define MKNOD_OTHER 0x0028
#define MKNOD_DIR 0x0800

int mkdir(char *path)
{
	char linkpath[256], linkdest[256];
	char *pcVar1;
	
	printf("mkdir: %s\015", path);
	mknod(path, MKNOD_DIR + MKNOD_OTHER + MKNOD_OWNER, 0);
	chown(path, 0);

	/* from Ghidra decompile of Tek4404 crdir command */
	strcpy(linkpath,path);
	strcat(linkpath,"/.");
	link(path,linkpath);
	chown(linkpath, 0);
	
	if (path[0] == '/') {
		strcpy(linkpath,path);
	}
	else {
		strcpy(linkpath,"./");
		strcat(linkpath,path);
	}
	pcVar1 = (char *)strchr(linkpath, '/');
	if (linkpath == pcVar1) {
		pcVar1 = pcVar1 + 1;
	}
	*pcVar1 = '\0';
	strcpy(linkdest,path);
	strcat(linkdest,"/..");
	link(linkpath,linkdest);
}

int vsnprintf(char *buffer, int n, char *fmt, va_list argptr)
{
	unsigned int val1;
	unsigned int val2;
	unsigned int val3;
	unsigned int val4;

    val1 = va_arg(argptr, unsigned int);
    val2 = va_arg(argptr, unsigned int);
    val3 = va_arg(argptr, unsigned int);
    val4 = va_arg(argptr, unsigned int);
	va_end(argptr);
	sprintf(buffer, fmt, val1,val2,val3,val4);
}

int __floatsisf()
{
}

int __extendsfdf2()
{

}

int __fixsfsi()
{

}

