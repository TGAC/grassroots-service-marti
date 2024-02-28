/*
 * parental_genotype_service.h
 *
 *  Created on: 13 Jul 2018
 *      Author: billy
 */

#ifndef SERVICES_MARTI_SERVICE_INCLUDE_MARTI_SERVICE_H_
#define SERVICES_MARTI_SERVICE_INCLUDE_MARTI_SERVICE_H_


#include "marti_service_library.h"
#include "service.h"
#include "schema_keys.h"
#include "marti_entry.h"

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Get the Service available for running the DFW Field Trial Service.
 *
 * @param user_p The User for the user trying to access the services.
 * This can be <code>NULL</code>.
 * @return The ServicesArray containing the DFW Field Trial Service. or
 * <code>NULL</code> upon error.
 *
 */
MARTI_SERVICE_API ServicesArray *GetServices (User *user_p, GrassrootsServer *grassroots_p);


/**
 * Free the ServicesArray and its associated DFW Field Trial Service.
 *
 * @param services_p The ServicesArray to free.
 *
 */
MARTI_SERVICE_API void ReleaseServices (ServicesArray *services_p);



MARTI_SERVICE_LOCAL bool AddErrorMessage (ServiceJob *job_p, const json_t *value_p, const char *error_s, const int index);


MARTI_SERVICE_LOCAL MartiEntry *GetMartiEntryByMartiIdString (const char * const marti_id_s, const MartiServiceData *data_p);


MARTI_SERVICE_LOCAL MartiEntry *GetMartiEntryByMongoIdString (const char * const mongo_id_s, const MartiServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_MARTI_SERVICE_INCLUDE_MARTI_SERVICE_H_ */
