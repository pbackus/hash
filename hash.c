#include "hash.h"

#include <stdlib.h>
#include <string.h>

#define INIT_BUCKET_COUNT 32 /* Totally arbitrary */

/* Chosen based on these notes from Cornell's data structures course:
 *   https://www.cs.cornell.edu/Courses/cs312/2008sp/lectures/lec20.html
 * We're not super concerned with performance, so any value that is basically
 * sane will do here.
 */
#define GROW_THRESHOLD 1.5
#define SHRINK_THRESHOLD (GROW_THRESHOLD / 4.0)

struct hash_entry {
	char *key;
	int value;
	struct hash_entry *next;
};

struct hash_table {
	double load_factor;
	size_t bucket_count;
	struct hash_entry *buckets[]; /* Flexible array */
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

/* make_hash: allocate and initialize a hash table
 *
 * Private helper function.
 */
static Hash *
make_hash(size_t bucket_count)
{
	Hash *h = malloc(sizeof *h + bucket_count * sizeof h->buckets[0]);
	if (!h) return NULL;

	h->load_factor = 0.0;
	h->bucket_count = bucket_count;

	for (size_t i = 0; i < h->bucket_count; i++) {
		h->buckets[i] = NULL;
	}

	return h;
}

/* hash_new: create a new, empty hash table
 *
 * Public method.
 */
Hash *
hash_new()
{
	return make_hash(INIT_BUCKET_COUNT);
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

/* hash_delete: free all memory associated with a hash table
 *
 * Public method.
 */
void
hash_delete(Hash *self)
{
	if (!self) return;

	for (size_t i = 0; i < self->bucket_count; i++) {
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

/* copy_pair_to: insert (k, v) into the hash table given as "context"
 *
 * Callback function for hash_iterate.
 */
static void
copy_pair_to(const char *k, int v, void *context)
{
	Hash *h = context;
	hash_set(&h, k, v);
}

/* hash_set: add a key, value pair to a hash
 *
 * Public method.
 */
void
hash_set(Hash **selfp, const char *key, const int value)
{
	Hash *self = *selfp;

	size_t i = hash(key) % self->bucket_count;
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

		self->load_factor += 1.0 / self->bucket_count;

		/* Rehash if necessary */
		if (self->load_factor > GROW_THRESHOLD) {
			Hash *new_self = make_hash(self->bucket_count * 2);
			if (!new_self) goto error;

			hash_iterate(self, copy_pair_to, new_self);

			*selfp = new_self;
			hash_delete(self);
		}
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
 * found and value_out is non-NULL, the corresponding value is stored in the
 * space it points to.
 *
 * Public method.
 */
int
hash_get(const Hash *self, const char *key, int *value_out)
{
	size_t i = hash(key) % self->bucket_count;
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

/* hash_remove: remove a key and its associated value from a hash table
 *
 * Also frees the memory associated with the removed pair.
 *
 * Public method.
 */
void
hash_remove(Hash **selfp, const char *key)
{
	Hash *self = *selfp;

	size_t i = hash(key) % self->bucket_count;
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

		self->load_factor -= 1.0 / self->bucket_count;

		/* Rehash if necessary */
		if (self->load_factor < SHRINK_THRESHOLD) {
			Hash *new_self = make_hash(self->bucket_count / 2);
			if (!new_self) goto error;

			hash_iterate(self, copy_pair_to, new_self);

			*selfp = new_self;
			hash_delete(self);
		}
	}

	return;

error:
	/* Out of memory */
	/* FIXME: is there a better way to handle this? */
	abort();
}

/* hash_iterate: iterate over pairs in a hash table
 *
 * Public method.
 */
void
hash_iterate(Hash *self, void (*callback)(const char *, int, void *),
             void *context)
{
	for (size_t i = 0; i < self->bucket_count; i++)
		for (struct hash_entry *e = self->buckets[i]; e; e = e->next)
			callback(e->key, e->value, context);
}
