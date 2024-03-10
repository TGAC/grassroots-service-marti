/*
 ** Copyright 2014-2016 The Earlham Institute
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#include <string.h>

#include "jansson.h"

#define ALLOCATE_MARTI_SERVICE_TAGS (1)
#include "marti_service.h"
#include "marti_service_data.h"

#include "marti_entry.h"

#include "memory_allocations.h"
#include "parameter.h"
#include "service_job.h"
#include "string_utils.h"
#include "json_tools.h"
#include "grassroots_server.h"
#include "string_linked_list.h"
#include "math_utils.h"
#include "search_options.h"
#include "time_util.h"
#include "io_utils.h"
#include "audit.h"

#include "search_service.h"
#include "submission_service.h"


#ifdef _DEBUG
#define MARTI_SERVICE_DEBUG	(STM_LEVEL_FINER)
#else
#define MARTI_SERVICE_DEBUG	(STM_LEVEL_NONE)
#endif



/*
 * STATIC PROTOTYPES
 */

static MartiEntry *GetMartiEntryByQuery (bson_t *query_p, const MartiServiceData *data_p);


/*
 * API FUNCTIONS
 */


ServicesArray *GetServices (User *user_p, GrassrootsServer *grassroots_p)
{
	uint32 num_services = 0;
	Service *submission_service_p = GetMartiSubmissionService (grassroots_p);
	Service *search_service_p = GetMartiSearchService (grassroots_p);

	if (submission_service_p)
		{
			++ num_services;
		}

	if (search_service_p)
		{
			++ num_services;
		}


	if (num_services)
		{
			ServicesArray *services_p = AllocateServicesArray (num_services);

			if (services_p)
				{
					num_services = 0;

					if (submission_service_p)
						{
							* (services_p -> sa_services_pp) = submission_service_p;
							++ num_services;
						}


					if (search_service_p)
						{
							* ((services_p -> sa_services_pp) + num_services) = search_service_p;
						}

					MartiServiceData *data_p =  (MartiServiceData *) ((* (services_p -> sa_services_pp)) -> se_data_p);

					if (AddCollectionSingleIndex (data_p -> msd_mongo_p, data_p -> msd_database_s, data_p -> msd_collection_s, ME_LOCATION_S, "2dsphere", false, false))
						{
							return services_p;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add index for db \"%s\" collection \"%s\" field \"%s\"", data_p -> msd_database_s, data_p -> msd_collection_s, ME_LOCATION_S);
							FreeServicesArray (services_p);
							return NULL;
						}

				}
		}


	if (submission_service_p)
		{
			FreeService (submission_service_p);
		}

	if (search_service_p)
		{
			FreeService (submission_service_p);
		}


	return NULL;
}


void ReleaseServices (ServicesArray *services_p)
{
	FreeServicesArray (services_p);
}


MartiEntry *GetMartiEntryByMartiIdString (const char * const marti_id_s, const MartiServiceData *data_p)
{
	MartiEntry *marti_p = NULL;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			if (BSON_APPEND_UTF8 (query_p, ME_MARTI_ID_S, marti_id_s))
				{
					marti_p = GetMartiEntryByQuery (query_p, data_p);
				}		/* if (BSON_APPEND_UTF8 (query_p, ME_MARTI_ID_S, marti_id_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to populate query for marti id \"%s\"", marti_id_s);
				}

			bson_destroy (query_p);
		}		/* if (query_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for marti id \"%s\"", marti_id_s);
		}

	return marti_p;
}


MartiEntry *GetMartiEntryByMongoIdString (const char * const id_s, const MartiServiceData *data_p)
{
	MartiEntry *marti_p = NULL;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			if (bson_oid_is_valid (id_s, strlen (id_s)))
				{
					bson_oid_t oid;
					bson_oid_init_from_string (&oid, id_s);

					if (BSON_APPEND_OID (query_p, MONGO_ID_S, &oid))
						{
							marti_p = GetMartiEntryByQuery (query_p, data_p);
						}		/* if (BSON_APPEND_UTF8 (query_p, MONGO_ID_S, marti_id_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to populate query for id \"%s\"", id_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "\"%s\" is not valid oid", id_s);
				}

			bson_destroy (query_p);
		}		/* if (query_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for mongo id \"%s\"", id_s);
		}

	return marti_p;
}



static MartiEntry *GetMartiEntryByQuery (bson_t *query_p, const MartiServiceData *data_p)
{
	MartiEntry *marti_p = NULL;
	MongoTool *tool_p = data_p -> msd_mongo_p;
	json_t *results_p = GetAllMongoResultsAsJSON (tool_p, query_p, NULL);

	if (results_p)
		{
			if (json_is_array (results_p))
				{
					size_t num_results = json_array_size (results_p);

					if (num_results == 1)
						{
							json_t *res_p = json_array_get (results_p, 0);

							marti_p = GetMartiEntryFromJSON (res_p, data_p);

							if (!marti_p)
								{
									PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "Failed to create MartiEntry searching with query");
								}

						}		/* if (num_results == 1) */
					else
						{
							char *query_s = bson_as_json (query_p, NULL);

							if (query_s)
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, SIZET_FMT " results when searching with \"%s\"", num_results, query_s);
									bson_free (query_s);
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, SIZET_FMT " results when searching", num_results);
								}

						}

				}		/* if (json_is_array (results_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "Results are not an array");
				}

			json_decref (results_p);
		}		/* if (results_p) */
	else
		{
			PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "Failed to get results searching with query");
		}


	return marti_p;
}
