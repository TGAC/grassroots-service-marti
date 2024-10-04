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

#include "marti_submission_service.h"
#include "marti_service.h"

#include "audit.h"
#include "streams.h"
#include "math_utils.h"
#include "string_utils.h"
#include "schema_keys.h"

#include "string_parameter.h"
#include "double_parameter.h"
#include "time_parameter.h"
#include "string_array_parameter.h"

#include "marti_entry.h"



/*
 * Static declarations
 */

static const char * const S_EMPTY_LIST_OPTION_S = "<empty>";


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


static bool SetUpEntriesListParameter (const MartiServiceData *data_p, StringParameter *param_p, const MartiEntry *active_entry_p, const bool empty_option_flag);

static json_t *GetAllEntriesAsJSON (const MartiServiceData *data_p);

static MartiEntry *GetMartiEntryFromResource (DataResource *resource_p, MartiServiceData *data_p);


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
	return "https://www.earlham.ac.uk/marti";
}


static ParameterSet *GetMartiSubmissionServiceParameters (Service *service_p, DataResource *resource_p, User *user_p)
{
	ParameterSet *param_set_p = AllocateParameterSet ("MARTi submission service parameters", "The parameters used for the MARTi submission service");

	if (param_set_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			MartiServiceData *marti_data_p = (MartiServiceData *) data_p;
			Parameter *param_p = NULL;
			char *id_s = NULL;
			MartiEntry *active_entry_p = GetMartiEntryFromResource (resource_p, marti_data_p);
			ParameterGroup *main_group_p = CreateAndAddParameterGroupToParameterSet ("Data", false, data_p, param_set_p);

			if (active_entry_p)
				{
					id_s = GetBSONOidAsString (active_entry_p -> me_id_p);

					if (!id_s)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBSONOidAsString () failed for \"%s\"", active_entry_p -> me_sample_name_s);
						}
				}


			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, main_group_p, MA_ID.npt_type, MA_ID.npt_name_s, "Load Sample", "Edit an existing MARTi sample", id_s, PL_ALL)) != NULL)
				{
					if (SetUpEntriesListParameter (marti_data_p, (StringParameter *) param_p, active_entry_p, true))
						{
							/*
							 * We want to update all of the values in the form
							 * when a user selects a study from the list so
							 * we need to make the parameter automatically
							 * refresh the values. So we set the
							 * pa_refresh_service_flag to true.
							 */
							param_p -> pa_refresh_service_flag = true;

							if (id_s)
								{
									FreeBSONOidString (id_s);
									id_s = NULL;
								}

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, main_group_p, MA_NAME.npt_type, MA_NAME.npt_name_s, "Name", "The name of this sample", active_entry_p ? active_entry_p -> me_sample_name_s : NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_required_flag = true;

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, main_group_p, MA_MARTI_ID.npt_type, MA_MARTI_ID.npt_name_s, "MARTi ID", "The ID for this sample within MARTi", active_entry_p ? active_entry_p -> me_marti_id_s : NULL, PL_ALL)) != NULL)
										{
											param_p -> pa_required_flag = true;

											if (AddCommonMartiParameters (param_set_p, NULL, active_entry_p, data_p))
												{
													if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, main_group_p, MA_SITE_NAME.npt_type, MA_SITE_NAME.npt_name_s, "Site", "The name of the location where this sample was taken", active_entry_p ? active_entry_p -> me_site_name_s : NULL, PL_ALL)) != NULL)
														{
															if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, main_group_p, MA_DESCRIPTION.npt_type, MA_DESCRIPTION.npt_name_s, "Comments", "Any additional information about this sample", active_entry_p ? active_entry_p -> me_comments_s : NULL, PL_ALL)) != NULL)
																{
																	char **taxa_ss = NULL;
																	size_t num_taxa = 0;
																	ParameterGroup *taxa_group_p = CreateAndAddParameterGroupToParameterSet ("Taxonomy Ids", true, data_p, param_set_p);
																	const char * const display_name_s = "Taxonomy Identifiers";
																	const char * const description_s = "The taxa ids, and their forebears, for this sample";

																	if (active_entry_p)
																		{
																			taxa_ss = active_entry_p -> me_taxa_ss;
																			num_taxa = active_entry_p -> me_num_taxa;
																		}

																	if (num_taxa > 1)
																		{
																			param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, taxa_group_p, MA_TAXA.npt_name_s, display_name_s, description_s, taxa_ss, num_taxa, PL_ALL);
																		}
																	else
																		{
																			char *taxa_s = NULL;

																			if (num_taxa == 1)
																				{
																					taxa_s = *taxa_ss;
																				}

																			param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, taxa_group_p, PT_STRING, MA_TAXA.npt_name_s, display_name_s, description_s, taxa_s, PL_ALL);
																		}

																	if (param_p)
																		{
																			if (AddRepeatableParameterGroupLabelParam (taxa_group_p, param_p))
																				{
																					return param_set_p;
																				}
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddRepeatableParameterGroupLabelParam failed for %s", param_p -> pa_name_s);
																				}

																		}
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_TAXA.npt_name_s);
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_DESCRIPTION.npt_name_s);
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", MA_SITE_NAME.npt_name_s);
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


						}
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
	bool success_flag = false;
	const NamedParameterType params [] =
		{
			MA_ID,
			MA_NAME,
			MA_MARTI_ID,
			MA_SITE_NAME,
			MA_DESCRIPTION,
			MA_TAXA,
			NULL
		};

	success_flag = DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);

	if (!success_flag)
		{
			success_flag = GetCommonParameterTypesForNamedParameters (service_p, param_name_s, pt_p);
		}

	return success_flag;
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



static ServiceJobSet *RunMartiSubmissionService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable * UNUSED_PARAM (providers_p))
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
					bool success_flag = false;
					const char *name_s = NULL;
					const char *id_s = NULL;
					bson_oid_t *id_p = NULL;

					/*
					 * Get the existing entry id if specified
					 */
					GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_ID.npt_name_s, &id_s);

					if (id_s)
						{
							if (strcmp (S_EMPTY_LIST_OPTION_S, id_s) != 0)
								{
									id_p = GetBSONOidFromString (id_s);

									if (!id_p)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to load MARTi entry \"%s\" for editing", id_s);
											return false;
										}
								}
						}		/* if (id_value.st_string_value_s) */


					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_NAME.npt_name_s, &name_s))
						{
							if (!IsStringEmpty (name_s))
								{
									const char *marti_id_s = NULL;

									if (GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_MARTI_ID.npt_name_s, &marti_id_s))
										{
											if (!IsStringEmpty (marti_id_s))
												{
													const double64 *latitude_p = NULL;
													const double64 *longitude_p = NULL;
													const struct tm *start_p = NULL;

													if (GetCommonParameters (param_set_p, &latitude_p, &longitude_p, &start_p, name_s, job_p))
														{
															const char *site_name_s = NULL;

															bool owns_user_flag = false;
															PermissionsGroup *permissions_group_p = NULL;
															MartiEntry *entry_p = NULL;

															if (GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_SITE_NAME.npt_name_s, &site_name_s))
																{
																	const char *description_s = NULL;
																	const char **taxa_ss = NULL;
																	size_t num_taxa = 0;

																	GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_DESCRIPTION.npt_name_s, &description_s);


																	GetCurrentStringArrayParameterValuesFromParameterSet (param_set_p, MA_TAXA.npt_name_s, &taxa_ss, &num_taxa);

																	entry_p = AllocateMartiEntry (id_p, user_p, permissions_group_p, owns_user_flag,
																																name_s, marti_id_s, site_name_s, description_s, *latitude_p, *longitude_p,
																																start_p, taxa_ss, num_taxa);



																	if (entry_p)
																		{
																			status = SaveMartiEntry (entry_p, job_p, data_p);

																			FreeMartiEntry (entry_p);
																		}
																	else
																		{
																			success_flag = false;
																		}

																}


														}		/* if (GetCurrentTimeParameterValueFromParameterSet (param_set_p, MA_START_DATE.npt_name_s, &start_p)) */

												}		/* if (!IsStringEmpty (marti_id_s)) */
											else
												{
													AddParameterErrorMessageToServiceJob (job_p, MA_MARTI_ID.npt_name_s, MA_MARTI_ID.npt_type, "MARTi Id is a required field");
												}
										}		/* if (GetCurrentStringParameterValueFromParameterSet (param_set_p, MA_MARTI_ID.npt_name_s, &marti_id_s)) */
									else
										{
											AddParameterErrorMessageToServiceJob (job_p, MA_MARTI_ID.npt_name_s, MA_MARTI_ID.npt_type, "MARTi Id is a required field");
										}

								}		/* if (!IsStringEmpty (name_s)) */
							else
								{
									AddParameterErrorMessageToServiceJob (job_p, MA_NAME.npt_name_s, MA_NAME.npt_type, "Name is a required field");
								}

						}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, MA_NAME.npt_name_s, &name_s)) */
					else
						{
							AddParameterErrorMessageToServiceJob (job_p, MA_NAME.npt_name_s, MA_NAME.npt_type, "Name is a required field");
						}

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



static json_t *GetAllEntriesAsJSON (const MartiServiceData *data_p)
{
	json_t *results_p = NULL;

	if (SetMongoToolDatabaseAndCollection (data_p -> msd_mongo_p, data_p -> msd_database_s, data_p -> msd_collection_s))
		{
			bson_t *query_p = NULL;
			bson_t *opts_p =  BCON_NEW ( "sort", "{", ME_NAME_S, BCON_INT32 (1), "}");

			results_p = GetAllMongoResultsAsJSON (data_p -> msd_mongo_p, query_p, opts_p);

			if (opts_p)
				{
					bson_destroy (opts_p);
				}

		}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_PROGRAMME])) */

	return results_p;
}



static bool SetUpEntriesListParameter (const MartiServiceData *data_p, StringParameter *param_p, const MartiEntry *active_entry_p, const bool empty_option_flag)
{
	bool success_flag = false;
	json_t *results_p = GetAllEntriesAsJSON (data_p);
	bool value_set_flag = false;

	if (results_p)
		{
			if (json_is_array (results_p))
				{
					const size_t num_results = json_array_size (results_p);

					success_flag = true;

					/*
					 * If there's an empty option, add it
					 */
					if (empty_option_flag)
						{
							success_flag = CreateAndAddStringParameterOption (& (param_p -> sp_base_param), S_EMPTY_LIST_OPTION_S, S_EMPTY_LIST_OPTION_S);
						}

					if (success_flag)
						{
							if (num_results > 0)
								{
									size_t i = 0;
									const char *param_value_s = GetStringParameterCurrentValue (param_p);

									bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

									if (id_p)
										{
											while ((i < num_results) && success_flag)
												{
													json_t *entry_p = json_array_get (results_p, i);

													if (GetMongoIdFromJSON (entry_p, id_p))
														{
															char *id_s = GetBSONOidAsString (id_p);

															if (id_s)
																{
																	const char *name_s = GetJSONString (entry_p, ME_NAME_S);

																	if (name_s)
																		{
																			if (param_value_s && (strcmp (param_value_s, id_s) == 0))
																				{
																					value_set_flag = true;
																				}

																			if (!CreateAndAddStringParameterOption (& (param_p -> sp_base_param), id_s, name_s))
																				{
																					success_flag = false;
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param option \"%s\": \"%s\"", id_s, name_s);
																				}

																		}		/* if (name_s) */
																	else
																		{
																			success_flag = false;
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get \"%s\"", ME_NAME_S);
																		}

																	FreeBSONOidString (id_s);
																}
															else
																{
																	success_flag = false;
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get Programme BSON oid");
																}

														}		/* if (GetMongoIdFromJSON (entry_p, id_p)) */
													else
														{
															success_flag = false;
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "GetMongoIdFromJSON () failed");
														}

													if (success_flag)
														{
															++ i;
														}

												}		/* while ((i < num_results) && success_flag) */

											FreeBSONOid (id_p);
										}		/* if (id_p) */

									/*
									 * If the parameter's value isn't on the list, reset it
									 */
									if ((param_value_s != NULL) && (strcmp (param_value_s, S_EMPTY_LIST_OPTION_S) != 0) && (value_set_flag == false))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing programmes", param_value_s);
										}

								}		/* if (num_results > 0) */
							else
								{
									/* nothing to add */
									success_flag = true;
								}

						}		/* if (success_flag) */


				}		/* if (json_is_array (results_p)) */

			json_decref (results_p);
		}		/* if (results_p) */

	if (success_flag)
		{
			if (active_entry_p)
				{
					char *id_s = GetBSONOidAsString (active_entry_p -> me_id_p);

					if (id_s)
						{
							success_flag = SetStringParameterDefaultValue (param_p, id_s);
							FreeBSONOidString (id_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id string for active program \"%s\"", active_entry_p -> me_sample_name_s);
							success_flag = false;
						}
				}
		}

	return success_flag;
}


static MartiEntry *GetMartiEntryFromResource (DataResource *resource_p, MartiServiceData *data_p)
{
	MartiEntry *marti_p = NULL;

	/*
	 * Have we been set some parameter values to refresh from?
	 */
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					json_t *params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);

					if (params_json_p)
						{
							const char *id_s =  NULL;
							const size_t num_entries = json_array_size (params_json_p);
							size_t i;

							for (i = 0; i < num_entries; ++ i)
								{
									const json_t *param_json_p = json_array_get (params_json_p, i);
									const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);

									if (name_s)
										{
											if (strcmp (name_s, MA_ID.npt_name_s) == 0)
												{
													id_s = GetJSONString (param_json_p, PARAM_CURRENT_VALUE_S);

													if (!id_s)
														{
															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_json_p, "Failed to get \"%s\" from \"%s\"", PARAM_CURRENT_VALUE_S, MA_ID.npt_name_s);
														}

													/* force exit from loop */
													i = num_entries;
												}
										}		/* if (name_s) */

								}		/* if (params_json_p) */

							/*
							 * Do we have an existing study id?
							 */
							if (id_s)
								{
									marti_p = GetMartiEntryByMongoIdString (id_s, data_p);

									if (!marti_p)
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, params_json_p, "Failed to load MartiEntry with mongo id \"%s\"", id_s);
										}

								}		/* if (study_id_s) */

						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_set_json_p, "Failed to get params with key \"%s\"", PARAM_SET_PARAMS_S);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, resource_p -> re_data_p, "Failed to get param set with key \"%s\"", PARAM_SET_KEY_S);
				}

		}		/* if (resource_p && (resource_p -> re_data_p)) */

	return marti_p;
}

