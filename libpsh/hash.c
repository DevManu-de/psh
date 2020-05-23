/*
    libpsh/hash.c - hash table manage functions of the psh

    Copyright 2017 Zhang Maiyun.

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "libpsh/xmalloc.h"

/* Resize the hash table, the new size cannot be lower than the old size,
 * otherwise return 1. return 2 if realloc failed, 0 if success */
PSH_HASH *realloc_hash(PSH_HASH *table, unsigned int newlen)
{
    /* XXX: This algorithm uses a helping table */
    PSH_HASH *newtable = new_hash(newlen);
    unsigned int i;
    for (i = 0; i < table->len; ++i)
    {
        unsigned int j;
        if (table[i].key != NULL)
        {
            /* rehash */
            add_hash(&newtable, table[i].key, table[i].val);
        }
        for (j = 0; j < table[i].next_count; ++j)
        {
            add_hash(&newtable, table[i].nexts[j].key, table[i].nexts[j].val);
        }
    }
    del_hash(table);
    return newtable;
}

/* Allocate a new hash table, return the table if success, NULL if not */
PSH_HASH *new_hash(unsigned int len)
{
    unsigned int i;
    PSH_HASH *table;
    if (len == 0)
        return xmalloc(0); /* Ask the libc malloc for a value */

    table = xmalloc(sizeof(PSH_HASH) * len);

    table[0].len = len;

    for (i = 0; i < len; ++i)
    {
        table[i].key = table[i].val = NULL;
        table[i].used = 0;
        table[i].next_count = 0;
    }

    return table;
}

/* Edit the value of an element, return 0 if success, 1 if not */
static int edit_hash_elem(PSH_HASH *elem, char *val)
{
    char *tmp;
    /* Need more space/free extra space */
    tmp = xrealloc(elem->val, strlen(val) + 1);
    elem->val = tmp;
    strcpy(elem->val, val);
    return 0;
}

/* Add or edit a hash element, return 0 if success, 1 if not */
int add_hash(PSH_HASH **arg_table, char *key, char *val)
{
    PSH_HASH *avail, *table = *arg_table;
    int i;
    unsigned int hash_result;
recheck:
    hash_result = hasher(key, table->len);
    if (table[hash_result].used != 0)
    {
        /* Doing edit */
        if (strcmp(table[hash_result].key, key) == 0)
        {
            return edit_hash_elem(&table[hash_result], val);
        }
        /* else */
        /* save to nexts */
        if (table[hash_result].next_count + 1 == 64) /*maximum exceeded*/
        {
            PSH_HASH *tmp;
            tmp = realloc_hash(table,
                               ((table->len) << 1) +
                                   1); /*get an approx. twice bigger table */
            if (tmp == NULL)
                return 1;
            table = tmp;
            goto recheck;
        }
        if (table[hash_result].next_count == 0) /* No elements in nexts */
        {
            table[hash_result].nexts = xmalloc(sizeof(PSH_HASH) * 64);
        }
        avail = &(table[hash_result].nexts[(
            table[hash_result].next_count)++]); /* The first blank element */
        avail->used = 1;
        avail->key = malloc(P_CS * (strlen(key) + 1));
        strcpy(avail->key, key);
        return edit_hash_elem(avail, val);
    }
    table[hash_result].used = 1;
    table[hash_result].key = malloc(P_CS * (strlen(key) + 1));
    strcpy(table[hash_result].key, key);
    /* Write element */
    i = edit_hash_elem(&table[hash_result], val);
    *arg_table = table; /* Apply changes to the pointer if there are any */
    return i;
}

/* Get a hash value by key, return value if success, NULL if not */
char *get_hash(PSH_HASH *table, char *key)
{
    unsigned int hash_result = hasher(key, table->len);
    if (table[hash_result].key != NULL)
    {
        if (strcmp(table[hash_result].key, key) == 0)
            return table[hash_result].val;
        else
        {
            unsigned int i;
            for (i = 0; i < table[hash_result].next_count; ++i)
            {
                if (strcmp(table[hash_result].nexts[i].key, key) == 0)
                    return table[hash_result].nexts[i].val;
            }
        }
    }
    return NULL;
}

/* Remove an element from the hash table, return 0 if success, 1 if specified
 * element not found */
int rm_hash(PSH_HASH *table, char *key)
{
    unsigned int hash_result = hasher(key, table->len);
    if (strcmp(table[hash_result].key, key) ==
        0) /* Deleting the first element */
    {
        if (table[hash_result].used == 0)
            return 1;
        table[hash_result].used = 0;
        free(table[hash_result].key);
        table[hash_result].key = NULL;
        free(table[hash_result].val);
        table[hash_result].val = NULL;
        if (table[hash_result].next_count != 0)
        {
            /* Move the last element in nexts here */
            unsigned int i = table[hash_result].next_count - 1;
            add_hash(&table, table[hash_result].nexts[i].key,
                     table[hash_result].nexts[i].val);
            table[hash_result].nexts[i].used = 0;
            table[hash_result].next_count--;
        }
        return 0;
    }
    else
    {
        unsigned int i;
        for (i = 0; i < table[hash_result].next_count; ++i)
        {
            if (strcmp(table[hash_result].nexts[i].key, key) == 0)
            {
                free(table[hash_result].nexts[i].key);
                table[hash_result].nexts[i].key = NULL;
                free(table[hash_result].nexts[i].val);
                table[hash_result].nexts[i].val = NULL;
                if ((i + 1) != table[hash_result].next_count)
                {
                    /* Move the last element in nexts here */
                    i = table[hash_result].next_count - 1;
                    add_hash(&table, table[hash_result].nexts[i].key,
                             table[hash_result].nexts[i].val);
                    table[hash_result].nexts[i].used = 0;
                    table[hash_result].next_count--;
                }
            }
        }
    }
    return 0;
}

/* Free a hash table. This function don’t return a value */
void del_hash(PSH_HASH *table)
{
    unsigned int i;
    for (i = 0; i < (table->len); ++i)
    {
        if (table[i].used != 0)
        {
            unsigned int j;
            free(table[i].key);
            free(table[i].val);
            for (j = 0; j < table[i].next_count; ++j)
            {
                free(table[i].nexts[j].key);
                free(table[i].nexts[j].val);
            }
            if (table[i].next_count != 0)
                free(table[i].nexts);
        }
    }
    free(table);
}
