/*
** Copyright 2014-2018 The Earlham Institute
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
/*
 * parental_genotype_service_service_data.c
 *
 *  Created on: 18 Nov 2018
 *      Author: billy
 */

#include "marti_service_data.h"

#include "streams.h"

#include "double_parameter.h"
#include "time_parameter.h"


MartiServiceData *AllocateMartiServiceData  (void)
{
	MartiServiceData *data_p = (MartiServiceData *) AllocMemory (sizeof (MartiServiceData));

	if (data_p)
		{
			data_p -> msd_mongo_p = NULL;
			data_p -> msd_database_s = NULL;
			data_p -> msd_collection_s = NULL;
			data_p -> msd_api_url_s = NULL;

			return data_p;
		}

	return NULL;
}


void FreeMartiServiceData (MartiServiceData *data_p)
{
	if (data_p -> msd_mongo_p)
		{
			FreeMongoTool (data_p -> msd_mongo_p);
		}

	FreeMemory (data_p);
}


bool ConfigureMartiService (MartiServiceData *data_p, GrassrootsServer *grassroots_p)
{
	bool success_flag = false;
	const json_t *service_config_p = data_p -> msd_base_data.sd_config_p;

	data_p -> msd_database_s = GetJSONString (service_config_p, "database");

	if (data_p -> msd_database_s)
		{
			if ((data_p -> msd_collection_s = GetJSONString (service_config_p, "collection")) != NULL)
				{
					if ((data_p -> msd_mongo_p = AllocateMongoTool (NULL, grassroots_p -> gs_mongo_manager_p)) != NULL)
						{
							if (SetMongoToolDatabaseAndCollection (data_p -> msd_mongo_p, data_p -> msd_database_s, data_p -> msd_collection_s))
								{
									data_p -> msd_api_url_s = GetJSONString (service_config_p, "marti_url");

									if (! (data_p -> msd_api_url_s))
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, service_config_p, "No MARTi API URL specified");
										}

									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set database to \"%s\" and collection to \"%s\"", data_p -> msd_database_s, data_p -> msd_collection_s);
								}

						}		/* if ((data_p -> msd_mongo_p = AllocateMongoTool (NULL, grassroots_p -> gs_mongo_manager_p)) != NULL) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate MongoTool");
						}

				} 	/* if ((data_p -> msd_collection_s = GetJSONString (service_config_p, "collection")) != NULL) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, service_config_p, "No collection specified");
				}

		}		/* if (data_p -> psd_database_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, service_config_p, "No database specified");
		}

	return success_flag;
}



bool AddCommonParameters (ParameterSet *param_set_p, ParameterGroup *param_group_p, ServiceData *data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;

	if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, param_set_p, param_group_p, MA_LATITUDE.npt_type, MA_LATITUDE.npt_name_s, "Latitude", "The latitude of this location", NULL, PL_ALL)) != NULL)
		{
			param_p -> pa_required_flag = true;

			if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (data_p, param_set_p, param_group_p, MA_LONGITUDE.npt_type, MA_LONGITUDE.npt_name_s, "Longitude", "The longitude of this location", NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_required_flag = true;

					if ((param_p = EasyCreateAndAddTimeParameterToParameterSet (data_p, param_set_p, param_group_p, MA_START_DATE.npt_name_s, "Start Date", "The starting date of when this sample was taken", NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_required_flag = true;

							success_flag = true;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_START_DATE.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_LONGITUDE.npt_name_s);
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_LATITUDE.npt_name_s);
		}

	return success_flag;
}


bool GetCommonParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			MA_LATITUDE,
			MA_LONGITUDE,
			MA_START_DATE,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


bool GetCommonParameters (ParameterSet *param_set_p, const double64 **latitude_pp, const double64 **longitude_pp, const struct tm **start_pp, const char * const name_s, ServiceJob *job_p)
{
	const double64 *latitude_p = NULL;

	if (GetCurrentDoubleParameterValueFromParameterSet (param_set_p, MA_LATITUDE.npt_name_s, &latitude_p))
		{
			const double64 *longitude_p = NULL;

			if (GetCurrentDoubleParameterValueFromParameterSet (param_set_p, MA_LONGITUDE.npt_name_s, &longitude_p))
				{
					const struct tm *start_p = NULL;

					if (GetCurrentTimeParameterValueFromParameterSet (param_set_p, MA_START_DATE.npt_name_s, &start_p))
						{
							*latitude_pp = latitude_p;
							*longitude_pp = longitude_p;
							*start_pp = start_p;

							return true;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No start date for %s", name_s);
							AddParameterErrorMessageToServiceJob (job_p, MA_START_DATE.npt_name_s, MA_START_DATE.npt_type, "Value required");
						}

				}		/* if (GetCurrentDoubleParameterValueFromParameterSet (param_set_p, MA_LONGITUDE.npt_name_s, &longitude_p)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No Longitude for %s", name_s);
					AddParameterErrorMessageToServiceJob (job_p, MA_LONGITUDE.npt_name_s, MA_LONGITUDE.npt_type, "Value required");
				}

		}		/* if (GetCurrentDoubleParameterValueFromParameterSet (param_set_p, MA_LATITUDE.npt_name_s, &latitude_p)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No Latitude for %s", name_s);
			AddParameterErrorMessageToServiceJob (job_p, MA_LATITUDE.npt_name_s, MA_LATITUDE.npt_type, "Value required");
		}


	return false;
}
