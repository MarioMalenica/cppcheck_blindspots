/* A collection of common C code errors and pitfalls
 * which are not detected by cppcheck.
 *
 * They are also not detected by gcc when compiled with
 * the following options:
 * -pedantic -Wall -Wextra -Wconversion
 *
 * Neither cppcheck or gcc were able to find a single error
 * or warning in this code.
 *
 * Note: Comments are valid for cppcheck version 1.86.
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
	char *foo = "12345abcde";
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
	struct dirent *entry;
	struct dirent *prev_entry = NULL;
	/* Suppress compiler warning */
	(void) prev_entry;

	/* Just allocate some size large enough for this test
	 * and avoid subtle differences between different OSs
	 * related to NAME_MAX size.
	 */
	entry = (struct dirent*) calloc((size_t)1, (size_t)0xFFFF);
	if(entry == NULL)
		return;

	/* Not checking if the return value is NULL. */
	dir = opendir("nonexisting_folder");

	/* This can cause a segmentation fault if dir is NULL. */
	while ((readdir_r(dir, entry, &prev_entry)) == 0) {
		(void)printf("%s\n", entry->d_name);
	}

	/* Not checking result of function call. closedir() can fail */
	closedir(dir);
	free(entry);

	/* Note: cppcheck flags readdir() as not being thread safe.
	 * At the same time glibc (version >= 2.24) is treating
	 * readdir_r as deprecated and readdir()/readdir64() as thread-safe */
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
	char *ptr = "abcdef";
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
	char *str = "1234567890";
	char *ptr;
	size_t len = 0;
	ptr = strstr(str, "abc");
	/* Not checking if ptr is NULL. */
	len = strlen(ptr);
	(void)printf("Substring is at the position %u", (unsigned int)len);
}

/* Overflow (strncpy) because size_t is an unsigned value */
static void out_of_bounds_access(char *str)
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

int main(void) {
	out_of_bounds_access("1");
	parsing_err();
	string_search();
	str_arr_initalising();
	buffer_overflow();
	readdir_crash();
	file_crash();
	return 0;
}

