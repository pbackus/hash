#include "hash.h"
#include "minunit.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define array_size(a) (sizeof (a) / sizeof (*a))

int tests_run = 0;

char *
test_insert()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	hash_set(&h, "foo", 123);

	int value;

	mu_assert("FAIL test_insert: key not found",
		hash_get(h, "foo", &value)
		|| (hash_delete(h), false) /* cleanup */);

	mu_assert("FAIL test_insert: incorrect value returned",
		value == 123
		|| (hash_delete(h), false) /* cleanup */);

	hash_delete(h);
	return NULL;
}

char *
test_retrieve()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	hash_set(&h, "foo", 123);

	mu_assert("FAIL test_retrieve: NULL not handled correctly",
		hash_get(h, "foo", NULL)
		|| (hash_delete(h), false) /* cleanup */);

	mu_assert("FAIL test_retrieve: nonexistent key found",
		!hash_get(h,"bar", NULL)
		|| (hash_delete(h), false) /* cleanup */);

	hash_delete(h);
	return NULL;
}

char *
test_update()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	hash_set(&h, "foo", 123);
	hash_set(&h, "foo", 456);

	int value;

	mu_assert("FAIL test_update: key not found",
		hash_get(h, "foo", &value)
		|| (hash_delete(h), false) /* cleanup */);

	mu_assert("FAIL test_update: incorrect value returned",
		value == 456
		|| (hash_delete(h), false) /* cleanup */);

	hash_delete(h);
	return NULL;
}

char *
test_remove()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	hash_set(&h, "foo", 123);
	hash_remove(&h, "foo");

	mu_assert("FAIL test_remove: removed key found",
		hash_get(h, "foo", NULL) == 0
		|| (hash_delete(h), false) /* cleanup */);

	hash_delete(h);
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
test_iterate()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	char possible_result[] = "foo=1\nbar=2\nbaz=3\n";
	char output_buf[array_size(possible_result)];
	char *bufp = output_buf;

	hash_set(&h, "foo", 1);
	hash_set(&h, "bar", 2);
	hash_set(&h, "baz", 3);

	hash_iterate(h, write_pair, &bufp);
	output_buf[array_size(output_buf)] = '\0';

	mu_assert("FAIL test_iterate: unexpected result",
		(strstr(output_buf, "foo=1") &&
		 strstr(output_buf, "bar=2") &&
		 strstr(output_buf, "baz=3"))
		|| (hash_delete(h), false) /* cleanup */);

	hash_delete(h);
	return NULL;
}

char *
test_grow()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	char buf[3];

	/* 100 inserts is enough to trigger a rehash */
	for (int i = 0; i < 100; i++) {

		/* Defensive coding */
		if (snprintf(buf, array_size(buf), "%d", i) >= array_size(buf)) {
			buf[array_size(buf) - 1] = '\0';
		}

		hash_set(&h, buf, i);
	}

	int value;

	/* Check to make sure everything's still there */
	for (int i = 0; i < 100; i++) {

		/* Defensive coding */
		if (snprintf(buf, array_size(buf), "%d", i) >= array_size(buf)) {
			buf[array_size(buf) - 1] = '\0';
		}

		mu_assert("FAIL test_grow: key not found",
			hash_get(h, buf, &value)
			|| (hash_delete(h), false) /* cleanup */);

		mu_assert("FAIL test_grow: incorrect value returned",
			value == i
			|| (hash_delete(h), false) /* cleanup */);
	}

	hash_delete(h);
	return NULL;
}

char *
test_shrink()
{
	Hash *h = hash_new();
	if (!h) { puts("Fatal error: out of memory"); abort(); }

	char buf[3];

	/* 100 inserts is enough to trigger a rehash */
	for (int i = 0; i < 100; i++) {

		/* Defensive coding */
		if (snprintf(buf, array_size(buf), "%d", i) >= array_size(buf)) {
			buf[array_size(buf) - 1] = '\0';
		}

		hash_set(&h, buf, i);
	}

	/* Remove 90% of entries to trigger another rehash */
	for (int i = 0; i < 100; i++) {
		if (i % 10 == 5) continue; /* Keep these ones */

		/* Defensive coding */
		if (snprintf(buf, array_size(buf), "%d", i) >= array_size(buf)) {
			buf[array_size(buf) - 1] = '\0';
		}

		hash_remove(&h, buf);
	}

	int value;

	/* Check to make sure the ones we kept are still there */
	for (int i = 0; i < 100; i++) {
		if (i % 10 != 5) continue;

		/* Defensive coding */
		if (snprintf(buf, array_size(buf), "%d", i) >= array_size(buf)) {
			buf[array_size(buf) - 1] = '\0';
		}

		mu_assert("FAIL test_shrink: key not found",
			hash_get(h, buf, &value)
			|| (hash_delete(h), false) /* cleanup */);

		mu_assert("FAIL test_shrink: incorrect value returned",
			value == i
			|| (hash_delete(h), false) /* cleanup */);
	}

	hash_delete(h);
	return NULL;
}

char *
all_tests()
{
	mu_run_test(test_insert);
	mu_run_test(test_retrieve);
	mu_run_test(test_update);
	mu_run_test(test_remove);
	mu_run_test(test_iterate);
	mu_run_test(test_grow);
	mu_run_test(test_shrink);

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
