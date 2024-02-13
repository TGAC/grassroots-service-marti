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

