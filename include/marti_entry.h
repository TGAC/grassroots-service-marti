/*
 * marti_entry.h
 *
 *  Created on: 30 Jan 2024
 *      Author: billy
 */

#ifndef SERVICES_MARTI_INCLUDE_MARTI_ENTRY_H_
#define SERVICES_MARTI_INCLUDE_MARTI_ENTRY_H_

#include <time.h>

#include "bson/bson.h"

#include "marti_service_library.h"
#include "marti_service_data.h"
#include "user_details.h"
#include "permission.h"


typedef struct
{
	bson_oid_t *me_id_p;

	User *me_user_p;

	bool me_owns_user_flag;

	PermissionsGroup *me_permissions_group_p;

	char *me_name_s;

	char *me_marti_id_s;

	double64 me_latitude;

	double64 me_longitude;

	struct tm *me_start_p;

	struct tm *me_end_p;

} MartiEntry;



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_MARTI_ENTRY_TAGS
	#define MARTI_ENTRY_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define MARTI_ENTRY_VAL(x)	= x
	#define MARTI_ENTRY_CONCAT_VAL(x,y)	= x y
#else
	#define MARTI_ENTRY_PREFIX extern
	#define MARTI_ENTRY_VAL(x)
	#define MARTI_ENTRY_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */




MARTI_ENTRY_PREFIX const char *ME_NAME_S MARTI_ENTRY_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "name");
MARTI_ENTRY_PREFIX const char *ME_MARTI_ID_S MARTI_ENTRY_VAL ("marti_id");
MARTI_ENTRY_PREFIX const char *ME_LOCATION_S MARTI_ENTRY_VAL ("location");
MARTI_ENTRY_PREFIX const char *ME_START_DATE_S MARTI_ENTRY_VAL ("start_date");
MARTI_ENTRY_PREFIX const char *ME_END_DATE_S MARTI_ENTRY_VAL ("end_date");



#ifdef __cplusplus
extern "C"
{
#endif



MARTI_SERVICE_LOCAL MartiEntry *AllocateMartiEntry (bson_oid_t *id_p, User *user_p, PermissionsGroup *permissions_group_p, const bool owns_user_flag,
																										const char *name_s, const char *marti_id_s, double64 latitutde, double64 longitutde,
																										struct tm *start_p, struct tm *end_p);

/**
 * Free a given MartiEntry.
 *
 * @param marti_p The MartiEntry to free.
 * @ingroup MartiEntry
 */
MARTI_SERVICE_LOCAL void FreeMartiEntry (MartiEntry *marti_p);




MARTI_SERVICE_LOCAL MartiEntry *GetMartiEntryFromJSON (const json_t *json_p, const MartiServiceData *data_p);



#ifdef __cplusplus
}
#endif


#endif /* SERVICES_MARTI_INCLUDE_MARTI_ENTRY_H_ */
