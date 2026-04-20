/*
 *    A-Shell Project - Performance Optimizations
 *    file: cache.h
 *    
 *    Command caching for faster PATH lookups
 */

#ifndef CACHE_H
#define CACHE_H

#define CACHE_SIZE 256

/**
 * Cache entry for command paths
 */
typedef struct {
    char *command;          /* Command name */
    char *path;             /* Full path or NULL if not found */
    unsigned int hits;      /* Lookup count */
} cache_entry_t;

/**
 * Initialize the command cache
 */
void cache_init(void);

/**
 * Look up a command in cache
 * Returns: Full path if found in cache, NULL if not cached
 */
char* cache_lookup(const char *command);

/**
 * Add an entry to cache
 * @param command - Command name
 * @param path - Full path (or NULL if not found)
 */
void cache_add(const char *command, const char *path);

/**
 * Clear the cache
 */
void cache_clear(void);

/**
 * Get cache statistics
 * @param hits - Output: Total cache hits
 * @param misses - Output: Total cache misses  
 * @param entries - Output: Number of cached entries
 */
void cache_stats(unsigned int *hits, unsigned int *misses, unsigned int *entries);

#endif /* CACHE_H */
