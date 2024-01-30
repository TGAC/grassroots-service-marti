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


#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"

#include "string_parameter.h"
#include "boolean_parameter.h"

/*
 * Static declarations
 */


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
	return "Marti search service";
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
					return param_set_p;
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
	bool success_flag = true;

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

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (param_set_p)
				{

				}		/* if (param_set_p) */


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


