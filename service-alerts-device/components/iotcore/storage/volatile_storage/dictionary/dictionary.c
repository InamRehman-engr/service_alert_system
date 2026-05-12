#include "dictionary.h"

#define TABLE_SIZE 100
dictionary_t *create_dictionary() {
  dictionary_t *dictionary = malloc(sizeof(dictionary_t));
  dictionary->entries = calloc(TABLE_SIZE, sizeof(dict_entry_t *));
  return dictionary;
}

unsigned int hash(const char *key) {
  unsigned int hash = 0;
  for (int i = 0; key[i] != '\0'; i++) {
    hash = (hash * 31 + key[i]) % TABLE_SIZE;
  }
  return hash;
}

dict_entry_t *find_entry(dict_entry_t *entry, const char *key) {
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}
void set_value(dictionary_t *dictionary, const char *key, int value) {
  unsigned int hash_value = hash(key);
  dict_entry_t *entry = dictionary->entries[hash_value];
  if (entry == NULL) {
    // No collision, create a new entry
    entry = malloc(sizeof(dict_entry_t));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;
    dictionary->entries[hash_value] = entry;
  } else {
    // Handle collision by appending to linked list
    dict_entry_t *existing_entry = find_entry(entry, key);
    if (existing_entry != NULL) {
      // Key already exists, update value
      existing_entry->value = value;
    } else {
      // Key does not exist, create a new entry
      dict_entry_t *new_entry = malloc(sizeof(dict_entry_t));
      new_entry->key = strdup(key);
      new_entry->value = value;
      new_entry->next = entry;
      dictionary->entries[hash_value] = new_entry;
    }
  }
}
esp_err_t get_value(dictionary_t *dictionary, const char *key,
                    uint32_t *value) {
  unsigned int hash_value = hash(key);
  dict_entry_t *entry = dictionary->entries[hash_value];
  dict_entry_t *found_entry = find_entry(entry, key);
  if (found_entry != NULL) {
    *value = found_entry->value;
    return ESP_OK;
  }
  return ESP_ERR_NOT_FOUND;
}
void free_dictionary(dictionary_t *dictionary) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    dict_entry_t *entry = dictionary->entries[i];
    while (entry != NULL) {
      dict_entry_t *next_entry = entry->next;
      free(entry->key);
      free(entry);
      entry = next_entry;
    }
  }
  free(dictionary->entries);
  free(dictionary);
}
int get_size(dictionary_t *dictionary) {
  int count = 0;
  for (int i = 0; i < TABLE_SIZE; i++) {
    dict_entry_t *entry = dictionary->entries[i];
    while (entry != NULL) {
      count++;
      entry = entry->next;
    }
  }
  return count;
}
