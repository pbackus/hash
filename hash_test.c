#include "hash.h"
#include "minunit.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define array_size(a) (sizeof (a) / sizeof (*a))

int tests_run = 0;

char *
test_insert(void)
{
	HashTable h = Hash.new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	Hash.set(h, "foo", 123);

	int value;

	mu_assert("FAIL test_insert: key not found",
		Hash.get(h, "foo", &value)
		|| (Hash.delete(h), false) /* cleanup */);

	mu_assert("FAIL test_insert: incorrect value returned",
		value == 123
		|| (Hash.delete(h), false) /* cleanup */);

	Hash.delete(h);
	return NULL;
}

char *
test_retrieve(void)
{
	HashTable h = Hash.new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	Hash.set(h, "foo", 123);

	mu_assert("FAIL test_retrieve: NULL not handled correctly",
		Hash.get(h, "foo", NULL)
		|| (Hash.delete(h), false) /* cleanup */);

	mu_assert("FAIL test_retrieve: nonexistent key found",
		!Hash.get(h,"bar", NULL)
		|| (Hash.delete(h), false) /* cleanup */);

	Hash.delete(h);
	return NULL;
}

char *
test_update(void)
{
	HashTable h = Hash.new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	Hash.set(h, "foo", 123);
	Hash.set(h, "foo", 456);

	int value;

	mu_assert("FAIL test_update: key not found",
		Hash.get(h, "foo", &value)
		|| (Hash.delete(h), false) /* cleanup */);

	mu_assert("FAIL test_update: incorrect value returned",
		value == 456
		|| (Hash.delete(h), false) /* cleanup */);

	Hash.delete(h);
	return NULL;
}

char *
test_remove(void)
{
	HashTable h = Hash.new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	Hash.set(h, "foo", 123);
	Hash.remove(h, "foo");

	mu_assert("FAIL test_remove: removed key found",
		Hash.get(h, "foo", NULL) == 0
		|| (Hash.delete(h), false) /* cleanup */);

	Hash.delete(h);
	return NULL;
}

void
write_pair(const char *k, int v, void *ctx)
{
	char **bufpp = (char **) ctx;
	int n = sprintf(*bufpp, "%s=%d\n", k, v);
	*bufpp += n;
}

char *
test_each(void)
{
	HashTable h = Hash.new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	char possible_result[] = "foo=1\nbar=2\nbaz=3\n";
	char output_buf[array_size(possible_result)];
	char *bufp = output_buf;

	Hash.set(h, "foo", 1);
	Hash.set(h, "bar", 2);
	Hash.set(h, "baz", 3);

	Hash.each(h, write_pair, &bufp);
	output_buf[array_size(output_buf)] = '\0';

	mu_assert("FAIL test_each: unexpected result",
		(strstr(output_buf, "foo=1") &&
		 strstr(output_buf, "bar=2") &&
		 strstr(output_buf, "baz=3"))
		|| (Hash.delete(h), false) /* cleanup */);

	Hash.delete(h);
	return NULL;
}

char *
all_tests(void)
{
	mu_run_test(test_insert);
	mu_run_test(test_retrieve);
	mu_run_test(test_update);
	mu_run_test(test_remove);
	mu_run_test(test_each);

	return NULL;
}

int
main (int argc, char **argv)
{
	char *result = all_tests();

	if (result) puts(result);
	else puts("All tests passed.");

	printf("Tests run: %d\n", tests_run);

	return result != NULL;
}
