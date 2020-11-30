/*
    libpsh/hash.c - hash table manage functions of the psh

    Copyright 2020 Zhang Maiyun.

    This file is part of Psh, P shell.

    Psh is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Psh is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "libpsh/hash.h"
#include "libpsh/util.h"
#include "libpsh/xmalloc.h"

#define FULL_RATE 0.7

/* Variable naming in this file:
    table: the psh_hash or _psh_hash_container structure in operation;
    using: the _psh_hash_internal structure currently in operation or iterating
over;
    this: the _psh_hash_item structure currently in operation or iterating
over.
*/

/** Find the next power of 2, for 64-bit types
 * @note from
 * http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * @param v The number to use
 * @return The next power of 2 after @ref v */
static inline size_t ceil_pow2(size_t v)
{
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
}

/** Add an allocated and filled struct _psh_hash_item to a table.
 * @note @ref item->next should be NULL or empty.
 * @param table The table to which to add.
 * @param item Pointer to the struct _psh_hash_item to add.
 * @return 0 if succeeded, 1 if not.
 */
static int add__psh_hash_item(psh_hash *table, struct _psh_hash_item *item)
{
    struct _psh_hash_internal *using;

    using = &(table->table[item->hash & (table->len - 1)]);
    if (using->used == 0)
        /* This hash value is still empty, initialize, and put the key-value
         * pair to add to the first place */
        using->head = using->tail = item;
    else
        using->tail = using->tail->next = item;
    using->used++;
    table->used++;
    return 0;
}

/* Allocate a new hash table, return the table if succeeded */
psh_hash *psh_hash_create(size_t len)
{
    psh_hash *table = xmalloc(sizeof(psh_hash));

    /* Use power of 2 as the length */
    len = ceil_pow2(len);
    table->len = len;
    table->used = 0;
    /* zero length gets handled too */
    /* NUL-init the internal table, because USED needs to be zero for
     * psh_hash_add to function */
    table->table = xcalloc(len, sizeof(struct _psh_hash_internal));

    return table;
}

/* Same as psh_hash_add, but resizes the hash table if the number of items gets
greater.
 * OLD: Table is potentially modified so a reference is passed in.
 * Table is not modified as psh_hash_realloc only modifies internal values.
 */
int psh_hash_add_chk(psh_hash *table, const char *key, void *value, int if_free)
{
    /* '=' for initial allocation with a length of zero */
    if (FULL_RATE * table->len <= table->used)
    {
        /* The table is almost full, performance degrades */
        size_t newlen;
        if (table->len == 0)
            newlen = 16;
        else
            newlen = table->len << 1;
        if (newlen < table->len) /* Integer overflow */
            newlen = table->len;
        psh_hash_realloc(table, newlen);
    }
    return psh_hash_add(table, key, value, if_free);
}

/* Add or edit a hash element.
 * If IF_FREE is set, VALUE will be free()d upon the
 * deallocation of the hash table. Returns 0 if succeeded, 1 if not */
int psh_hash_add(psh_hash *table, const char *key, void *value, int if_free)
{
    size_t hash_result;
    struct _psh_hash_internal *using;
    struct _psh_hash_item *item;

    hash_result = hasher(key);
    using = &(table->table[hash_result & (table->len - 1)]);
    if (using->used != 0)
    {
        /* This hash value has been taken, first try to find duplicate keys and
         * edit, then, if that failed, add a new one to the linked-list */
        size_t count;
        struct _psh_hash_item *this = using->head;
        /* Iterate over the existing ones to see if an edit should occur, also
         * filtering out any duplicate keys */
        for (count = 0; count < using->used; ++count)
        {
            /* Comparing the hash which is an integer first */
            if (hash_result == this->hash && strcmp(key, this->key) == 0)
            {
                if (this->if_free)
                    xfree(this->value);
                this->hash = hash_result;
                this->if_free = if_free;
                this->value = value;
                /* Skip any further iterates, as every items should have been
                 * done this with before being added */
                return 0;
            }
            /* Last one: this->next is random, but the corresponding loop
             * iterate won't be run, so no SEGV.
             * "++count" gets run before evaluating "(count < using->used)", so
             * count == using->used indicates that the for loop reached the end
             * without finding a duplicate key */
            this = this->next;
        } /* for */
    }
    /* No duplicate keys found, append the key-value pair to add to the table */
    item = xmalloc(sizeof(struct _psh_hash_item));
    item->hash = hash_result;
    item->if_free = if_free;
    /* Duplicate key to prevent further modification */
    item->key = psh_strdup(key);
    item->next = NULL;
    item->value = value;
    return add__psh_hash_item(table, item);
}

/* Get a hash value by key, return value if success, NULL if not */
void *psh_hash_get(psh_hash *table, const char *key)
{
    struct _psh_hash_internal *using;
    struct _psh_hash_item *this;
    size_t count;

    using = &(table->table[hasher(key) & (table->len - 1)]);
    this = using->head;

    for (count = 0; count < using->used; ++count)
    {
        if (strcmp(key, this->key) == 0)
            return this->value;
        /* Same discussion as above */
        this = this->next;
    }
    return NULL;
}

/* Resize the hash table.
 * Only the array object is reallocated, the table structure remains the same
 * one, and the _psh_hash_item structures are all reused.
 * Therefore, table is not modified, so the return value doesn't need to be
 * checked.
 */
psh_hash *psh_hash_realloc(psh_hash *table, size_t newlen)
{
    struct _psh_hash_internal *using;
    size_t count, oldlen = table->len;
    struct _psh_hash_internal *oldtable = table->table;
#ifdef DEBUG
    fprintf(stderr, "[hash] realloc %zu\n", ceil_pow2(newlen));
#endif

    newlen = ceil_pow2(newlen);
    table->len = newlen;
    /* NUL-init the internal table, because USED needs to be zero for
     * psh_hash_add to function */
    table->table = xcalloc(newlen, sizeof(struct _psh_hash_internal));
    table->used = 0;
    /* Go through the old table and settle the items into the new table */
    for (using = oldtable; using < oldtable + oldlen; ++using)
    {
        struct _psh_hash_item *this = using->head;
        for (count = 0; count < using->used; ++count)
        {
            add__psh_hash_item(table, this);
            this = this->next;
        }
    }
    /* free the old table array but not items or values */
    xfree(oldtable);

    return table;
}

/* Remove an element from the hash table, return 0 if success, 1 if specified
 * item not found */
int psh_hash_rm(psh_hash *table, const char *key)
{
    struct _psh_hash_internal *using;
    struct _psh_hash_item *this, *old_this;
    size_t count;

    using = &(table->table[hasher(key) & (table->len - 1)]);
    old_this = NULL;
    this = using->head;

    for (count = 0; count < using->used; ++count)
    {
        if (strcmp(key, this->key) == 0)
        {
            xfree(this->key);
            xfree(this->value);
            if (!old_this)
            {
                /* Removing the first element, using->head == this */
                using->head = this->next;
                /* No modification to tail needed */
            }
            else
                old_this->next = this->next;

            /* Decrease local count */
            using->used--;
            /* Decrease global count */
            table->used--;
            /* Deallocate linked-list item */
            xfree(this);
            return 0;
        }
        /* Same discussion */
        old_this = this;
        this = this->next;
    }
    return 1;
}

/* Free a hash table. */
void psh_hash_free(psh_hash *table)
{
    struct _psh_hash_internal *using;
    struct _psh_hash_item *this, *tmp;
    size_t count;

    if (table == NULL)
        return;
    for (using = table->table; using < table->table + table->len; ++using)
    {
        this = using->head;
        for (count = 0; count < using->used; ++count)
        {
            xfree(this->key);
            if (this->if_free)
                xfree(this->value);
            tmp = this;
            this = this->next;
            xfree(tmp);
        }
    }
    xfree(table->table);
    xfree(table);
}
