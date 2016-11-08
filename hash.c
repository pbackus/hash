#include "hash.h"

#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 32 /* Totally arbitraty */

struct hash_entry {
	char *key;
	int value;
	struct hash_entry *next;
};

struct hash_table {
	struct hash_entry *buckets[BUCKET_COUNT];
};

/* djb2 hash function
 * From http://www.cse.yorku.ca/~oz/hash.html
 */
static unsigned long
hash(const char *key)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *key++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

/* hash_new: allocate and initialize a new hash
 *
 * Public method.
 */
Hash *
hash_new(void)
{
	Hash *h = malloc(sizeof *h);
	if (!h) return NULL;

	for (size_t i = 0; i < BUCKET_COUNT; i++) {
		h->buckets[i] = NULL;
	}

	return h;
}

/* delete_bucket: free an entire "chain" of hash entries
 *
 * Private helper function.
 */
static void
delete_bucket(struct hash_entry *head)
{
	struct hash_entry *next;

	while (head) {
		next = head->next;
		free(head->key);
		free(head);
		head = next;
	}
}

/* hash_delete: free all memory associated with a hash
 *
 * Public method.
 */
void
hash_delete(Hash *self)
{
	if (!self) return;

	for (size_t i = 0; i < BUCKET_COUNT; i++) {
		delete_bucket(self->buckets[i]);
	}

	free(self);
}

/* new_entry: allocate and initialize a new hash_entry structure
 *
 * Private helper function.
 */
static struct hash_entry *
new_entry(const char *key, const int value)
{
	struct hash_entry *p = NULL;

	p = malloc(sizeof *p);
	if (p == NULL) goto error;

	p->key = malloc(strlen(key) + 1);
	if (p->key == NULL) goto error;

	strcpy(p->key, key);
	p->value = value;
	p->next = NULL;

	return p;

error:
	free(p);
	return NULL;
}

/* hash_set: add a key, value pair to a hash
 *
 * Public method.
 */
void
hash_set(Hash *self, const char *key, const int value)
{
	size_t i = hash(key) % BUCKET_COUNT;
	struct hash_entry *entry;

	/* Search for key */
	entry = self->buckets[i];
	while (entry && strcmp(key, entry->key) !=0)
		entry = entry->next;

	if (entry) {
		/* Key found */
		entry->value = value;
	} else {
		/* Key not found */
		entry = new_entry(key, value);
		if (!entry) goto error;

		entry->next = self->buckets[i];
		self->buckets[i] = entry;
	}

	return;

error:
	/* Out of memory */
	/* FIXME: is there a better way to handle this? */
	abort();
}

/* hash_get: search for a key in a hash
 *
 * Returns 1 if the key is found, and 0 if not. Additionally, if the key is
 * found and value_out is non-null, the corresponding value is stored there.
 *
 * Public method.
 */
int
hash_get(const Hash *self, const char *key, int *value_out)
{
	size_t i = hash(key) % BUCKET_COUNT;
	struct hash_entry *entry;

	/* Search for key */
	entry = self->buckets[i];
	while (entry && strcmp(key, entry->key) != 0)
		entry = entry->next;

	if (entry) {
		/* Key found */
		if (value_out) *value_out = entry->value;
		return 1;
	} else {
		/* Key not found */
		return 0;
	}
}

/* hash_remove: remove a key and its associated value from a hash
 *
 * Also frees the memory associated with the removed pair.
 *
 * Public method.
 */
void
hash_remove(Hash *self, const char *key)
{
	size_t i = hash(key) % BUCKET_COUNT;
	struct hash_entry *entry, *prev;

	/* Search for key */
	entry = self->buckets[i];
	prev = NULL;
	while (entry && strcmp(key, entry->key) != 0) {
		prev = entry;
		entry = entry->next;
	}

	if (entry) {
		/* Key found */
		if (prev) prev->next = entry->next;
		else self->buckets[i] = entry->next;

		free(entry->key);
		free(entry);
	}
}

/* hash_iterate: iterate over pairs in a hash table
 *
 * Public method.
 */
void
hash_iterate(Hash *self, void (*callback)(const char *, int, void *),
             void *context)
{
	for (size_t i = 0; i < BUCKET_COUNT; i++)
		for (struct hash_entry *e = self->buckets[i]; e; e = e->next)
			callback(e->key, e->value, context);
}
