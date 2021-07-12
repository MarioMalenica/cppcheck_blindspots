/* A collection of common C code errors and pitfalls
 * which are not detected by cppcheck.
 *
 * They are also not detected by gcc when compiled with
 * the following options:
 * -pedantic -Wall -Wextra -Wconversion
 *
 * Neither cppcheck 2.4.1 or gcc were able to find a single error
 *
 *
 * Splint was used as a reference static analysis tool.
 * Result of its scan is stored in "splint_check_result.txt"
 * Code was made with C90 compatibility because splint
 * does not support C99 or C11.
 *
 * The errors were mostly found in various embedded
 * systems that are supposed to run unattended 24/7/365.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#if !defined(WIN32)
#include <dirent.h>
#endif

/* Several potential crashes caused by file access functions */
static void file_crash(void)
{
	FILE *fp;
	char text[20];
	memset(text, 0, sizeof(text));
	/* Not checking the return value, it can be NULL. */
	fp = fopen("nonexisting_file.txt", "r");

	/* 1. This would cause a segmentation fault if fp is NULL.
	 * 2. Not checking result of function call. fgets() can fail */
	fgets(text, sizeof(text) - 1, fp);

	/* 1. This would cause a segmentation fault if fp is NULL.
	 * 2. Not checking result of function call. fclose() can fail. */
	fclose(fp);
}

static void buffer_overflow(void)
{
	const char *foo = "12345abcde";
	char bar[9];
	size_t len;
	int cmp;

	strncpy(bar, foo, sizeof(bar) - 1);
	/* Buffer overflow */
	strcat(bar, "012");
	len = strlen(foo);
	/* Accessing a location outside of array boundaries */
	cmp = strncmp(foo, bar, len);
	(void)printf("%s %s %c\n", foo, bar, (char)cmp);
}

#if !defined(WIN32)
static void readdir_crash(void)
{
	DIR *dir;
	struct dirent *entry = NULL;

	/* Not checking if the return value is NULL. */
	dir = opendir("nonexisting_folder");

	do {
		/* This might cause a segmentation fault. And if it does not,
		 * we're not checking result of function call.
		 */
		entry = readdir(dir);
		/* And if the previous one doesn't crash, this one will try to
		 * dereference a null pointer.
		 */
		(void)printf("%s\n", entry->d_name);
	} while(entry != NULL);

	/* Not checking result of function call. closedir() can fail */
	closedir(dir);
}
#else
static void readdir_crash(void)
{

}
#endif

#define TEST_ARR_SIZE (4)
static char *test_arr[TEST_ARR_SIZE] = {
		"abc",
		"def",
		"ghi" /* Forgot to add a comma. */
		"jkl"
};

static void str_arr_initalising(void)
{
	int n;
	for(n = 0; n < TEST_ARR_SIZE; n++) {
		(void)printf("%s\n", test_arr[n]);
	}
}

static void parsing_err(void)
{
	/* atoi() will happily parse any garbage data and try to return a (meaningful) result. */
	const char *ptr = "abcdef";
	int val;
	errno = 0;
	val = atoi(ptr);
	/* It will also probably not set errno in case if the input was not valid. */
	(void)printf("Value is %d, errno is %d\n", val, errno);
	/* It can also happily crash in a case of NULL parameter. */
	errno = 0;
	val = atoi(strchr(ptr, '1'));
	(void)printf("Value is %d, errno is %d\n", val, errno);
	/* On some platforms atoi() is internally replaced by strtol(ptr, NULL, 10).
	 * It will not crash on NULL ptr parameter, but it will still silently try
	 * to parse a garbage input. */
}

static void string_search(void)
{
	const char *str = "1234567890";
	const char *ptr;
	size_t len = 0;
	ptr = strstr(str, "abc");
	/* Not checking if ptr is NULL. */
	len = strlen(ptr);
	(void)printf("Substring is at the position %u", (unsigned int)len);
}

/* Overflow (strncpy) because size_t is an unsigned value */
static void out_of_bounds_access(const char *str)
{
	char buf[64];
	size_t len;
	if(!str)
		return;

	len = strlen(str);
	if((len == 0) || (len > sizeof(buf) - 1))
		return;

	len -= 2;
	memset(buf, 0x00, sizeof(buf));
	strncpy(buf, str, len);
}

#define MAX_SIZE (6)
struct foo
{
	int arry[MAX_SIZE];
	int some_val;
};
/* Copy 5 integers from arr1 and one integer outside its boundary into foo.arry,
 * starting from foo.arry's third member.
 * Two array boundary issues and a potential stack corruption. This is a simplified
 * version of a real-world code. Instead of arr1 it was reading the data from
 * EEPROM.
 * gcc 10.3.0 and Valgrind did not see any issue here.
 */
static void buffer_overflow_and_stack_corruption(size_t size, struct foo *foo_param)
{
	int n;
	int arr1[] = {1,2,3,4,5};
	foo_param->some_val = 3;
	if(size > MAX_SIZE)
		size = MAX_SIZE;
	/* First two bytes were used for a header, and then someone tried to
	 * copy the data after the array boundary.
         */
	memcpy(foo_param->arry + 2, arr1, sizeof(int) * size);
	for(n = 0; n < MAX_SIZE; n++)
	{
		printf("%d ", foo_param->arry[n]);
	}
	printf("\n");
}

int main(void) {
	struct foo foo1;
	memset(&foo1, 0, sizeof(foo1));
	buffer_overflow_and_stack_corruption(8, &foo1);
	printf("some_val: %d\n", foo1.some_val);
	out_of_bounds_access("1");
	parsing_err();
	string_search();
	str_arr_initalising();
	buffer_overflow();
	readdir_crash();
	file_crash();
	return 0;
}

