#include <ngramsLocations.h>

#ifndef _OSM_CLASSIFIER_H_
#define _OSM_CLASSIFIER_H_

ngramsLocations_t *osmClassifier_init ( const char *filename );
LatLongList_t *osmClassifier_lookup ( ngramsLocations_t *ngramsLocations, const char *phrase );

#endif
