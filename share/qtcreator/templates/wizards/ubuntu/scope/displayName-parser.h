#ifndef __%DISPLAYNAME_UPPER%_PARSER_H__
#define __%DISPLAYNAME_UPPER%_PARSER_H__

GSList *get_results(const char *search_term);
void result_cleanup(gpointer data);

/**
 * This is just an example result type with some sample
 * fields. You should modify the fields to match the
 * data you are expecting from your search source
 */
typedef struct {
    gchar *link;
    gchar *icon_url;
    gchar *title;
    gchar *description;
    gchar *creation_date;
    gchar *author;
} result_t;

#endif /* __%DISPLAYNAME_UPPER%_PARSER_H__ */
