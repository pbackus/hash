#include "hash.h"

#include <stdlib.h>
#include <string.h>

#define MIN_BUCKET_COUNT 32 /* Totally arbitrary */

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
	return make_hash(MIN_BUCKET_COUNT);
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

/* make_entry: allocate and initialize a new hash_entry structure
 *
 * Private helper function.
 */
static struct hash_entry *
make_entry(const char *key, const int value)
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

/* try_set: attempt to add a key, value pair to a hash table
 *
 * Returns 1 on success, 0 on failure.
 *
 * Private helper function.
 */
static int
try_set(Hash *self, const char *key, const int value)
{
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
		entry = make_entry(key, value);
		if (!entry) return 0;

		entry->next = self->buckets[i];
		self->buckets[i] = entry;

		self->load_factor += 1.0 / self->bucket_count;
	}

	return 1;
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

/* Context struct for rehash_callback */
struct rehash_ctx {
	int success;
	Hash *new_self;
};

/* rehash_callback: try to insert (k, v) into context->new_self
 *
 * If any of the inserts fail, context->success will be 0 at the end of
 * iteration.
 *
 * Callback function for hash_iterate.
 */
static void
rehash_callback(const char *k, int v, void *context)
{
	/* Unpack context */
	Hash *new_self = ((struct rehash_ctx *) context)->new_self;
	int *success = &((struct rehash_ctx*) context)->success;

	/* Attempt the insert and record the result */
	*success = *success && try_set(new_self, k, v);
}

/* hash_set: add a key, value pair to a hash table
 *
 * Public method.
 */
void
hash_set(Hash **selfp, const char *key, const int value)
{
	Hash *self = *selfp;

	if (!try_set(self, key, value)) goto error;

	/* Rehash if necessary */
	if (self->load_factor > GROW_THRESHOLD) {
		struct rehash_ctx result = {
			.success = 1,
			.new_self = make_hash(self->bucket_count * 2)
		};

		/* Defer rehashing if allocation fails */
		if (!result.new_self) return;

		hash_iterate(self, rehash_callback, &result);

		/* Defer rehashing if allocation fails */
		if (!result.success) {
			hash_delete(result.new_self);
			return;
		}

		*selfp = result.new_self;
		hash_delete(self);
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
	struct hash_entry **entryp;

	/* Search for key */
	entryp = &self->buckets[i];
	while (*entryp && strcmp(key, (*entryp)->key) != 0) {
		entryp = &(*entryp)->next;
	}

	if (*entryp) {
		/* Key found */
		struct hash_entry *entry = *entryp;

		*entryp = entry->next;

		free(entry->key);
		free(entry);

		self->load_factor -= 1.0 / self->bucket_count;
	}

	/* Rehash if necessary */
	if (self->load_factor < SHRINK_THRESHOLD &&
	    self->bucket_count > MIN_BUCKET_COUNT)
	{
		struct rehash_ctx result = {
			.success = 1,
			.new_self = make_hash(self->bucket_count / 2)
		};

		/* Defer rehashing if allocation fails */
		if (!result.new_self) return;

		hash_iterate(self, rehash_callback, &result);

		/* Defer rehashing if allocation fails */
		if (!result.success) {
			hash_delete(result.new_self);
			return;
		}

		*selfp = result.new_self;
		hash_delete(self);
	}

	return;
}
