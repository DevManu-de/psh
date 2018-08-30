/*
   hash.h - hash functions and structures

   Copyright 2017 Zhang Maiyun.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdio.h>

typedef struct psh_hash_struct
{
    char *key;
    char *val;
    unsigned used : 1;
    unsigned int len;              /* Only used in the first element */
    unsigned next_count : 6;       /* count for nexts */
    struct psh_hash_struct *nexts; /* array with 64 elements */
} PSH_HASH;

PSH_HASH *realloc_hash(PSH_HASH *, unsigned int);
PSH_HASH *new_hash(unsigned int);
int add_hash(PSH_HASH **, char *, char *);
char *get_hash(PSH_HASH *, char *);
int rm_hash(PSH_HASH *, char *);
void del_hash(PSH_HASH *);

unsigned int hasher(const char *s, unsigned int ulimit);
