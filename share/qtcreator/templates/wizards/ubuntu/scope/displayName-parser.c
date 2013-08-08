/*
 * Feed parser backend
 *
 * In this module you'll be implementing the feed parser backend.
 * This code will not be Unity-specific, and it will vary depending
 * on the type of data returned by your source. Common feed types
 * returned when doing search queries to sources are JSON, RSS, XML.
 *
 * You'll simply need to implement a parser for the type of feed your
 * source returns and send the results back to Unity via the get_results()
 * function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "%DISPLAYNAME_LOWER%-parser.h"

/*
 * This is the URI against which you'll be submitting your
 * search query from the Dash, which you'll need to adapt
 * to your source.
 */
#define BASE_URI "http://%DISPLAYNAME_LOWER%.org/api/search/?query="

/**
 * @brief Cleans up (frees the memory allocated memory)
 *        of the given results structure
 * @param data Structure that contains the data to free
 */
void
result_cleanup(gpointer data) {
    /* This is specific to a type of result, so you'll
     * need to adapt it to the results sent by your
     * source.
     */
    result_t *result = (result_t *)data;
    if (result->link) {
        free(result->link);
    }
    if (result->icon_url) {
        free(result->icon_url);
    }
    if (result->title) {
        free(result->title);
    }
    if (result->description) {
        free(result->description);
    }
    if (result->creation_date) {
        free(result->creation_date);
    }
    if (result->author) {
        free(result->author);
    }
}

/**
 * @brief get_results Get and parse the results from a search query
 * @param search_term String submitted as the search term
 * @return Search results
 */
GSList *
get_results(const char *search_term) {
    GString *url = NULL;
    GSList *results = NULL;

    /* Check if an actual search term was submitted, return otherwise */
    if (search_term == NULL) {
        g_warning("get_results: search_term cannot be null");
        return results;
    }

    /* Construct the full search query */
    url = g_string_new(BASE_URI);
    g_string_append(url, search_term);
    g_debug("Searching %s", url->str);
    g_string_free(url, TRUE);

    /*
     * Submit the query to the source, parse the results and populate
     * the results list here.
     */

    return results;
}
