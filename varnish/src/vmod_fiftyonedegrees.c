/*
 * This Source Code Form is copyright of 51Degrees Mobile Experts Limited.
 * Copyright 2015 51Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY
 *
 * This Source Code Form is the subject of the following patent
 * applications, owned by 51Degrees Mobile Experts Limited of 5 Charlotte
 * Close, Caversham, Reading, Berkshire, United Kingdom RG4 7BY:
 * European Patent Application No. 13192291.6; and
 * United States Patent Application Nos. 14/085,223 and 14/085,301.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.
 *
 * If a copy of the MPL was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 *
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "vrt.h"
#include "cache/cache.h"
#include "src/pattern/51Degrees.h"

#include "vcc_if.h"

typedef enum { false, true } bool;

// Global provider available to the module.
fiftyoneDegreesProvider *provider;

// Array of pointers to the important header name strings.
const char **importantHeaders;

static void
addValue(
		const char *delimiter,
		char *buffer, const char *str)
{
	if (buffer[0] != '\0')
	{
		strcat(buffer, delimiter);
	}
	strcat(buffer, str);
}

static void
initHttpHeaders()
{
	importantHeaders = (const char**)malloc(sizeof(char*) * provider->activePool->dataSet->httpHeadersCount);
	for (int httpHeaderIndex = 0;
		httpHeaderIndex < provider->activePool->dataSet->httpHeadersCount;
		httpHeaderIndex++) {
			importantHeaders[httpHeaderIndex] = provider->activePool->dataSet->httpHeaders[httpHeaderIndex].headerName;
	}
}

int cacheSize = 0,
poolSize = 20;
const char *requiredProperties = "";
const char *propertyDelimiter = ",";

VCL_STRING vmod_get_version(const struct vrt_ctx *ctx)
{
	char *p;
	unsigned u, v;

	// Reserve some work space.
	u = WS_Reserve(ctx->ws, 0);
	// Get pointer to the front of the work space.
	p = ctx->ws->f;
	// Print the value to memory that has been reserved.
	v = snprintf(p, u, "%d.%d.%d.%d", provider->activePool->dataSet->header.versionMajor,
				provider->activePool->dataSet->header.versionMinor,
				provider->activePool->dataSet->header.versionBuild,
				provider->activePool->dataSet->header.versionRevision);

	v++;

	if (v > u) {
		// No space, reset and leave.
		WS_Release(ctx->ws, 0);
		return (NULL);
	}
	// Update work space with what has been used.
	WS_Release(ctx->ws, v);
	return (p);
}

void vmod_set_cache(const struct vrt_ctx *ctx, VCL_INT size)
{
	cacheSize = size;
}

void vmod_set_properties(const struct vrt_ctx *ctx, VCL_STRING properties)
{
	requiredProperties = properties;
}

void vmod_set_pool(const struct vrt_ctx *ctx, VCL_INT size)
{
	poolSize = size;
}

void vmod_set_delimiter(const struct vrt_ctx *ctx, VCL_STRING delimiter)
{
	propertyDelimiter = delimiter;
}

void
vmod_start(
		const struct vrt_ctx *ctx,
		VCL_STRING filePath)
{
    // Allocate and initialise the provider.
	provider = (fiftyoneDegreesProvider*)malloc(sizeof(fiftyoneDegreesProvider));
	fiftyoneDegreesInitProviderWithPropertyString(
        filePath,
        provider,
        requiredProperties,
        poolSize,
        cacheSize);

	// Get the names of the important headers from the data set.
	initHttpHeaders();
}

char*
searchHeaders(const struct vrt_ctx *ctx, const char *headerName)
{
	char *currentHeader;
	int i;
	for (i = 0; i < ctx->http_req->nhd; i++)
	{
		currentHeader = ctx->http_req->hd[i].b;
		if (currentHeader != NULL
			&& strncmp(currentHeader, headerName, strlen(headerName)) == 0)
		{
			return currentHeader + strlen(headerName) + 2;
		}
	}

	return NULL;
}

char*
getValue(
		fiftyoneDegreesWorkset *fodws,
		const char *requiredPropertyName)
{
	int i, j;
    // TODO set max buffer length properly.
    char *valueName = malloc(sizeof(char) * fiftyoneDegreesGetMaxPropertyValueLength(provider->activePool->dataSet, (char*)requiredPropertyName));
    valueName[0] = '\0';
	bool found = false;
	char *currentPropertyName;
	    // Get the requested property value from the match.
    if (strcmp(requiredPropertyName, "Method") == 0)
    {
		switch(fodws->method) {
			case EXACT: sprintf(valueName, "Exact"); break;
			case NUMERIC: sprintf(valueName, "Numeric"); break;
			case NEAREST: sprintf(valueName, "Nearest"); break;
			case CLOSEST: sprintf(valueName, "Closest"); break;
			default:
			case NONE: sprintf(valueName, "None"); break;
		}
    }
    else if (strcmp(requiredPropertyName, "Difference") == 0)
    {
        sprintf(valueName, "%d", fodws->difference);
    }
    else if (strcmp(requiredPropertyName, "DeviceId") == 0)
    {
        fiftyoneDegreesGetDeviceId(fodws, valueName, 24);
    }
    else if (strcmp(requiredPropertyName, "Rank") == 0)
    {
        sprintf(valueName, "%d", fiftyoneDegreesGetSignatureRank(fodws));
    }
	else {
        // Property is not a match metric, so search the required properties.
        for (i = 0; i < fodws->dataSet->requiredPropertyCount; i++)
        {
            currentPropertyName
            = (char*)fiftyoneDegreesGetPropertyName(fodws->dataSet, fodws->dataSet->requiredProperties[i]);
            if (strcmp(currentPropertyName, requiredPropertyName) == 0)
            {
                fiftyoneDegreesSetValues(fodws, i);
                for (j = 0; j < fodws->valuesCount; j++)
                {
                	addValue("|", valueName, fiftyoneDegreesGetValueName(fodws->dataSet, fodws->values[j]));
                }
                found = true;
                break;
            }
        }
        if (!found)
            // Property was not found, so set value accordingly.
            sprintf(valueName, FIFTYONEDEGREES_PROPERTY_NOT_FOUND);
    }
    return valueName;
}

VCL_STRING
vmod_match_single(
				const struct vrt_ctx *ctx,
				VCL_STRING userAgent,
				VCL_STRING propertyInputString)
{
	char *p;
	unsigned u, v;

    // Create a workset to use for the match.
	fiftyoneDegreesWorkset *fodws
		= fiftyoneDegreesProviderWorksetGet(provider);

	// Get a match for the User-Agent supplied and store in the workset.
	fiftyoneDegreesMatch(fodws, userAgent);

	// Get the value of the requested property and store as valueName.
	char *valueName = getValue(fodws, propertyInputString);

	// Reserve some work space.
	u = WS_Reserve(ctx->ws, 0);
	// Get pointer to the front of the work space.
	p = ctx->ws->f;
	// Print the value to memory that has been reserved.
	v = snprintf(p, u, "%s", valueName);
	free(valueName);

	v++;

    // Free the workset.
    fiftyoneDegreesWorksetRelease(fodws);

	if (v > u) {
		// No space, reset and leave.
		WS_Release(ctx->ws, 0);
		return (NULL);
	}
	// Update work space with what has been used.
	WS_Release(ctx->ws, v);
	return (p);
}

VCL_STRING
vmod_match_all(
			const struct vrt_ctx *ctx,
			VCL_STRING propertyInputString)
{
	char *p;
	unsigned u, v;
	int headerIndex;
	char *searchResult;

    // Create a workset to use for the match.
	fiftyoneDegreesWorkset *fodws
		= fiftyoneDegreesProviderWorksetGet(provider);

    // Reset the headers count before adding any to the workset.
	fodws->importantHeadersCount = 0;
	// Loop over all important headers.
	for (headerIndex = 0;
	     headerIndex < fodws->dataSet->httpHeadersCount;
	     headerIndex++)
	{
		// Look for the current header in the request.
		searchResult
			= searchHeaders(ctx, fodws->dataSet->httpHeaders[headerIndex].headerName);
		if (searchResult) {
			// The request contains the header, so add it to the important
			// headers in the workset.
			fodws->importantHeaders[fodws->importantHeadersCount].header
				= fodws->dataSet->httpHeaders + headerIndex;
			fodws->importantHeaders[fodws->importantHeadersCount].headerValue
				= (const char*)searchResult;
			fodws->importantHeaders[fodws->importantHeadersCount].headerValueLength
				= strlen(searchResult);
			fodws->importantHeadersCount++;
		}
	}

	// Get a match for the headers that have just been added and store in
	// the workset.
	fiftyoneDegreesMatchForHttpHeaders(fodws);

	// Get the value for the requested property and store as valueName.
	char *valueName = getValue(fodws, propertyInputString);

	// Reserve some work space.
	u = WS_Reserve(ctx->ws, 0);
	// Pointer to the front of work space area.
	p = ctx->ws->f;
	// Print the value to the memory that has been reserved.
	v = snprintf(p, u, "%s", valueName);
	free(valueName);

	v++;

    // Free the workset.
    fiftyoneDegreesWorksetRelease(fodws);

	if (v > u) {
		// No space, reset and leave.
		WS_Release(ctx->ws, 0);
		return (NULL);
	}
	// Update work space with what has been used.
	WS_Release(ctx->ws, v);
	return (p);
}
