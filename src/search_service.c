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
 * search_service.c
 *
 *  Created on: 24 Oct 2018
 *      Author: billy
 */

#include "search_service.h"
#include "marti_service.h"
#include "marti_entry.h"

#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"
#include "time_util.h"

#include "string_parameter.h"
#include "boolean_parameter.h"
#include "time_parameter.h"
#include "unsigned_int_parameter.h"


/*
 * Static declarations
 */

static NamedParameterType S_MAX_DISTANCE = { "Maximum Distance", PT_UNSIGNED_INT };



static const char *GetMartiSearchServiceName (const Service *service_p);

static const char *GetMartiSearchServiceDescription (const Service *service_p);

static const char *GetMartiSearchServiceAlias (const Service *service_p);

static const char *GetMartiSearchServiceInformationUri (const Service *service_p);

static ParameterSet *GetMartiSearchServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetMartiSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseMartiSearchServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunMartiSearchService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForMartiSearchService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseMartiSearchService (Service *service_p);

static ServiceMetadata *GetMartiSearchServiceMetadata (Service *service_p);

static json_t *GetSearchQuery (const double64 latitude, const double64 longitude, const struct tm *start_date_p, const struct tm *end_date_p, const uint32 min_distance, const uint32 max_distance);

static bool AddNumberToArray (json_t *array_p, const double64 value);

static bool AddNonTrivialTimeToQuery (json_t *query_p, const struct tm *time_p, const char * const field_s, const char * const op_s);


/*
 * API definitions
 */


Service *GetMartiSearchService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			MartiServiceData *data_p = AllocateMartiServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
																 GetMartiSearchServiceName,
																 GetMartiSearchServiceDescription,
																 GetMartiSearchServiceAlias,
																 GetMartiSearchServiceInformationUri,
																 RunMartiSearchService,
																 IsResourceForMartiSearchService,
																 GetMartiSearchServiceParameters,
																 GetMartiSearchServiceParameterTypesForNamedParameters,
																 ReleaseMartiSearchServiceParameters,
																 CloseMartiSearchService,
																 NULL,
																 false,
																 SY_SYNCHRONOUS,
																 (ServiceData *) data_p,
																 GetMartiSearchServiceMetadata,
																 NULL,
																 grassroots_p))
						{
							if (ConfigureMartiService (data_p, grassroots_p))
								{
									return service_p;
								}

						}		/* if (InitialiseService (.... */
					else
						{
							FreeMartiServiceData (data_p);
						}
				}

			if (service_p)
				{
					FreeService (service_p);
				}

		}		/* if (service_p) */

	return NULL;
}



static const char *GetMartiSearchServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "MARTi search service";
}


static const char *GetMartiSearchServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to get the parental data for given markers and populations";
}


static const char *GetMartiSearchServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return GT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "search";
}

static const char *GetMartiSearchServiceInformationUri (const Service * UNUSED_PARAM (service_p))
{
	return NULL;
}


static ParameterSet *GetMartiSearchServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("MARTi search service parameters", "The parameters used for the MARTi search service");

	if (param_set_p)
		{
			ServiceData *data_p = service_p -> se_data_p;

			if (AddCommonParameters (param_set_p, NULL, data_p))
				{
					Parameter *param_p  = EasyCreateAndAddTimeParameterToParameterSet (data_p, param_set_p, NULL, MA_END_DATE.npt_name_s, "End Date", "The ending date of when this sample was taken", NULL, PL_ALL);

					if (param_p)
						{
							if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, NULL, S_MAX_DISTANCE.npt_name_s, "Radius", "The maximum distance to find matching locations for", NULL, PL_ALL)) != NULL)
								{
									return param_set_p;
								}

						}

				}

			FreeParameterSet (param_set_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetMartiSearchServiceName (service_p));
		}

	return NULL;
}


static bool GetMartiSearchServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;
	const NamedParameterType params [] =
		{
			S_MAX_DISTANCE,
			NULL
		};

	success_flag = DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);

	if (!success_flag)
		{
			success_flag = GetCommonParameterTypesForNamedParameters (service_p, param_name_s, pt_p);
		}

	return success_flag;
}



static void ReleaseMartiSearchServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}


static bool CloseMartiSearchService (Service *service_p)
{
	bool success_flag = true;

	FreeMartiServiceData ((MartiServiceData *) (service_p -> se_data_p));;

	return success_flag;
}


static ServiceJobSet *RunMartiSearchService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	MartiServiceData *data_p = (MartiServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Marti");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);
			OperationStatus status = OS_FAILED_TO_START;

			LogParameterSet (param_set_p, job_p);


			if (param_set_p)
				{
					const double64 *latitude_p = NULL;
					const double64 *longitude_p = NULL;
					const struct tm *start_p = NULL;

					if (GetCommonParameters (param_set_p, &latitude_p, &longitude_p, &start_p, "search", job_p))
						{
							uint32 min_distance = 0;
							uint32 max_distance = 1000;
							const uint32 *max_distance_p = NULL;
							const struct tm *start_date_p = NULL;
							const struct tm *end_date_p = NULL;

							GetCurrentUnsignedIntParameterValueFromParameterSet (param_set_p, S_MAX_DISTANCE.npt_name_s, &max_distance_p);

							if (max_distance_p)
								{
									max_distance = *max_distance_p;
								}

							GetCurrentTimeParameterValueFromParameterSet (param_set_p, MA_START_DATE.npt_name_s, &start_date_p);
							GetCurrentTimeParameterValueFromParameterSet (param_set_p, MA_END_DATE.npt_name_s, &end_date_p);

							json_t *query_p = GetSearchQuery (*latitude_p, *longitude_p, start_date_p, end_date_p, min_distance, max_distance);

							if (query_p)
								{
									bson_t *bson_query_p = ConvertJSONToBSON (query_p);

									if (bson_query_p)
										{
											if (FindMatchingMongoDocumentsByBSON (data_p -> msd_mongo_p, bson_query_p, NULL, NULL))
												{
													json_t *results_p = GetAllExistingMongoResultsAsJSON (data_p -> msd_mongo_p);

													if (results_p)
														{
															json_t *result_p;
															size_t i;
															size_t num_successes = 0;
															const size_t num_results  = json_array_size (results_p);

															json_array_foreach (results_p, i, result_p)
																{
																	MartiEntry *marti_p = GetMartiEntryFromJSON (result_p, data_p);

																	if (marti_p)
																		{
																			json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, marti_p -> me_name_s, result_p);

																			if (dest_record_p)
																				{
																					if (AddResultToServiceJob (job_p, dest_record_p))
																						{
																							++ num_successes;
																						}
																					else
																						{
																							json_decref (dest_record_p);
																						}
																				}

																			FreeMartiEntry (marti_p);
																		}


																}		/* json_array_foreach (results_p, i, result_p) */

															if (num_successes == num_results)
																{
																	status = OS_SUCCEEDED;
																}
															else if (num_successes > 0)
																{
																	status = OS_PARTIALLY_SUCCEEDED;
																}

															json_decref (results_p);
														}		/* if (results_p) */

												}		/* if (FindMatchingMongoDocumentsByJSON (data_p -> msd_mongo_p, query_p, NULL, NULL)) */
											else
												{
													status = OS_FAILED;
												}

											bson_free (bson_query_p);
										}		/* if (bson_query_p) */


									json_decref (query_p);
								}

						}		/* if (GetCommonParameters (param_set_p, &latitude_p, &longitude_p, &start_p, "search", job_p)) */

				}		/* if (param_set_p) */

			SetServiceJobStatus (job_p, status);

			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetMartiSearchServiceMetadata (Service * UNUSED_PARAM (service_p))
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_0625";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Genotype and phenotype",
																							 "The study of genetic constitution of a living entity, such as an individual, and organism, a cell and so on, "
																							 "typically with respect to a particular observable phenotypic traits, or resources concerning such traits, which "
																							 "might be an aspect of biochemistry, physiology, morphology, anatomy, development and so on.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0304";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Query and retrieval", "Search or query a data resource and retrieve entries and / or annotation.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_0968";
							input_p = AllocateSchemaTerm (term_url_s, "Keyword",
																						"Boolean operators (AND, OR and NOT) and wildcard characters may be allowed. Keyword(s) or phrase(s) used (typically) for text-searching purposes.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											SchemaTerm *output_p;


											/* Genotype */
											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
											output_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
																										 "(e.g. a cell) and the information about the genetic content (genotype).");

											if (output_p)
												{
													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
														{
															return metadata_p;
														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (output_p);
														}

												}		/* if (output_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
												}

										}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
											FreeSchemaTerm (input_p);
										}

								}		/* if (input_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
								}

							FreeServiceMetadata (metadata_p);
						}		/* if (metadata_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service metadata");
						}

				}		/* if (subcategory_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate sub-category term %s for service metadata", term_url_s);
				}

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}


static ParameterSet *IsResourceForMartiSearchService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


/*
 {
		$nearSphere: {
			 $geometry: {
					type : "Point",
					coordinates : [ <longitude>, <latitude> ]
			 },
			 $minDistance: <distance in meters>,
			 $maxDistance: <distance in meters>
		}
	}
 */
static json_t *GetSearchQuery (const double64 latitude, const double64 longitude, const struct tm *start_date_p, const struct tm *end_date_p, const uint32 min_distance, const uint32 max_distance)
{
	json_t *root_p = json_object ();

	if (root_p)
		{
			json_t *query_p = json_object ();

			if (query_p)
				{
					if (json_object_set_new (root_p, ME_LOCATION_S, query_p) == 0)
						{
							json_t *near_sphere_p = json_object ();

							if (near_sphere_p)
								{
									if (json_object_set_new (query_p, "$nearSphere", near_sphere_p) == 0)
										{
											json_t *geometry_p = json_object ();

											if (geometry_p)
												{
													if (json_object_set_new (near_sphere_p, "$geometry", geometry_p) == 0)
														{
															if (SetJSONString (geometry_p, "type", "Point"))
																{
																	json_t *coords_p = json_array ();

																	if (coords_p)
																		{
																			if (json_object_set_new (geometry_p, "coordinates", coords_p) == 0)
																				{
																					if (AddNumberToArray (coords_p, longitude))
																						{
																							if (AddNumberToArray (coords_p, latitude))
																								{
																									if ((min_distance == 0) || (SetJSONInteger (near_sphere_p, "$minDistance", min_distance)))
																										{
																											if ((max_distance == 0) || (SetJSONInteger (near_sphere_p, "$maxDistance", max_distance)))
																												{
																													if (AddNonTrivialTimeToQuery (query_p, start_date_p, ME_END_DATE_S, "$lte"))
																														{
																															if (AddNonTrivialTimeToQuery (query_p, end_date_p, ME_START_DATE_S, "$gte"))
																																{
																																	return root_p;
																																}

																														}
																												}

																										}
																								}

																						}

																				}
																			else
																				{
																					json_decref (coords_p);
																				}

																		}


																}		/* if (SetJSONString (geometry_p, "type", "Point")) */
														}
													else
														{
															json_decref (geometry_p);
														}
												}

										}
									else
										{
											json_decref (near_sphere_p);
										}
								}

						}
					else
						{
							json_decref (query_p);
						}

				}		/* if (query_p) */

			json_decref (root_p);
		}


	return NULL;
}


static bool AddNumberToArray (json_t *array_p, const double64 value)
{
	bool success_flag = false;
	json_t *number_p = json_real (value);

	if (number_p)
		{
			if (json_array_append_new (array_p, number_p) == 0)
				{
					success_flag = true;
				}
			else
				{
					json_decref (number_p);
				}
		}

	return success_flag;
}


static bool AddNonTrivialTimeToQuery (json_t *query_p, const struct tm *time_p, const char * const field_s, const char * const op_s)
{
	bool success_flag = false;

	if (time_p)
		{
			char *time_s = GetTimeAsString (time_p, false, NULL);

			if (time_s)
				{
					json_t *op_p = json_object ();

					if (op_p)
						{
							if (SetJSONString (op_p, op_s, time_s))
								{

									if (json_object_set_new (query_p, field_s, op_p) == 0)
										{
											success_flag = true;
										}
									else
										{

										}

								}

							if (!success_flag)
								{
									json_decref (op_p);
								}
						}

					FreeTimeString (time_s);
				}

		}		/* if (time_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}
