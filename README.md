Simple C Hash Table
===================

This is a simple hash table I wrote originally for use in a class project. It
uses strings as keys and integers as values. Collisions are resolved using
separate chaining with linked lists.

Design goals:

- Amortized O(1) insert/lookup/remove.
- Ease of use.

Example
-------

    Hash *h = hash_new();

    hash_set(&h, "foo", 413);
    hash_set(&h, "bar", 612);
    hash_set(&h, "baz", 1025);

    hash_remove(&h, "bar");

    int v;
    assert(hash_get(h, "foo", &v) == 1);
    assert(v == 413);
    assert(hash_get(h, "bar", NULL) == 0);
    assert(hash_get(h, "baz", &v) == 1);
    assert(v == 1025);

    hash_delete(h);
