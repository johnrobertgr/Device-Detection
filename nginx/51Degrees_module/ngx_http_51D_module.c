/* *********************************************************************
 * This Source Code Form is copyright of 51Degrees Mobile Experts Limited.
 * Copyright 2017 51Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY
 *
 * This Source Code Form is the subject of the following patents and patent
 * applications, owned by 51Degrees Mobile Experts Limited of 5 Charlotte
 * Close, Caversham, Reading, Berkshire, United Kingdom RG4 7BY:
 * European Patent No. 2871816;
 * European Patent Application No. 17184134.9;
 * United States Patent Nos. 9,332,086 and 9,350,823; and
 * United States Patent Application No. 15/686,066.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.
 *
 * If a copy of the MPL was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 *
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
 ********************************************************************** */
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_rbtree.h>
#include <ngx_string.h>
#ifdef FIFTYONEDEGREES_PATTERN
#include "src/pattern/51Degrees.h"
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
#include "src/trie/51Degrees.h"
#endif // FIFTYONEDEGREES_TRIE

// Define default settings.
#ifdef FIFTYONEDEGREES_PATTERN
#define FIFTYONEDEGREES_DEFAULTFILE (u_char*) "51Degrees.dat"
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
#define FIFTYONEDEGREES_DEFAULTFILE (u_char*) "51Degrees.trie"
#endif // FIFTYONEDEGREES_TRIE
#ifndef FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE
#define FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE "NA"
#endif // FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE
#ifndef FIFTYONEDEGREES_VALUE_SEPARATOR
#define FIFTYONEDEGREES_VALUE_SEPARATOR (u_char*) ","
#endif // FIFTYONEDEGREES_VALUE_SEPARATOR
#ifndef FIFTYONEDEGREES_MAX_STRING
#define FIFTYONEDEGREES_MAX_STRING 2048
#endif // FIFTYONEDEGREES_MAX_STRING
#ifndef FIFTYONEDEGREES_CACHE_KEY_LENGTH
#define FIFTYONEDEGREES_CACHE_KEY_LENGTH 500
#endif // FIFTYONEDEGREES_CACHE_KEY_LENGTH
#define FIFTYONEDEGREES_IMPORTANT_HEADERS_COUNT 5

// Module declaration.
ngx_module_t ngx_http_51D_module;

// Configuration function declarations.
static void *ngx_http_51D_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_51D_create_srv_conf(ngx_conf_t *cf);
static void *ngx_http_51D_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_51D_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_51D_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);

// Set function declarations.
static char *ngx_http_51D_set_loc(ngx_conf_t* cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_51D_set_srv(ngx_conf_t* cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_51D_set_main(ngx_conf_t* cf, ngx_command_t *cmd, void *conf);

// Request handler declaration.
static ngx_int_t ngx_http_51D_handler(ngx_http_request_t *r);

// Declaration of shared memory zones.
static ngx_shm_zone_t *ngx_http_51D_shm_dataSet;
static ngx_int_t ngx_http_51D_init_shm_dataSet(ngx_shm_zone_t *shm_zone, void *data);
#ifdef FIFTYONEDEGREES_PATTERN
static ngx_shm_zone_t *ngx_http_51D_shm_cache;
static ngx_int_t ngx_http_51D_init_shm_cache(ngx_shm_zone_t *shm_zone, void *data);
// Cache size integer accessible by init_shm_cache.
static int ngx_http_51D_cacheSize;
#endif // FIFTYONEDEGREES_PATTERN
// Atomic integer used to ensure a new data set memory zone on each reload.
ngx_atomic_t ngx_http_51D_shm_tag = 1;

ngx_atomic_t *ngx_http_51D_worker_count;

// Structure containing details of a specific header to be set as per the
// config file.
typedef struct ngx_http_51D_header_to_set_t ngx_http_51D_header_to_set;

struct ngx_http_51D_header_to_set_t {
    ngx_uint_t multi;          /* 0 for single User-Agent match, 1 for multiple
	                              HTTP header match. */
    ngx_uint_t propertyCount;  /* The number of properties in the property
	                              array. */
    ngx_str_t **property;      /* Array of properties to set. */
    ngx_str_t name;            /* The header name to set. */
    ngx_str_t lowerName;       /* The header name in lower case. */
    ngx_str_t variableName;    /* The name of the variable to use a
	                              User-Agent */
    size_t maxString;          /* The max string length the header value can
	                              take. */
	ngx_http_51D_header_to_set *next; /* The next header in the list. */
};

// Values of a prematched header, this is stored in the cache.
typedef struct {
	ngx_str_t name;      /* The header name */
	u_char* lowerName;   /* The header name in lower case. */
	ngx_str_t value;     /* The value of the header (a comma separated list of
	                        property values). */
} ngx_http_51D_matched_header_t;

// Match config structure set from the config file.
typedef struct {
#ifdef FIFTYONEDEGREES_PATTERN
	uint32_t key;                        /* Location key, a hash of all the
	                                        properties to be found and header
	                                        names. */
#endif // FIFTYONEDEGREES_PATTERN
	ngx_uint_t hasMulti;                 /* Indicated whether there is a
	                                        multiple HTTP header match in this
	                                        location. */
    ngx_uint_t headerCount;              /* The number of headers to set. */
    ngx_http_51D_header_to_set *header;  /* List of headers to set. */
} ngx_http_51D_match_conf_t;

// Module location config.
typedef struct {
	ngx_http_51D_match_conf_t matchConf; /* The match to carry out in this
	                                        location. */
} ngx_http_51D_loc_conf_t;

// Module main config.
typedef struct {
	char properties[FIFTYONEDEGREES_MAX_STRING]; /* Properties string to
	                                                initialise the data
	                                                set with. */
    ngx_str_t dataFile;                          /* 51Degrees data file. */
#ifdef FIFTYONEDEGREES_PATTERN
    ngx_uint_t cacheSize;                        /* Size of the cache. */
    fiftyoneDegreesWorkset *ws;                  /* 51Degrees workset, local
                                                    to each process. */
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	fiftyoneDegreesDeviceOffsets *offsets;       /* 51Degrees offsets, local
	                                                to each process. */
#endif // FIFTYONEDEGREES_TRIE
	fiftyoneDegreesDataSet *dataSet;             /* 51Degrees data set, shared
	                                                across all process'. */
	ngx_str_t valueSeparator;                    /* Match header value
                                                    separator. */

	ngx_http_51D_match_conf_t matchConf;         /* The match to carry out in
	                                                this block's locations. */
} ngx_http_51D_main_conf_t;

// Module server config.
typedef struct {
	ngx_http_51D_match_conf_t matchConf; /* The match to carry out in this
	                                        server's locations. */
} ngx_http_51D_srv_conf_t;

#ifdef FIFTYONEDEGREES_PATTERN
// 51Degrees LRU element declaration.
typedef struct ngx_http_51D_cache_lru_list_s ngx_http_51D_cache_lru_list_t;

// 51Degrees cache node.
typedef struct {
	ngx_rbtree_node_t node;                /* rbtree node (contains hash
	                                          key. */
	ngx_http_51D_cache_lru_list_t *lruItem;/* Pointer to element in the LRU. */
	ngx_str_t name;                        /* Name of the node, this is set to
	                                          the User-Agent. */
	ngx_uint_t headersCount;               /* Number of headers stored. */
	ngx_http_51D_matched_header_t** header;/* Array of headers stored. */
} ngx_http_51D_cache_node_t;

// 51Degrees LRU element.
struct ngx_http_51D_cache_lru_list_s {
	ngx_http_51D_cache_node_t *node;     /* Pointer to the node in the tree. */
	ngx_http_51D_cache_lru_list_t *prev; /* Pointer to previous element (for
	                                        first element, will point to
	                                        last element). */
	ngx_http_51D_cache_lru_list_t *next; /* Pointer to next element (for last
	                                        element, will be NULL). */
};

// 51Degrees cache.
typedef struct {
	ngx_http_51D_cache_lru_list_t *lru; /* Pointer to LRU linked list. */
	ngx_rbtree_t *tree;                 /* Pointer to nginx standard rbtree. */
} ngx_http_51D_cache_t;

/**
 * Get cache size function. Calculates the size in memory needed for a given
 * number of cache elements. Used to set the size of the shared memory.
 * @param cacheSize the number of elements in the cache.
 * @return size_t the size needed for the cache.
 */
size_t ngx_http_51D_get_cache_shm_size(int cacheSize)
{
	size_t size = 0;
	// Size of the shared memory zone.
	size += sizeof(ngx_shm_zone_t);
	// Size of the cache structure.
	size += sizeof(ngx_http_51D_cache_t);
	// Size of the lru elements.
	size += sizeof(ngx_http_51D_cache_lru_list_t) * cacheSize;
	// Size of the red black tree.
	size += sizeof(ngx_rbtree_t);
	// Size of the rbtree elements.
	size += sizeof(ngx_http_51D_cache_node_t) * cacheSize;
	size += sizeof(u_char) * FIFTYONEDEGREES_MAX_STRING * (1 + 3 * FIFTYONEDEGREES_IMPORTANT_HEADERS_COUNT) * cacheSize;

	return size;
}
#endif // FIFTYONEDEGREES_PATTERN

/**
 * Throws an error if data set initialisation fails. Used by the init module
 * function.
 * @param cycle the current nginx cycle.
 * @param status 51Degrees data set init status.
 * @param fileName the file being loaded.
 * @return NGX_ERROR.
 */
static ngx_int_t reportDatasetInitStatus(ngx_cycle_t *cycle, fiftyoneDegreesDataSetInitStatus status,
										const char* fileName) {
	switch (status) {
	case DATA_SET_INIT_STATUS_INSUFFICIENT_MEMORY:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Insufficient memory to load '%s'.", fileName);
		break;
	case DATA_SET_INIT_STATUS_CORRUPT_DATA:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Device data file '%s' is corrupted.", fileName);
		break;
	case DATA_SET_INIT_STATUS_INCORRECT_VERSION:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Device data file '%s' is not correct version.", fileName);
		break;
	case DATA_SET_INIT_STATUS_FILE_NOT_FOUND:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Device data file '%s' not found.", fileName);
		break;
	case DATA_SET_INIT_STATUS_NULL_POINTER:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Null pointer to the existing dataset or memory location.");
		break;
	case DATA_SET_INIT_STATUS_POINTER_OUT_OF_BOUNDS:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Allocated continuous memory containing 51Degrees data file "
			"appears to be smaller than expected. Most likely because the"
			" data file was not fully loaded into the allocated memory.");
		break;
	default:
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Device data file '%s' could not be loaded.", fileName);
		break;
	}
	return NGX_ERROR;
}

/**
 * Module post config. Adds the module to the HTTP access phase array, sets the
 * defaults if necessary, and sets the shared memory zones.
 * @param cf nginx config.
 * @return ngx_int_t nginx conf status.
 */
static ngx_int_t
ngx_http_51D_post_conf(ngx_conf_t *cf)
{
	ngx_http_handler_pt *h;
	ngx_http_core_main_conf_t *cmcf;
	ngx_http_51D_main_conf_t *fdmcf;
	ngx_atomic_int_t tagOffset;
	ngx_str_t dataSetName;
	size_t size;
#ifdef FIFTYONEDEGREES_PATTERN
	ngx_str_t cacheName;
	size_t cacheSize;
#endif // FIFTYONEDEGREES_PATTERN

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
	fdmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_51D_module);

	h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
	if (h == NULL) {
		return NGX_ERROR;
	}

	*h = ngx_http_51D_handler;

	// Set the default value separator if necessary.
	if ((int)fdmcf->valueSeparator.len <= 0) {
		fdmcf->valueSeparator.data = FIFTYONEDEGREES_VALUE_SEPARATOR;
		fdmcf->valueSeparator.len = ngx_strlen(fdmcf->valueSeparator.data);
	}

	// Set default data file if necessary.
	if ((int)fdmcf->dataFile.len <= 0) {
		ngx_log_error(NGX_LOG_INFO, cf->cycle->log, 0, "51D_filePath was not set. No provider was loaded.");
		return NGX_OK;
	}
#ifdef FIFTYONEDEGREES_PATTERN
	// Set the cache size to 0 if not set.
	if ((int)fdmcf->cacheSize < 0) {
		fdmcf->cacheSize = 0;
	}
	ngx_http_51D_cacheSize = (int)fdmcf->cacheSize;
#endif // FIFTYONEDEGREES_PATTERN

	// Initialise the shared memory zone for the data set.
	dataSetName.data = (u_char*) "51Degrees Shared Data Set";
	dataSetName.len = ngx_strlen(dataSetName.data);
	tagOffset = ngx_atomic_fetch_add(&ngx_http_51D_shm_tag, (ngx_atomic_int_t)1);
#ifdef FIFTYONEDEGREES_PATTERN
	size = fiftyoneDegreesGetProviderSizeWithPropertyString((const char*)fdmcf->dataFile.data, (const char*)fdmcf->properties, 0, 0);
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	size = fiftyoneDegreesGetProviderSizeWithPropertyString((const char*)fdmcf->dataFile.data, (const char*)fdmcf->properties);
#endif // FIFTYONEDEGREES_TRIE
	if ((int)size < 1) {
		// If there was a problem, throw an error.
		return reportDatasetInitStatus(cf->cycle, 0, (const char*)fdmcf->dataFile.data);
	}
	size *= 1.1;
	ngx_http_51D_shm_dataSet = ngx_shared_memory_add(cf, &dataSetName, size, &ngx_http_51D_module + tagOffset);
	ngx_http_51D_shm_dataSet->init = ngx_http_51D_init_shm_dataSet;

#ifdef FIFTYONEDEGREES_PATTERN
	// Initialise the shared memory zone for the cache.
	if (ngx_http_51D_cacheSize > 0) {
		cacheName.data = (u_char*) "51Degrees Shared Cache";
		cacheName.len = ngx_strlen(cacheName.data);
		cacheSize = ngx_http_51D_get_cache_shm_size(ngx_http_51D_cacheSize);
		ngx_http_51D_shm_cache = ngx_shared_memory_add(cf, &cacheName, cacheSize, &ngx_http_51D_module);
		ngx_http_51D_shm_cache->init = ngx_http_51D_init_shm_cache;
	}
#endif // FIFTYONEDEGREES_PATTERN
	return NGX_OK;
}

/**
 * Initialises the match config so no headers are set.
 * @param matchConf to initialise.
 */
static void
ngx_http_51D_init_match_conf(ngx_http_51D_match_conf_t *matchConf)
{
#ifdef FIFTYONEDEGREES_PATTERN
    matchConf->key = 0;
#endif // FIFTYONEDEGREES_PATTERN
    matchConf->hasMulti = 0;
	matchConf->headerCount = 0;
	matchConf->header = NULL;
}

/**
 * Create main config. Allocates memory to the configuration and initialises
 * cacheSize to -1 (unset).
 * @param cf nginx config.
 * @return Pointer to module main config.
 */
static void *
ngx_http_51D_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_51D_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_51D_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    ngx_log_debug0(NGX_LOG_DEBUG_ALL, cf->log, 0, "51Degrees creating main config.");
#ifdef FIFTYONEDEGREES_PATTERN
	conf->cacheSize = NGX_CONF_UNSET_UINT;
#endif // FIFTYONEDEGREES_PATTERN
	ngx_http_51D_init_match_conf(&conf->matchConf);
    return conf;
}

/**
 * Create location config. Allocates memory to the configuration.
 * @param cf nginx config.
 * @return Pointer to module location config.
 */
static void *
ngx_http_51D_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_51D_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_51D_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    ngx_http_51D_init_match_conf(&conf->matchConf);
	return conf;
}

/**
 * Create server config. Allocates memory to the configuration.
 * @param cf nginx config.
 * @return Pointer to the server config.
 */
static void *
ngx_http_51D_create_srv_conf(ngx_conf_t *cf)
{
	ngx_http_51D_srv_conf_t *conf;

	conf = ngx_palloc(cf->pool, sizeof(ngx_http_51D_srv_conf_t));
	if (conf == NULL) {
		return NULL;
	}
	ngx_http_51D_init_match_conf(&conf->matchConf);
	return conf;
}

/**
 * Merge location config. Either gets the value of count that is set, or sets
 * to the default of 0.
 */
static char *
ngx_http_51D_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_51D_loc_conf_t  *prev = parent;
	ngx_http_51D_loc_conf_t  *conf = child;

	ngx_conf_merge_uint_value(conf->matchConf.headerCount, prev->matchConf.headerCount, 0);

	return NGX_CONF_OK;
}

/**
 * Merge server config. Either gets the value of count that is set, or sets
 * to the default of 0.
 */
static char *
ngx_http_51D_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_51D_srv_conf_t  *prev = parent;
	ngx_http_51D_srv_conf_t  *conf = child;

	ngx_conf_merge_uint_value(conf->matchConf.headerCount, prev->matchConf.headerCount, 0);

	return NGX_CONF_OK;
}

/**
 * Shared memory alloc function. Replaces fiftyoneDegreesMalloc to store
 * the data set in the shared memory zone.
 * @param __size the size of memory to allocate.
 * @return void* a pointer to the allocated memory.
 */
void *ngx_http_51D_shm_alloc(size_t __size)
{
	void *ptr = NULL;
	ngx_slab_pool_t *shpool;
	shpool = (ngx_slab_pool_t *) ngx_http_51D_shm_dataSet->shm.addr;
	ptr = ngx_slab_alloc_locked(shpool, __size);
	ngx_log_debug2(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0, "51Degrees shm alloc %d %p", __size, ptr);
	if (ptr == NULL) {
		ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "51Degrees shm failed to allocate memory, not enough shared memory.");
	}
	return ptr;
}

/**
 * Shared memory free function. Replaces fiftyoneDegreesFree to free pointers
 * to the shared memory zone.
 * @param __ptr pointer to the memory to be freed.
 */
void ngx_http_51D_shm_free(void *__ptr)
{
	ngx_slab_pool_t *shpool;
	shpool = (ngx_slab_pool_t *) ngx_http_51D_shm_dataSet->shm.addr;
	if ((u_char *) __ptr < shpool->start || (u_char *) __ptr > shpool->end) {
		// The memory is not in the shared memory pool, so free with standard
		// free function.
	ngx_log_debug1(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0, "51Degrees shm free (non shared) %p", __ptr);
		free(__ptr);
	}
	else {
		ngx_log_debug1(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0, "51Degrees shm free %p", __ptr);
		ngx_slab_free_locked(shpool, __ptr);
	}
}

/**
 * Init data set memory zone. Allocates space for the provider in the shared
 * memory zone.
 * @param shm_zone the shared memory zone.
 * @param data if the zone has been carried over from a reload, this is the
 *        old data.
 * @return ngx_int_t nginx conf status.
 */
static ngx_int_t ngx_http_51D_init_shm_dataSet(ngx_shm_zone_t *shm_zone, void *data)
{
	ngx_slab_pool_t *shpool;
	fiftyoneDegreesDataSet *dataSet;
	shpool = (ngx_slab_pool_t *) ngx_http_51D_shm_dataSet->shm.addr;

	// Allocate space for the data set.
	dataSet = (fiftyoneDegreesDataSet*)ngx_slab_alloc(shpool, sizeof(fiftyoneDegreesDataSet));

	// Set the data set as the shared data for this zone.
	shm_zone->data = dataSet;
	if (dataSet == NULL) {
		ngx_log_error(NGX_LOG_ERR, shm_zone->shm.log, 0, "51Degrees shared memory could not be allocated.");
		return NGX_ERROR;
	}
	ngx_log_debug1(NGX_LOG_DEBUG_ALL, shm_zone->shm.log, 0, "51Degrees initialised shared memory with size %d.", shm_zone->shm.size);

	return NGX_OK;
}

#ifdef FIFTYONEDEGREES_PATTERN
/**
 * Insert node function. Is used by ngx_rbtree_insert to find where the node
 * belongs in the tree.
 * @param temp the current node in the tree.
 * @param node the node to insert.
 * @param sentinel the end of a branch.
 */
void
ngx_http_51D_insert_node(ngx_rbtree_node_t *temp,
    ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
    ngx_http_51D_cache_node_t      *n, *t;
    ngx_rbtree_node_t  **p;

    for ( ;; ) {

        n = (ngx_http_51D_cache_node_t *) node;
        t = (ngx_http_51D_cache_node_t *) temp;

		// Compare the nodes hashes.
        if (node->key != temp->key) {

            p = (node->key < temp->key) ? &temp->left : &temp->right;

        }
        // If hashes are equal, compare the nodes name lengths.
        else if (n->name.len != t->name.len) {

            p = (n->name.len < t->name.len) ? &temp->left : &temp->right;

        }
        // If name lengths are equal, compare the names.
        else {
            p = (ngx_memcmp(n->name.data, t->name.data, n->name.len) < 0)
                 ? &temp->left : &temp->right;
        }

		// If the node is at the end of a branch, stop comparing.
        if (*p == sentinel) {
            break;
        }

		// Move to the next node.
        temp = *p;
    }

	// Set node.
    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ngx_rbt_red(node);
}

/**
 * Init cache memory zone. If a cache does not already exist from a reload,
 * Initialise one with an rbtree and an LRU linked list.
 * @param shm_zone the shared memory zone to use.
 * @param data the existing cache if reloading (and the cache size is the same).
 * @return ngx_int_t nginx status.
 */
static ngx_int_t ngx_http_51D_init_shm_cache(ngx_shm_zone_t *shm_zone, void *data)
{
	ngx_slab_pool_t *shpool;
	ngx_rbtree_t *tree;
	ngx_rbtree_node_t *sentinel;
	ngx_http_51D_cache_t *cache;

	// A cache already exists, so keep it.
	if (data) {
		shm_zone->data = data;
		return NGX_OK;
	}

	shpool = (ngx_slab_pool_t*) shm_zone->shm.addr;

	// Allocate the rbtree.
	tree = ngx_slab_alloc(shpool, sizeof(ngx_rbtree_t));
	if (tree == NULL) {
		return NGX_ERROR;
	}

	// Allocate the trees sentinel node.
	sentinel = ngx_slab_alloc(shpool, sizeof(ngx_rbtree_node_t));
	if (sentinel == NULL) {
		ngx_slab_free(shpool, tree);
		return NGX_ERROR;
	}

	// Initialise the rbtree.
	ngx_rbtree_sentinel_init(sentinel);
	tree->root = sentinel;
	tree->sentinel = sentinel;
	tree->insert = ngx_http_51D_insert_node;

	// Allocate the cache structure initialise it.
	cache = (ngx_http_51D_cache_t*)ngx_slab_alloc(shpool, sizeof(ngx_http_51D_cache_t));
	if (cache == NULL) {
		ngx_slab_free(shpool, sentinel);
		ngx_slab_free(shpool, tree);
		return NGX_ERROR;
	}
	cache->tree = tree;
	cache->lru = (ngx_http_51D_cache_lru_list_t*)ngx_slab_alloc(shpool, sizeof(ngx_http_51D_cache_lru_list_t));
	if (cache->lru == NULL) {
		ngx_slab_free(shpool, sentinel);
		ngx_slab_free(shpool, tree);
		ngx_slab_free(shpool, cache);
		return NGX_ERROR;
	}
	cache->lru->node = NULL;

	// Allocate all the LRU elements.
	int i = 1;
	ngx_http_51D_cache_lru_list_t *current;
	current = cache->lru;
	while (i < ngx_http_51D_cacheSize) {
		current->next = (ngx_http_51D_cache_lru_list_t*)ngx_slab_alloc(shpool, sizeof(ngx_http_51D_cache_lru_list_t));
		if (current->next == NULL) {
			current = current->prev;
			while ( current != cache->lru ) {
				ngx_slab_free(shpool, current->next);
				current = current->prev;
			}
			ngx_slab_free(shpool, cache->lru);
			ngx_slab_free(shpool, sentinel);
			ngx_slab_free(shpool, tree);
			ngx_slab_free(shpool, cache);
			return NGX_ERROR;
		}
		current->next->prev = current;
		current->next->node = NULL;
		current = current->next;
		i++;
	}
	current->next = NULL;
	cache->lru->prev = current;

	// Set the cache structure as the shared data.
	shm_zone->data = cache;

	return NGX_OK;
}

/**
 * Lookup node function. Searches for a node in the rbtree using first
 * its hash, then the name.
 * @param rbtree the tree to search in.
 * @param name the name of the node (set as the User-Agent).
 * @param hash the hash to search for in the tree.
 * @return ngx_http_51D_cache_node_t* the node that was found in the tree,
 *                                    or NULL if it was not found.
 */
static ngx_http_51D_cache_node_t *
ngx_http_51D_lookup_node(ngx_rbtree_t *rbtree, ngx_str_t *name, uint32_t hash)
{
    ngx_int_t           rc;
    ngx_http_51D_cache_node_t     *n;
    ngx_rbtree_node_t  *node, *sentinel;

    node = rbtree->root;
    sentinel = rbtree->sentinel;

    while (node != sentinel) {

        n = (ngx_http_51D_cache_node_t*) node;

        if (hash != node->key) {
            node = (hash < node->key) ? node->left : node->right;
            continue;
        }

        if (name->len != n->name.len) {
            node = (name->len < n->name.len) ? node->left : node->right;
            continue;
        }

        rc = ngx_memcmp(name->data, n->name.data, name->len);

        if (rc < 0) {
            node = node->left;
            continue;
        }

        if (rc > 0) {
            node = node->right;
            continue;
        }

        return n;
    }

    return NULL;
}

/**
 * LRU lookup function. Searches the rbtree, and if found updates the LRU and
 * returns the node. The cache MUST be locked while the node data is in use,
 * otherwise the data may be removed or overwritten another process.
 * @param name the name of the node (set to the User-Agent).
 * @param hash the hash to search for.
 * @return ngx_http_51D_cache_node_t* the node that was found, or NULL if it was
 *                                    not found.
 */
ngx_http_51D_cache_node_t *ngx_http_51D_lookup_node_lru(ngx_str_t *name, int32_t hash)
{
	ngx_http_51D_cache_node_t *node;
	ngx_http_51D_cache_lru_list_t *lruItem;
	ngx_http_51D_cache_t *cache = (ngx_http_51D_cache_t*)ngx_http_51D_shm_cache->data;
	// Look for the node in the rbtree.
	node = ngx_http_51D_lookup_node(cache->tree, name, hash);
	if (node != NULL && node != cache->lru->node) {
		// Node exists in the cache, and is not at the top of the lru,
		// so move it to the top.
		lruItem = node->lruItem;
		if (lruItem->next != NULL) {
			lruItem->next->prev = lruItem->prev;
			lruItem->prev = cache->lru->prev;
			cache->lru->prev = lruItem;
		}
		lruItem->prev->next = lruItem->next;
		lruItem->next = cache->lru;
		cache->lru = lruItem;
	}

	return node;
}

/**
 * Cache purge function. Removes the last n entries in the LRU.
 * Note the shared memory zone must be locked before calling this
 * function.
 * @param cache a 51Degrees cache structure.
 * @param count the number of entries to purge.
 */
void ngx_http_51D_cache_clean(ngx_http_51D_cache_t *cache, int count)
{
	int i = 0, j;
	ngx_http_51D_cache_node_t *node;
	ngx_slab_pool_t *shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_cache->shm.addr;
	ngx_http_51D_cache_lru_list_t *lruItem = cache->lru->prev;

	// If the cache is not full, skip past the empty elements.
	while (lruItem->node == NULL) {
		lruItem = lruItem->prev;
	}

	while (i < count) {
		// Remove the node from the rbtree.
		node = lruItem->node;

		// Make sure the node is not NULL.
		if (node == NULL) {
			lruItem = lruItem->prev;
			i++;
			continue;
		}
		ngx_rbtree_delete(cache->tree, (ngx_rbtree_node_t*)node);

		// Free everything in the node.
		for (j = 0; j < (int)node->headersCount; j++) {
			ngx_slab_free_locked(shpool, node->header[j]->lowerName);
			ngx_slab_free_locked(shpool, node->header[j]->name.data);
			ngx_slab_free_locked(shpool, node->header[j]->value.data);
			ngx_slab_free_locked(shpool, node->header[j]);
		}
		ngx_slab_free_locked(shpool, node->name.data);
		ngx_slab_free_locked(shpool, node->header);
		ngx_slab_free_locked(shpool, node);

		// Empty the element in the LRU.
		lruItem->node = NULL;

		// Move to the next element.
		lruItem = lruItem->prev;
		i++;
	}
}

/**
 * Cache alloc function. Allocates memory in the caches shared memory zone.
 * If there is no space, purge the cache.
 * @param __size the size of memory to allocate.
 * @return void* pointer to the memory allocated.
 */
void *ngx_http_51D_cache_alloc(size_t __size)
{
	ngx_slab_pool_t *shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_cache->shm.addr;
	ngx_http_51D_cache_t *cache = (ngx_http_51D_cache_t*)ngx_http_51D_shm_cache->data;
	void *ptr = ngx_slab_alloc(shpool, __size);
	if (ptr == NULL) {
		ngx_shmtx_lock(&shpool->mutex);
		ngx_http_51D_cache_clean(cache, 3);
		ngx_slab_alloc_locked(shpool, __size);
		ngx_shmtx_unlock(&shpool->mutex);
	}
	return ptr;
}

/**
 * Add header to node function. Adds a header to the cache node structure.
 * @param node the 51Degrees cache node to add the header to.
 * @param h the nginx header structure to copy.
 */
void ngx_http_51D_add_header_to_node(ngx_http_51D_cache_node_t *node, ngx_table_elt_t *h)
{
	ngx_http_51D_matched_header_t *header;
	header = node->header[(int)node->headersCount];
	// Copy the header name.
	header->name.data = (u_char*)ngx_http_51D_cache_alloc(sizeof(u_char) * (h->key.len + 1));
	ngx_cpystrn(header->name.data, h->key.data, h->key.len + 1);
	header->name.len = h->key.len;
	// Copy the header lower case name.
	header->lowerName = (u_char*)ngx_http_51D_cache_alloc(sizeof(u_char) * (ngx_strlen(h->lowcase_key) + 1));
	ngx_cpystrn(header->lowerName, h->lowcase_key, ngx_strlen(h->lowcase_key) + 1);
	// Copy the header value.
	header->value.data = (u_char*)ngx_http_51D_cache_alloc(sizeof(u_char) * (h->value.len) + 1);
	ngx_cpystrn(header->value.data, h->value.data, h->value.len +1);
	header->value.len = h->value.len;
	// Increment the header count.
	node->headersCount++;
}

/**
 * Create cache node function. Allocates a new node in the cache with space for
 * the specified number of headers and sets the hash.
 * @param name the name of the header (should be set to the User-Agent).
 * @param hash the hash to use as the nodes key.
 * @param headerCount the number of headers the node will store.
 * @return ngx_http_51D_cache_node_t* the newly created cache node. Note
 *                                    this is not yet in the cache, just
 *                                    stored in the same shared memory.
 */
ngx_http_51D_cache_node_t *ngx_http_51D_create_node(ngx_str_t *name, uint32_t hash, int headerCount)
{
	int i;
	ngx_http_51D_cache_node_t *node;

	// Create the node.
	node = (ngx_http_51D_cache_node_t*)ngx_http_51D_cache_alloc(sizeof(ngx_http_51D_cache_node_t));
	node->node.key = hash;
	node->headersCount = 0;
	node->header = (ngx_http_51D_matched_header_t**)ngx_http_51D_cache_alloc(sizeof(ngx_http_51D_matched_header_t*)*headerCount);
	// Add the headers.
	for (i = 0; i < headerCount; i++) {
		node->header[i] = (ngx_http_51D_matched_header_t*)ngx_http_51D_cache_alloc(sizeof(ngx_http_51D_matched_header_t));
	}
	// Set the name.
	node->name.data = (u_char*)ngx_http_51D_cache_alloc(name->len + 1);
	ngx_cpystrn(node->name.data, name->data, name->len + 1);
	node->name.len = name->len;

	return node;
}

/**
 * Cache insert function. Inserts the supplied node in caches rbtree and LRU.
 * Note the cache must not be locked before calling this function.
 * @param node the node to insert.
 */
void ngx_http_51D_cache_insert(ngx_http_51D_cache_node_t *node)
{
	int i;
	ngx_slab_pool_t *shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_cache->shm.addr;
	ngx_http_51D_cache_t *cache = (ngx_http_51D_cache_t*)ngx_http_51D_shm_cache->data;

	// Lock the cache
	ngx_shmtx_lock(&shpool->mutex);

	// Check if an identical node has been inserted by another process while this
	// node was created.
	if (ngx_http_51D_lookup_node(cache->tree, &node->name, node->node.key) != NULL) {
		// Free everything in the node.
		for (i = 0; i < (int)node->headersCount; i++) {
			ngx_slab_free_locked(shpool, node->header[i]->lowerName);
			ngx_slab_free_locked(shpool, node->header[i]->name.data);
			ngx_slab_free_locked(shpool, node->header[i]->value.data);
			ngx_slab_free_locked(shpool, node->header[i]);
		}
		ngx_slab_free_locked(shpool, node->name.data);
		ngx_slab_free_locked(shpool, node->header);
		ngx_slab_free_locked(shpool, node);
	}
	else {
		if (cache->lru->prev->node != NULL) {
			// The cache is full, so purge the last 3 from the lru.
			ngx_http_51D_cache_clean(cache, 3);
		}

		// Add node to the rbtree.
		ngx_rbtree_insert(cache->tree, (ngx_rbtree_node_t*)node);

		// Add node to the LRU.
		cache->lru->prev->next = cache->lru;
		cache->lru->prev->node = node;
		cache->lru->prev->prev->next = NULL;
		cache->lru = cache->lru->prev;
		node->lruItem = cache->lru;
	}
	// Unlock the cache.
	ngx_shmtx_unlock(&shpool->mutex);
}

/**
 * Appends a value to the cache key if there is enough space.
 * @param cacheKey the key to append.
 * @param value the string to add to the key.
 * @param maxLength the number of bytes allocated to the key.
 */
void
ngx_http_51D_cache_append_key(ngx_str_t *cacheKey, ngx_str_t *value, size_t maxLength) {
	size_t len = cacheKey->len + value->len > maxLength ?
		maxLength - cacheKey->len :
		value->len;
    strncpy((char*)cacheKey->data + cacheKey->len, (const char*)value->data, len);
    cacheKey->len += len;
}

/**
 * Set max string function. Gets the maximum possible length of the returned
 * list of values for specified header structure and allocates space for it.
 * @param fdmcf 51Degrees module main config.
 * @param header pointer to the header to set the max length for.
 */
static void
ngx_http_51D_set_max_string(ngx_http_51D_main_conf_t *fdmcf, ngx_http_51D_header_to_set *header)
{
	const fiftyoneDegreesDataSet *dataSet = fdmcf->dataSet;
	ngx_uint_t i, tmpLength, length = 0;
	for (i = 0; i < header->propertyCount; i++) {
		if (i != 0) {
			length += (int)fdmcf->valueSeparator.len;
		}
		tmpLength = fiftyoneDegreesGetMaxPropertyValueLength(dataSet, (char*)header->property[i]->data);
		if ((int)tmpLength > 0) {
			length += (int)tmpLength;
		}
		else {
			length += ngx_strlen(FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE);
		}
	}

	header->maxString = (size_t) length * sizeof(char);
}
#endif // FIFTYONEDEGREES_PATTERN

/**
 * Init module function. Initialises the provider with the given initialisation
 * parameters. Throws an error if the data set could not be initialised.
 * @param cycle the current nginx cycle.
 * @return ngx_int_t nginx conf status.
 */
static ngx_int_t
ngx_http_51D_init_module(ngx_cycle_t *cycle)
{
	fiftyoneDegreesDataSetInitStatus status;
	ngx_http_51D_main_conf_t *fdmcf;
	ngx_slab_pool_t *shpool;

	// Get module main config.
	fdmcf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_51D_module);

	if (ngx_http_51D_shm_dataSet == NULL) {
		return NGX_OK;
	}
	fdmcf->dataSet = (fiftyoneDegreesDataSet*)ngx_http_51D_shm_dataSet->data;

	shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_dataSet->shm.addr;

	// Set the memory allocation functions to use the shared memory zone.
	fiftyoneDegreesMalloc = ngx_http_51D_shm_alloc;
	fiftyoneDegreesFree = ngx_http_51D_shm_free;

	// Initialise the data set.
	ngx_shmtx_lock(&shpool->mutex);
	ngx_http_51D_worker_count = (ngx_atomic_t*)ngx_http_51D_shm_alloc(sizeof(ngx_atomic_t));
	ngx_atomic_cmp_set(ngx_http_51D_worker_count, 0, 0);

	status = fiftyoneDegreesInitWithPropertyString((const char*)fdmcf->dataFile.data, fdmcf->dataSet, (const char*)fdmcf->properties);
	ngx_shmtx_unlock(&shpool->mutex);
	if (status != DATA_SET_INIT_STATUS_SUCCESS) {
		return reportDatasetInitStatus(cycle, status, (const char*)fdmcf->dataFile.data);
	}
	ngx_log_debug2(NGX_LOG_DEBUG_ALL, cycle->log, 0, "51Degrees initialised from file '%s' with properties '%s'.", (char*)fdmcf->dataFile.data, fdmcf->properties);

	// Reset the malloc and free functions as nothing else should be allocated
	// in the shared memory zone.
	fiftyoneDegreesMalloc = malloc;
	fiftyoneDegreesFree = free;

	return NGX_OK;
}

/**
 * Definitions of functions which can be called from the config file.
 * --51D_match_single takes two string arguments, the name of the header
 * to be set and a comma separated list of properties to be returned.
 * Is called within location block. Enables User-Agent matching.
 * --51D_match_all takes two string arguments, the name of the header to
 * be set and a comma separated list of properties to be returned. Is
 * called within location block. Enables multiple http header matching.
 * --51D_filePath takes one string argument, the path to a
 * 51Degrees data file. Is called within server block.
 * --51D_cache takes one integer argument, the size of the
 * 51Degrees cache.
 */
static ngx_command_t  ngx_http_51D_commands[] = {

	{ ngx_string("51D_match_single"),
	NGX_HTTP_LOC_CONF|NGX_CONF_TAKE23,
	ngx_http_51D_set_loc,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_match_single"),
	NGX_HTTP_SRV_CONF|NGX_CONF_TAKE23,
	ngx_http_51D_set_srv,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_match_single"),
	NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE23,
	ngx_http_51D_set_main,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_match_all"),
	NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
	ngx_http_51D_set_loc,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_match_all"),
	NGX_HTTP_SRV_CONF|NGX_CONF_TAKE2,
	ngx_http_51D_set_srv,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_match_all"),
	NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE2,
	ngx_http_51D_set_main,
	NGX_HTTP_LOC_CONF_OFFSET,
    0,
	NULL },

	{ ngx_string("51D_filePath"),
	NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
	ngx_conf_set_str_slot,
	NGX_HTTP_MAIN_CONF_OFFSET,
	offsetof(ngx_http_51D_main_conf_t, dataFile),
	NULL },

	{ ngx_string("51D_valueSeparator"),
	NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
	ngx_conf_set_str_slot,
	NGX_HTTP_MAIN_CONF_OFFSET,
	offsetof(ngx_http_51D_main_conf_t, valueSeparator),
	NULL },

#ifdef FIFTYONEDEGREES_PATTERN
	{ ngx_string("51D_cache"),
	NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
	ngx_conf_set_num_slot,
	NGX_HTTP_MAIN_CONF_OFFSET,
	offsetof(ngx_http_51D_main_conf_t, cacheSize),
	NULL },
#endif // FIFTYONEDEGREES_PATTERN

	ngx_null_command
};

/**
 * Module context. Sets the configuration functions.
 */
static ngx_http_module_t ngx_http_51D_module_ctx = {
	NULL,                          /* preconfiguration */
	ngx_http_51D_post_conf,        /* postconfiguration */

	ngx_http_51D_create_main_conf, /* create main configuration */
	NULL,                          /* init main configuration */

	ngx_http_51D_create_srv_conf,  /* create server configuration */
	ngx_http_51D_merge_srv_conf,   /* merge server configuration */

	ngx_http_51D_create_loc_conf,  /* create location configuration */
	ngx_http_51D_merge_loc_conf,   /* merge location configuration */
};

/**
 * Init process function. Creates a work set from the shared data set.
 * This work set is local to the process.
 * @param cycle the current nginx cycle.
 * @return ngx_int_t nginx status.
 */
static ngx_int_t
ngx_http_51D_init_process(ngx_cycle_t *cycle)
{
	ngx_http_51D_main_conf_t *fdmcf;

	if (ngx_http_51D_shm_dataSet == NULL) {
		return NGX_OK;
	}

	fdmcf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_51D_module);

#ifdef FIFTYONEDEGREES_PATTERN
	fdmcf->ws = fiftyoneDegreesWorksetCreate(fdmcf->dataSet, NULL);
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	fdmcf->offsets = fiftyoneDegreesCreateDeviceOffsets(fdmcf->dataSet);
#endif // FIFTYONEDEGREES_TRIE

	// Increment the workers which are using the dataset.
	ngx_atomic_fetch_add(ngx_http_51D_worker_count, 1);

	return NGX_OK;
}

/**
 * Exit process function. Frees the work set that was created on process
 * init.
 * @param cycle the current nginx cycle.
 */
void
ngx_http_51D_exit_process(ngx_cycle_t *cycle)
{
	ngx_slab_pool_t *shpool;
	fiftyoneDegreesDataSet *dataSet;
	ngx_http_51D_main_conf_t *fdmcf;

	if (ngx_http_51D_shm_dataSet == NULL) {
		return;
	}

	dataSet = (fiftyoneDegreesDataSet*)ngx_http_51D_shm_dataSet->data;
	fdmcf = ngx_http_cycle_get_module_main_conf(cycle, ngx_http_51D_module);

#ifdef FIFTYONEDEGREES_PATTERN
	fiftyoneDegreesWorksetFree(fdmcf->ws);
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	fiftyoneDegreesFreeDeviceOffsets(fdmcf->offsets);
#endif // FIFTYONEDEGREES_TRIE

	shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_dataSet->shm.addr;

	// Decrement the worker count, and free the dataset if this is the last
	// worker to be closed.
	if (ngx_atomic_fetch_add(ngx_http_51D_worker_count, -1) == 1) {
		ngx_shmtx_lock(&shpool->mutex);
		if (*ngx_http_51D_worker_count == 0) {
			fiftyoneDegreesFree = ngx_http_51D_shm_free;
			fiftyoneDegreesDataSetFree(dataSet);
			fiftyoneDegreesFree = free;
			ngx_http_51D_shm_free((void*)ngx_http_51D_worker_count);
			ngx_log_debug0(NGX_LOG_DEBUG_ALL, ngx_cycle->log, 0, "51Degrees shared data set has been freed.");
		}
		ngx_shmtx_unlock(&shpool->mutex);
	}
}

/**
 * Module definition. Set the module context, commands, type and init function.
 */
ngx_module_t ngx_http_51D_module = {
	NGX_MODULE_V1,
	&ngx_http_51D_module_ctx,      /* module context */
	ngx_http_51D_commands,         /* module directives */
	NGX_HTTP_MODULE,               /* module type */
	NULL,                          /* init master */
	ngx_http_51D_init_module,      /* init module */
	ngx_http_51D_init_process,     /* init process */
	NULL,                          /* init thread */
	NULL,                          /* exit thread */
	ngx_http_51D_exit_process,     /* exit process */
	NULL,                          /* exit master */
	NGX_MODULE_V1_PADDING
};

/**
 * Search headers function. Searched request headers for the name supplied.
 * Used when matching multiple http headers to find important headers.
 * See:
 * https://www.nginx.com/resources/wiki/start/topics/examples/headers_management
 */
static ngx_table_elt_t *
search_headers_in(ngx_http_request_t *r, u_char *name, size_t len) {
    ngx_list_part_t            *part;;
    ngx_table_elt_t            *h;
    ngx_uint_t                  i;

    part = &r->headers_in.headers.part;
    h = part->elts;

    for (i = 0; /* void */ ; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (len != h[i].key.len || ngx_strcasecmp(name, h[i].key.data) != 0) {
            continue;
        }

        return &h[i];
    }

    return NULL;
}

/**
 * Add value function. Appends a string to a list separated by the
 * delimiter specified with 51D_valueSeparator, or a comma by default.
 * @param delimiter to split values with.
 * @param val the string to add to dst.
 * @param dst the string to append the val to.
 * @param length the size remaining to append val to dst.
 */
static void add_value(char *delimiter, char *val, char *dst, size_t length)
{
	// If the buffer already contains characters, append a pipe.
	if (dst[0] != '\0') {
		strncat(dst, delimiter, length);
		length -= strlen(delimiter);
	}

    // Append the value.
    strncat(dst, val, length);
}

/**
 * Get match function. Gets a match from the work set for either a single
 * User-Agent or all request headers.
 * @param fdmcf module main config.
 * @param r the current HTTP request.
 * @param multi 0 for a single User-Agent match, 1 for a multiple HTTP
 *              header match.
 */
void ngx_http_51D_get_match(ngx_http_51D_main_conf_t *fdmcf, ngx_http_request_t *r, int multi, ngx_str_t *userAgent)
{
	int headerIndex;
	ngx_table_elt_t *searchResult;
#ifdef FIFTYONEDEGREES_PATTERN
	fiftyoneDegreesWorkset *ws = fdmcf->ws;
	// If single requested, match for single User-Agent.
	if (multi == 0) {
			fiftyoneDegreesMatch(ws, (const char*)userAgent->data);
	}
	// If multi requested, match for multiple http headers.
	else if (multi == 1) {
		ws->importantHeadersCount = 0;
		for (headerIndex = 0; headerIndex < ws->dataSet->httpHeadersCount; headerIndex++) {
			searchResult = search_headers_in(r, (u_char*)ws->dataSet->httpHeaders[headerIndex].headerName, ngx_strlen(ws->dataSet->httpHeaders[headerIndex].headerName));
			if (searchResult) {
				ws->importantHeaders[ws->importantHeadersCount].header = ws->dataSet->httpHeaders + headerIndex;
				ws->importantHeaders[ws->importantHeadersCount].headerValue = (const char*) searchResult->value.data;
				ws->importantHeaders[ws->importantHeadersCount].headerValueLength = searchResult->value.len;
				ws->importantHeadersCount++;
			}
			fiftyoneDegreesMatchForHttpHeaders(ws);
		}
	}
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	fiftyoneDegreesDataSet *dataSet = fdmcf->dataSet;
	fiftyoneDegreesDeviceOffsets *offsets = fdmcf->offsets;

	// If single requested, match for single User-Agent.
	if (multi == 0) {
		offsets->size = 1;
            fiftyoneDegreesSetDeviceOffset(dataSet, (const char*)userAgent->data, 0, offsets->firstOffset);
	}
	// If multi requested, match for multiple http headers.
	else if (multi == 1) {
		headerIndex = 0;
		offsets->size = 0;
		const char *httpHeaderName = fiftyoneDegreesGetHttpHeaderNamePointer(dataSet, headerIndex);
		while (httpHeaderName != NULL) {
			searchResult = search_headers_in(r, (u_char*)httpHeaderName, ngx_strlen(httpHeaderName));
			if (searchResult!= NULL) {
				fiftyoneDegreesSetDeviceOffset(
					dataSet,
					(const char*)searchResult->value.data,
					headerIndex,
					&offsets->firstOffset[offsets->size]);
				offsets->size++;
			}
			headerIndex++;
			httpHeaderName = fiftyoneDegreesGetHttpHeaderNamePointer(dataSet, headerIndex);
		}
	}
#endif // FIFTYONEDEGREES_TRIE
}

/**
 * Get value function. Gets the requested value for the current match and
 * appends the value to the list of values separated by the delimiter specified
 * with 51D_valueSeparator.
 * @param fdmcf 51Degrees module main config.
 * @param r the current HTTP request.
 * @param values_string the string to append the returned value to.
 * @param requiredPropertyName the name of the property to get the value for.
 * @param length the size allocated to the values_string variable.
 */
void ngx_http_51D_get_value(ngx_http_51D_main_conf_t *fdmcf, ngx_http_request_t *r, char *values_string, const char *requiredPropertyName, size_t length)
{
	size_t remainingLength = length - strlen(values_string);
#ifdef FIFTYONEDEGREES_PATTERN
	fiftyoneDegreesWorkset *ws = fdmcf->ws;
	char *valueDelimiter = (char*)fdmcf->valueSeparator.data;
	char *methodName, *propertyName;
	char *buffer = (char*)ngx_palloc(r->pool, length);
	int i, j, found = 0;
	if (ngx_strcmp("Method", requiredPropertyName) == 0) {
		switch(ws->method) {
			case EXACT: methodName = "Exact"; break;
			case NUMERIC: methodName = "Numeric"; break;
			case NEAREST: methodName = "Nearest"; break;
			case CLOSEST: methodName = "Closest"; break;
			default:
			case NONE: methodName = "None"; break;
		}
		add_value(valueDelimiter, methodName, values_string, remainingLength);
		found = 1;
	}
	else if (ngx_strcmp("Difference", requiredPropertyName) == 0) {
		if (snprintf(buffer, length, "%d", ws->difference) < (int)strlen(buffer)) {
			ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
				"Difference value was not printed in full.");
		}
		add_value(valueDelimiter, buffer, values_string, remainingLength);
		found = 1;
	}
	else if (ngx_strcmp("Rank", requiredPropertyName) == 0) {
		if (snprintf(buffer, length, "%d", fiftyoneDegreesGetSignatureRank(ws)) < (int)strlen(buffer)) {
			ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
				"Rank value was not printed in full.");
		}
		add_value(valueDelimiter, buffer, values_string, remainingLength);
		found = 1;
	}
	else if (ngx_strcmp("DeviceId", requiredPropertyName) == 0) {
		fiftyoneDegreesGetDeviceId(ws, buffer, 24);
		add_value(valueDelimiter, buffer, values_string, remainingLength);
		found = 1;

	}
	else {
		for (i = 0; i < ws->dataSet->requiredPropertyCount; i++) {
			propertyName = (char*)fiftyoneDegreesGetPropertyName(ws->dataSet, ws->dataSet->requiredProperties[i]);
			if (ngx_strcmp(propertyName, requiredPropertyName) == 0) {
				fiftyoneDegreesSetValues(ws, i);
				buffer[0] = '\0';
				for (j = 0; j < ws->valuesCount; j++)
				{
					add_value("|", (char*)fiftyoneDegreesGetValueName(ws->dataSet, *(ws->values + j)), buffer, length - strlen(buffer));
				}
				add_value(valueDelimiter, buffer, values_string, remainingLength);
				found = 1;
				break;
			}
		}
		if (!found) {
			add_value(valueDelimiter, FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE, values_string, remainingLength);
		}
	}
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	fiftyoneDegreesDataSet *dataSet = fdmcf->dataSet;
	fiftyoneDegreesDeviceOffsets *offsets = fdmcf->offsets;
	char *valueDelimiter = (char*)fdmcf->valueSeparator.data;
	int requiredPropertyIndex = fiftyoneDegreesGetRequiredPropertyIndex(dataSet, requiredPropertyName);
	if (requiredPropertyIndex >= 0 &&
			requiredPropertyIndex <
			fiftyoneDegreesGetRequiredPropertiesCount(dataSet)) {
		char *value = (char*)fiftyoneDegreesGetValuePtrFromOffsets(
			dataSet,
			offsets,
			requiredPropertyIndex);
		if (value != NULL) {
			add_value(valueDelimiter, value, values_string, remainingLength);
		}
	}
	else {
		add_value(valueDelimiter, FIFTYONEDEGREES_PROPERTY_NOT_AVAILABLE, values_string, remainingLength);
	}
#endif // FIFTYONEDEGREES_TRIE
}


static ngx_str_t* emptyString(ngx_http_request_t *r) {
	ngx_str_t *str = (ngx_str_t*)ngx_palloc(r->pool, sizeof(ngx_str_t));
	str->len = 0;
	str->data = (u_char*)"";
	return str;
}

static ngx_str_t*
ngx_http_51D_get_user_agent(ngx_http_request_t *r, ngx_http_51D_header_to_set *header)
{
	u_char *src, *dst;
	ngx_uint_t variableNameHash;
	ngx_variable_value_t *variable;
	ngx_str_t *userAgent;
	// Get the User-Agent from the request or variable.
	if (header != NULL && (int)header->variableName.len > 0) {
		variableNameHash = ngx_hash_strlow(header->variableName.data,
							header->variableName.data,
							header->variableName.len);
		variable = ngx_http_get_variable(r, &header->variableName, variableNameHash);
		if ((int)variable->len > 0) {
			userAgent = (ngx_str_t*)ngx_palloc(r->pool, sizeof(ngx_str_t));
			userAgent->data = ngx_palloc(r->pool, sizeof(char*) * (size_t)variable->len);
			src = variable->data;
			dst = userAgent->data;
			ngx_unescape_uri(&dst, &src, variable->len, 0);
			userAgent->len = dst - userAgent->data;
		}
		else {
			userAgent = emptyString(r);
		}
	}
	else if (r->headers_in.user_agent != NULL) {
		userAgent = &r->headers_in.user_agent[0].value;
	}
	else {
		userAgent = emptyString(r);
	}
	return userAgent;
}

int
process(ngx_http_request_t *r,
		ngx_http_51D_main_conf_t *fdmcf,
		ngx_http_51D_header_to_set *header,
		ngx_table_elt_t *h[],
		int matchIndex,
		int haveMatch,
		ngx_str_t *userAgent) {
#ifdef FIFTYONEDEGREES_PATTERN
	if ((int)header->maxString < 0) {
		ngx_http_51D_set_max_string(fdmcf, header);
	}
#endif // FIFTYONEDEGREES_PATTERN
	// Allocate the value string in the request pool.
	char *valueString = (char*)ngx_palloc(r->pool, header->maxString + 1);
	valueString[0] = '\0';
	// Get a match. If there are multiple instances of
	// 51D_match_single or 51D_match_all, then don't get the
	// match if it has already been fetched.
	if (haveMatch == 0) {
		ngx_http_51D_get_match(fdmcf, r, header->multi, userAgent);
	}
	// For each property, set the value in values_string_array.
	int property_index;
	for (property_index = 0; property_index < (int)header->propertyCount; property_index++) {
		ngx_http_51D_get_value(fdmcf, r, valueString, (const char*) header->property[property_index]->data, header->maxString);
	}

	// Escape characters which cannot be added in a header value, most importantly '\n' and '\r'.
	size_t valueStringLength = strlen(valueString);
	size_t escapedChars = (size_t)ngx_escape_json(NULL, (u_char*)valueString, valueStringLength);
	u_char* escapedValueString = (u_char*)ngx_palloc(r->pool, (valueStringLength + escapedChars) * sizeof(char));
	ngx_escape_json(escapedValueString, (u_char*)valueString, valueStringLength);
	escapedValueString[valueStringLength + escapedChars] = '\0';

	// For each property value pair, set a new header name and value.
	h[matchIndex] = ngx_list_push(&r->headers_in.headers);
	h[matchIndex]->key.data = (u_char*)header->name.data;
	h[matchIndex]->key.len = ngx_strlen(h[matchIndex]->key.data);
	h[matchIndex]->hash = ngx_hash_key(h[matchIndex]->key.data, h[matchIndex]->key.len);
	h[matchIndex]->value.data = escapedValueString;
	h[matchIndex]->value.len = ngx_strlen(h[matchIndex]->value.data);
	h[matchIndex]->lowcase_key = (u_char*)header->lowerName.data;
	return 1;
}

/**
 * Module handler, gets a match using either the User-Agent or multiple http
 * headers, then sets properties as headers.
 * @param r the HTTP request.
 * @return ngx_int_t nginx status.
 */
static ngx_int_t
ngx_http_51D_handler(ngx_http_request_t *r)
{

	ngx_http_51D_main_conf_t *fdmcf;
	ngx_http_51D_srv_conf_t *fdscf;
	ngx_http_51D_loc_conf_t *fdlcf;
	ngx_http_51D_match_conf_t *matchConf[3];
	ngx_uint_t matchIndex = 0, multi, haveMatch;
	ngx_http_51D_header_to_set *currentHeader;
	int totalHeaderCount, matchConfIndex;
	ngx_str_t *userAgent, *nextUserAgent;
#ifdef FIFTYONEDEGREES_PATTERN
	ngx_http_51D_matched_header_t *header;
	ngx_http_51D_cache_node_t *node;
	uint32_t hash;
	ngx_str_t cacheKey;
	u_char cacheKeyData[FIFTYONEDEGREES_CACHE_KEY_LENGTH];
	cacheKey.data = cacheKeyData;
	int headerIndex;
	ngx_table_elt_t *searchResult;
	ngx_slab_pool_t *shpool;
#endif // FIFTYONEDEGREES_PATTERN

	if (r->main->internal || ngx_http_51D_shm_dataSet == NULL) {
		return NGX_DECLINED;
	}
	r->main->internal = 1;

	// Get 51Degrees configs.
	fdlcf = ngx_http_get_module_loc_conf(r, ngx_http_51D_module);
	fdscf = ngx_http_get_module_srv_conf(r, ngx_http_51D_module);
	fdmcf = ngx_http_get_module_main_conf(r, ngx_http_51D_module);

	matchConf[0] = &fdlcf->matchConf;
	matchConf[1] = &fdscf->matchConf;
	matchConf[2] = &fdmcf->matchConf;

	totalHeaderCount = matchConf[0]->headerCount +
					   matchConf[1]->headerCount +
					   matchConf[2]->headerCount;

	ngx_table_elt_t *h[totalHeaderCount];

	userAgent = ngx_http_51D_get_user_agent(r, NULL);

	// If no headers are specified in this location, tell nginx
	// this handler is done.
	if ((int)totalHeaderCount == 0) {
		return NGX_DECLINED;
	}
#ifdef FIFTYONEDEGREES_PATTERN
	hash = 0;
	cacheKey.len = 0;
	if (ngx_http_51D_cacheSize > 0) {
		shpool = (ngx_slab_pool_t*)ngx_http_51D_shm_cache->shm.addr;
		// Get the hash for this location and request headers.
		hash = ngx_hash(hash, matchConf[0]->key);
		hash = ngx_hash(hash, matchConf[1]->key);
		hash = ngx_hash(hash, matchConf[2]->key);
		hash = ngx_hash(hash, ngx_hash_key(userAgent->data, userAgent->len));
		for (headerIndex = 0; headerIndex < fdmcf->dataSet->httpHeadersCount; headerIndex++) {
			searchResult = search_headers_in(r, (u_char*)fdmcf->dataSet->httpHeaders[headerIndex].headerName, ngx_strlen(fdmcf->dataSet->httpHeaders[headerIndex].headerName));
			if (searchResult != NULL) {
				ngx_http_51D_cache_append_key(&cacheKey, &searchResult->value, sizeof(cacheKeyData));
				hash = ngx_hash(hash, ngx_hash_key(searchResult->value.data, searchResult->value.len));
			}
		}
		// Also hash any parameters which are matched.
		for (matchConfIndex = 0; matchConfIndex < 3; matchConfIndex++)
		{
			currentHeader = matchConf[matchConfIndex]->header;
			while (currentHeader != NULL)
			{
				if (currentHeader->multi == 0 && (int)currentHeader->variableName.len > 0) {
					nextUserAgent = ngx_http_51D_get_user_agent(r, currentHeader);
					ngx_http_51D_cache_append_key(&cacheKey, nextUserAgent, sizeof(cacheKeyData));
					hash = ngx_hash(hash, ngx_hash_key(nextUserAgent->data, nextUserAgent->len));
				}
				currentHeader = currentHeader->next;
			}
		}

		// Look in the cache for a match with the hash.
		ngx_shmtx_lock(&shpool->mutex);
		node = ngx_http_51D_lookup_node_lru(&cacheKey, hash);
	}
	else {
		node = NULL;
	}
	// If a match is not found in the cache, carry out a match.
	if (node == NULL) {
		// Release the cache lock as it is not needed while matching.
		if (ngx_http_51D_cacheSize > 0) {
			ngx_shmtx_unlock(&shpool->mutex);
		}
#endif // FIFTYONEDEGREES_PATTERN

		// Look for single User-Agent matches, then multiple HTTP header
		// matches. Single and multi matches are done separately to reuse
		// a match instead of retrieving it multiple times.
		for (multi = 0; multi < 2; multi++) {
			haveMatch = 0;

			// Go through the requested matches in location, server and
			// main configs.
			for (matchConfIndex = 0; matchConfIndex < 3; matchConfIndex++)
			{

				// Loop over all match instances in the config.
				currentHeader = matchConf[matchConfIndex]->header;
				while (currentHeader != NULL)
				{
					// Process the match if it is the type we are looking for on
					// this pass.
					if (currentHeader->multi == multi && (int)currentHeader->variableName.len < 0) {
						haveMatch = process(r, fdmcf, currentHeader, h, matchIndex, haveMatch, userAgent);
						matchIndex++;
					}
					// Look at the next header.
					currentHeader = currentHeader->next;
				}
			}
		}

		userAgent = &(ngx_str_t)ngx_string("");
		for (matchConfIndex = 0; matchConfIndex < 3; matchConfIndex++)
		{
			currentHeader = matchConf[matchConfIndex]->header;
			while (currentHeader != NULL)
			{
				if (currentHeader->multi == 0 && (int)currentHeader->variableName.len > 0) {
					nextUserAgent = ngx_http_51D_get_user_agent(r, currentHeader);
					if (strcmp((const char*)userAgent->data, (const char*)nextUserAgent->data) == 0) {
						process(r, fdmcf, currentHeader, h, matchIndex, 1, userAgent);
					} else {
						userAgent = nextUserAgent;
						process(r, fdmcf, currentHeader, h, matchIndex, 0, userAgent);
					}
					matchIndex++;
				}
				currentHeader = currentHeader->next;
			}
		}

#ifdef FIFTYONEDEGREES_PATTERN
		// Add the match to the cache.
		if (ngx_http_51D_cacheSize > 0) {
			// Create a new node so the match can be added to the cache.
			node = ngx_http_51D_create_node(&cacheKey, hash, totalHeaderCount);
			while (node->headersCount < matchIndex) {
				ngx_http_51D_add_header_to_node(node, h[node->headersCount]);
			}
			ngx_http_51D_cache_insert(node);
		}
	}
	else {
		// The match already exists in the cache, so set the headers from
		// the node.
		for (matchIndex = 0; (int)matchIndex < totalHeaderCount; matchIndex++) {
			header = node->header[(int)matchIndex];
			h[matchIndex] = ngx_list_push(&r->headers_in.headers);
			h[matchIndex]->key = header->name;
			h[matchIndex]->hash = ngx_hash_key(h[matchIndex]->key.data, h[matchIndex]->key.len);
			h[matchIndex]->value = header->value;
			h[matchIndex]->lowcase_key = header->lowerName;
		}
		ngx_shmtx_unlock(&shpool->mutex);
	}
#endif // FIFTYONEDEGREES_PATTERN

	// Tell nginx to continue with other module handlers.
	return NGX_DECLINED;
}

/**
 * Set header function. Initialises the header structure for a given occurrence
 * of "51D_match_single" or "51D_match_all" in the config file. Allocates
 * space required and sets the name and properties.
 * @param cf the nginx config.
 * @param header the header to be set.
 * @param value the values passed from the config file.
 * @param fdmcf 51Degrees main config.
 */
char*
ngx_http_51D_set_header(ngx_conf_t *cf, ngx_http_51D_header_to_set *header, ngx_str_t *value, ngx_http_51D_main_conf_t *fdmcf)
{
	char *tok;
	int propertiesCount, charPos;
	char *propertiesString;

	// Initialise the property count.
	header->propertyCount = 0;
#ifdef FIFTYONEDEGREES_PATTERN
	// Initialise the max string.
	header->maxString = NGX_CONF_UNSET_SIZE;
#endif // FIFTYONEDEGREES_PATTERN
#ifdef FIFTYONEDEGREES_TRIE
	// Set the max string.
	header->maxString = (size_t)FIFTYONEDEGREES_MAX_STRING * sizeof(char*);
#endif // FIFTYONEDEGREES_TRIE

	// Set the name of the header.
	header->name.data = (u_char*)ngx_palloc(cf->pool, sizeof(value[1]));
	header->lowerName.data = (u_char*)ngx_palloc(cf->pool, sizeof(value[1]));
	header->name.data = value[1].data;
	header->name.len = value[1].len;
	ngx_strlow(header->lowerName.data, header->name.data, header->name.len);
	header->lowerName.len = header->name.len;

	// Set the properties string to the second argument in the config file.
    propertiesString = (char*)value[2].data;

	// Count the properties in the string.
	propertiesCount = 1;
	for (charPos = 0; charPos < (int)value[2].len; charPos++) {
		if (propertiesString[charPos] == ',') {
			propertiesCount++;
		}
	}
	// Allocate space for the properties array.
	header->property = (ngx_str_t**)ngx_palloc(cf->pool, sizeof(ngx_str_t*)*propertiesCount);

	// Allocate and set all the properties.
	tok = strtok((char*)propertiesString, (const char*)",");
	while (tok != NULL) {
		header->property[header->propertyCount] = (ngx_str_t*)ngx_palloc(cf->pool, sizeof(ngx_str_t));
		header->property[header->propertyCount]->data = (u_char*)ngx_palloc(cf->pool, sizeof(u_char)*(ngx_strlen(tok)));
		header->property[header->propertyCount]->data = (u_char*)tok;
		header->property[header->propertyCount]->len = ngx_strlen(header->property[header->propertyCount]->data);
		if (ngx_strstr(fdmcf->properties, tok) == NULL) {
			add_value(",", tok, fdmcf->properties, FIFTYONEDEGREES_MAX_STRING - strlen(fdmcf->properties));
		}
		header->propertyCount++;
		tok = strtok(NULL, ",");
	}

	// Set the variable name if one is specified.
	if ((int)cf->args->nelts == 4) {
		if (value[3].data[0] == '$') {
			header->variableName.data = value[3].data + 1;
			header->variableName.len = value[3].len -1;
		}
		else {
			ngx_log_error(NGX_LOG_ERR,
							cf->cycle->log,
							0,
							"Argument '%s' is a string, this should be a variable",
							(const char*)value[3].data);
			return NGX_CONF_ERROR;
		}
	}
	else {
		header->variableName.data = NULL;
		header->variableName.len = -1;
	}
	return NGX_OK;
}

/**
 * Set function. Is called for each occurrence of "51D_match_single" or
 * "51D_match_all". Allocates space for the header structure and initialises
 * it with the set header function.
 * @param cf the nginx conf.
 * @param cmd the name of the command called from the config file.
 * @param matchConf the match config.
 * @return char* nginx conf status.
 */
static char *ngx_http_51D_set(ngx_conf_t *cf, ngx_command_t *cmd, ngx_http_51D_match_conf_t *matchConf)
{
	ngx_http_51D_main_conf_t *fdmcf;
	ngx_http_51D_header_to_set *header;
	ngx_str_t *value;
	char *status;

	// Get the arguments from the config.
	value = cf->args->elts;

	// Get the modules location and main config.
	fdmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_51D_module);

	// Get the end of the header list.
	if (matchConf->header == NULL)
	{
		// Allocate the space for the first header in the current location config.
		matchConf->header = (ngx_http_51D_header_to_set*)ngx_palloc(cf->pool, sizeof(ngx_http_51D_header_to_set));
		header = matchConf->header;
	}
	else
	{
		header = matchConf->header;
		while (header->next != NULL)
		{
			header = header->next;
		}
		// Allocate the space for the next header in the current location config.
		header->next = (ngx_http_51D_header_to_set*)ngx_palloc(cf->pool, sizeof(ngx_http_51D_header_to_set));
		header = header->next;
	}

	// Set the next pointer to NULL to show it is the last in the list.
	header->next = NULL;

	// Enable single User-Agent matching.
	if (ngx_strcmp(cmd->name.data, "51D_match_single") == 0) {
		header->multi = 0;
	}
	// Enable multiple HTTP header matching.
	else if (ngx_strcmp(cmd->name.data, "51D_match_all") == 0) {
		header->multi = 1;
		matchConf->hasMulti = 1;
	}

	// Set the properties for the selected location.
	status = ngx_http_51D_set_header(cf, header, value, fdmcf);
	if (status != NGX_OK) {
		return status;
	}

#ifdef FIFTYONEDEGREES_PATTERN
	// Add the arguments to the locations hash key, this is used to
	// differentiate locations in the cache.
	matchConf->key = ngx_hash(matchConf->key, ngx_hash_key(value[1].data, value[1].len));
	matchConf->key = ngx_hash(matchConf->key, ngx_hash_key(value[2].data, value[2].len));
#endif // FIFTYONEDEGREES_PATTERN

	matchConf->headerCount++;

	return NGX_OK;
}

/**
 * Set function. Is called for occurrences of "51D_match_single" or
 * "51D_match_all" in a location config block. Allocates space for the
 * header structure and initialises it with the set header function.
 * @param cf the nginx conf.
 * @param cmd the name of the command called from the config file.
 * @param match the module config.
 * @return char* nginx conf status.
 */
static char *ngx_http_51D_set_loc(ngx_conf_t* cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_51D_loc_conf_t *fdlcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_51D_module);
	return ngx_http_51D_set(cf, cmd, &fdlcf->matchConf);
}

/**
 * Set function. Is called for occurrences of "51D_match_single" or
 * "51D_match_all" in a server config block. Allocates space for the
 * header structure and initialises it with the set header function.
 * @param cf the nginx conf.
 * @param cmd the name of the command called from the config file.
 * @param match the module config.
 * @return char* nginx conf status.
 */
static char *ngx_http_51D_set_srv(ngx_conf_t* cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_51D_srv_conf_t *fdscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_51D_module);
	return ngx_http_51D_set(cf, cmd, &fdscf->matchConf);
}

/**
 * Set function. Is called for occurrences of "51D_match_single" or
 * "51D_match_all" in a http config block. Allocates space for the
 * header structure and initialises it with the set header function.
 * @param cf the nginx conf.
 * @param cmd the name of the command called from the config file.
 * @param match the module config.
 * @return char* nginx conf status.
 */
static char *ngx_http_51D_set_main(ngx_conf_t* cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_51D_main_conf_t *fdmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_51D_module);
	return ngx_http_51D_set(cf, cmd, &fdmcf->matchConf);
}
