#ifndef HASH_H
#define HASH_H

typedef struct hash_table *HashTable;

struct hash_api {
	/* Constructor and destructor */
	HashTable (*new)(void);
	void (*delete)(HashTable self);

	/* Public methods */
	void (*set)(HashTable self, const char *key, const int value);
	int  (*get)(const HashTable self, const char *key, int *value_out);
	void (*remove)(HashTable self, const char *key);
	void (*each)(HashTable self, void (*callback)(const char *, int, void *),
	             void *context);
};

extern const struct hash_api Hash;

#endif
