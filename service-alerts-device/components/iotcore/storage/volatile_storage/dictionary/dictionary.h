#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "esp_err.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct dictionary dictionary_t;

typedef struct dict_entry {
  char *key;
  int value;
  struct dict_entry *next;
} dict_entry_t;

typedef struct dictionary {
  dict_entry_t **entries;
} dictionary_t;

dictionary_t *create_dictionary();
void set_value(dictionary_t *dictionary, const char *key, int value);
esp_err_t get_value(dictionary_t *dictionary, const char *key, uint32_t *value);
void free_dictionary(dictionary_t *dictionary);
int get_size(dictionary_t *dictionary);
dict_entry_t *find_entry(dict_entry_t *entry, const char *key);
unsigned int hash(const char *key);

#endif