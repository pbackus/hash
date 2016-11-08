#ifndef HASH_H
#define HASH_H

typedef struct hash_table Hash;

/* Constructor and destructor. */
Hash *hash_new(void);
void hash_delete(Hash *self);

/* Basic operations. */
void hash_set(Hash *self, const char *key, const int value);
int hash_get(const Hash *self, const char *key, int *value_out);
void hash_remove(Hash *self, const char *key);

/* Iteration. */
void hash_iterate(Hash *self, void (*callback)(const char*, int, void *),
                  void *context);

#endif
