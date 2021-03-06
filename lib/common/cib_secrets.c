/*
 * Copyright (c) 2011 SUSE, Attachmate
 * Author: Dejan Muhamedagic <dejan@suse.de>
 *
 * This source code is licensed under the GNU Lesser General Public License
 * version 2.1 or later (LGPLv2.1+) WITHOUT ANY WARRANTY.
 */

#include <crm_internal.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <glib.h>

#include <crm/common/util.h>
#include <crm/common/cib_secrets.h>

static int is_magic_value(char *p);
static int check_md5_hash(char *hash, char *value);
static void add_secret_params(gpointer key, gpointer value, gpointer user_data);
static char *read_local_file(char *local_file);

#define MAX_VALUE_LEN 255
#define MAGIC "lrm://"

static int
is_magic_value(char *p)
{
    return !strcmp(p, MAGIC);
}

static int
check_md5_hash(char *hash, char *value)
{
    int rc = FALSE;
    char *hash2 = NULL;

    hash2 = crm_md5sum(value);
    crm_debug("hash: %s, calculated hash: %s", hash, hash2);
    if (safe_str_eq(hash, hash2)) {
        rc = TRUE;
    }

    free(hash2);
    return rc;
}

static char *
read_local_file(char *local_file)
{
    FILE *fp = fopen(local_file, "r");
    char buf[MAX_VALUE_LEN+1];
    char *p;

    if (!fp) {
        if (errno != ENOENT) {
            crm_perror(LOG_ERR, "cannot open %s" , local_file);
        }
        return NULL;
    }

    if (!fgets(buf, MAX_VALUE_LEN, fp)) {
        crm_perror(LOG_ERR, "cannot read %s", local_file);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    /* strip white space */
    for (p = buf+strlen(buf)-1; p >= buf && isspace(*p); p--)
		;
    *(p+1) = '\0';
    return strdup(buf);
}

/*
 * returns 0 on success or no replacements necessary
 * returns -1 if replacement failed for whatever reasone
 */

int
replace_secret_params(char *rsc_id, GHashTable *params)
{
    char local_file[FILENAME_MAX+1], *start_pname;
    char hash_file[FILENAME_MAX+1], *hash;
    GList *secret_params = NULL, *l;
    char *key, *pvalue, *secret_value;
    int rc = 0;

    if (params == NULL) {
        return 0;
    }

    /* secret_params could be cached with the resource;
     * there are also parameters sent with operations
     * which cannot be cached
     */
    g_hash_table_foreach(params, add_secret_params, &secret_params);
    if (!secret_params) { /* none found? */
        return 0;
    }

    crm_debug("replace secret parameters for resource %s", rsc_id);

    if (snprintf(local_file, FILENAME_MAX, LRM_CIBSECRETS_DIR "/%s/", rsc_id)
            > FILENAME_MAX) {
        crm_err("filename size exceeded for resource %s", rsc_id);
        return -1;
    }
    start_pname = local_file + strlen(local_file);

    for (l = g_list_first(secret_params); l; l = g_list_next(l)) {
        key = (char *)(l->data);
        pvalue = g_hash_table_lookup(params, key);
        if (!pvalue) { /* this cannot really happen */
            crm_err("odd, no parameter %s for rsc %s found now", key, rsc_id);
            continue;
        }

        if ((strlen(key) + strlen(local_file)) >= FILENAME_MAX-2) {
            crm_err("%d: parameter name %s too big", key);
            rc = -1;
            continue;
        }

        strcpy(start_pname, key);
        secret_value = read_local_file(local_file);
        if (!secret_value) {
            crm_err("secret for rsc %s parameter %s not found in %s",
                    rsc_id, key, LRM_CIBSECRETS_DIR);
            rc = -1;
            continue;
        }

        strcpy(hash_file, local_file);
        if (strlen(hash_file) + 5 > FILENAME_MAX) {
            crm_err("cannot build such a long name "
                    "for the sign file: %s.sign", hash_file);
            free(secret_value);
            rc = -1;
            continue;

        } else {
            strncat(hash_file, ".sign", 5);
            hash = read_local_file(hash_file);
            if (hash == NULL) {
                crm_err("md5 sum for rsc %s parameter %s "
                        "cannot be read from %s", rsc_id, key, hash_file);
                free(secret_value);
                rc = -1;
                continue;

            } else if (!check_md5_hash(hash, secret_value)) {
                crm_err("md5 sum for rsc %s parameter %s "
                        "does not match", rsc_id, key);
                free(secret_value);
                free(hash);
                rc = -1;
                continue;
            }
            free(hash);
        }
        g_hash_table_replace(params, strdup(key), secret_value);
    }
    g_list_free(secret_params);
    return rc;
}

static void
add_secret_params(gpointer key, gpointer value, gpointer user_data)
{
    GList **lp = (GList **)user_data;

    if (is_magic_value((char *)value)) {
	*lp = g_list_append(*lp, (char *)key);
    }
}
