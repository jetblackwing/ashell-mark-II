/*
 *    A-Shell Project - Performance Optimizations
 *    file: cache.c
 *    
 *    Implementation of command cache for PATH lookups
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "error.h"

static cache_entry_t *cache[CACHE_SIZE];
static unsigned int cache_entries = 0;
static unsigned int total_hits = 0;
static unsigned int total_misses = 0;

/**
 * Initialize the command cache
 */
void cache_init(void)
{
    memset(cache, 0, sizeof(cache));
    cache_entries = 0;
    total_hits = 0;
    total_misses = 0;
}

/**
 * Look up a command in cache
 */
char* cache_lookup(const char *command)
{
    if (!command || cache_entries == 0) {
        total_misses++;
        return NULL;
    }
    
    for (unsigned int i = 0; i < cache_entries; i++) {
        if (cache[i] && strcmp(cache[i]->command, command) == 0) {
            cache[i]->hits++;
            total_hits++;
            return cache[i]->path;
        }
    }
    
    total_misses++;
    return NULL;
}

/**
 * Add an entry to cache
 */
void cache_add(const char *command, const char *path)
{
    if (!command || cache_entries >= CACHE_SIZE) {
        return;
    }
    
    /* Check if already cached */
    for (unsigned int i = 0; i < cache_entries; i++) {
        if (cache[i] && strcmp(cache[i]->command, command) == 0) {
            /* Update existing entry */
            free(cache[i]->path);
            cache[i]->path = path ? strdup(path) : NULL;
            return;
        }
    }
    
    /* Add new entry */
    cache_entry_t *entry = malloc(sizeof(cache_entry_t));
    if (!entry) {
        SHELL_ERROR("cache: memory allocation failed");
        return;
    }
    
    entry->command = strdup(command);
    entry->path = path ? strdup(path) : NULL;
    entry->hits = 0;
    
    if (!entry->command) {
        free(entry);
        SHELL_ERROR("cache: memory allocation failed");
        return;
    }
    
    cache[cache_entries++] = entry;
}

/**
 * Clear the cache
 */
void cache_clear(void)
{
    for (unsigned int i = 0; i < cache_entries; i++) {
        if (cache[i]) {
            free(cache[i]->command);
            free(cache[i]->path);
            free(cache[i]);
            cache[i] = NULL;
        }
    }
    cache_entries = 0;
}

/**
 * Get cache statistics
 */
void cache_stats(unsigned int *hits, unsigned int *misses, unsigned int *entries)
{
    if (hits) *hits = total_hits;
    if (misses) *misses = total_misses;
    if (entries) *entries = cache_entries;
}
