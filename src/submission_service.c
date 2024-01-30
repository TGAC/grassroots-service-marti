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
 * submission_service.c
 *
 *  Created on: 22 Oct 2018
 *      Author: billy
 */

#include <string.h>

#include "submission_service.h"
#include "marti_service.h"

#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"
#include "schema_keys.h"

#include "string_parameter.h"
#include "time_parameter.h"


/*
 * Static declarations
 */

static const char *GetMartiSubmissionServiceName (const Service *service_p);

static const char *GetMartiSubmissionServiceDescription (const Service *service_p);

static const char *GetMartiSubmissionServiceAlias (const Service *service_p);

static const char *GetMartiSubmissionServiceInformationUri (const Service *service_p);

static ParameterSet *GetMartiSubmissionServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static void ReleaseMartiSubmissionServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunMartiSubmissionService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForMartiSubmissionService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseMartiSubmissionService (Service *service_p);

static ServiceMetadata *GetMartiSubmissionServiceMetadata (Service *service_p);

static bool GetMartiSubmissionServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);



/*
 * API definitions
 */


Service *GetMartiSubmissionService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			MartiServiceData *data_p = AllocateMartiServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
																 GetMartiSubmissionServiceName,
																 GetMartiSubmissionServiceDescription,
																 GetMartiSubmissionServiceAlias,
																 GetMartiSubmissionServiceInformationUri,
																 RunMartiSubmissionService,
																 IsResourceForMartiSubmissionService,
																 GetMartiSubmissionServiceParameters,
																 GetMartiSubmissionServiceParameterTypesForNamedParameters,
																 ReleaseMartiSubmissionServiceParameters,
																 CloseMartiSubmissionService,
																 NULL,
																 false,
																 SY_SYNCHRONOUS,
																 (ServiceData *) data_p,
																 GetMartiSubmissionServiceMetadata,
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

				}		/* if (data_p) */

			FreeService (service_p);
		}		/* if (service_p) */

	return NULL;
}



static const char *GetMartiSubmissionServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "MARTi submission service";
}


static const char *GetMartiSubmissionServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to submit MARTi data";
}


static const char *GetMartiSubmissionServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return GT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "submit";
}


static const char *GetMartiSubmissionServiceInformationUri (const Service * UNUSED_PARAM (service_p))
{
	return NULL;
}


static ParameterSet *GetMartiSubmissionServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = AllocateParameterSet ("MARTi submission service parameters", "The parameters used for the MARTi submission service");

	if (param_set_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, NULL, MA_NAME.npt_type, MA_NAME.npt_name_s, "Name", "The name of the location", NULL, PL_ALL)) != NULL)
				{
					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, NULL, MA_MARTI_ID.npt_type, MA_NAME.npt_name_s, "MARTi ID", "The ID for this sample within MARTi", NULL, PL_ALL)) != NULL)
						{
							if (AddCommonParameters (param_set_p, NULL, data_p))
								{
									if ((param_p = EasyCreateAndAddTimeParameterToParameterSet (data_p, param_set_p, NULL, MA_END_DATE.npt_name_s, "End Date", "The ending date, if different to the start date, of when this sample was taken", NULL, PL_ALL)) != NULL)
										{
											return param_set_p;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_END_DATE.npt_name_s);
										}
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_MARTI_ID.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_NAME.npt_name_s);
				}

			FreeParameterSet (param_set_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetMartiSubmissionServiceName (service_p));
		}

	return NULL;
}


static bool GetMartiSubmissionServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			MA_NAME,
			MA_MARTI_ID,
			MA_LATITUDE,
			MA_LONGITUDE,
			MA_START_DATE,
			MA_END_DATE,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


static void ReleaseMartiSubmissionServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseMartiSubmissionService (Service *service_p)
{
	bool success_flag = true;

	FreeMartiServiceData ((MartiServiceData *) (service_p -> se_data_p));;

	return success_flag;
}



static ServiceJobSet *RunMartiSubmissionService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	MartiServiceData *data_p = (MartiServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Marti");

	if (service_p -> se_jobs_p)
		{
			OperationStatus status = OS_FAILED_TO_START;
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			if (param_set_p)
				{


				}		/* if (param_set_p) */

			SetServiceJobStatus (job_p, status);
			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetMartiSubmissionServiceMetadata (Service *service_p)
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


static ParameterSet *IsResourceForMartiSubmissionService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}
