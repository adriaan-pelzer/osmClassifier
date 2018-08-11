#include <ngramsLocations.h>

ngramsLocations_t *osmClassifier_init ( const char *filename ) {
    return ngramsLocations_deserialise ( filename );
}

LatLongList_t *osmClassifier_lookup ( ngramsLocations_t *ngramsLocations, const char *phrase ) {
    return (LatLongList_t *) hashTable_find_entry_value ( (hashTable_t *) ngramsLocations, phrase );
}
