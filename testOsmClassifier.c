#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <osmClassifier.h>

int main ( int argc, char **argv ) {
    int rc = EXIT_FAILURE;
    char *ngtfile = NULL;
    ngramsLocations_t *ngramsLocations = NULL;
    size_t i = 0, j = 0;

    if ( argc < 2 ) {
        fprintf ( stderr, "Usage: %s ngtfile\n", argv[0] );
        goto over;
    }

    ngtfile = argv[1];

    if ( ( ngramsLocations = osmClassifier_init ( ngtfile ) ) == NULL ) {
        fprintf ( stderr, "Cannot initialise osm classifier from file %s: %s\n", ngtfile, strerror ( errno ) );
        goto over;
    }

    for ( i = 0; i < ngramsLocations->size; i++ ) {
        hashTable_entry_t *entry = ngramsLocations->entries[i];

        while ( entry ) {
            LatLongList_t *lll = NULL;

            if ( ( lll = osmClassifier_lookup ( ngramsLocations, entry->key ) ) == NULL ) {
                fprintf ( stderr, "Cannot perform lookup for entry \"%s\": %s\n", entry->key, strerror ( errno ) );
                goto over;
            }

            /*if ( strncmp ( entry->key, lll->ngram, strlen ( entry->key ) ) ) {
                fprintf ( stderr, "Entry key (%s) does not match lat/lon list ngram (%s)\n", entry->key, lll->ngram );
                goto over;
            }*/

            printf ( "%s: ", entry->key );

            for ( j = 0; j < lll->size; j++ ) {
                printf ( "%lf,%lf ", (double) lll->lat_lon[j].lat, (double) lll->lat_lon[j].lon );
            }

            printf ( "\n" );

            entry = entry->next;
        }
    }

    rc = EXIT_SUCCESS;
over:
    if ( ngramsLocations )
        ngramsLocations_free ( ngramsLocations );

    return rc;
}
