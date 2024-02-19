/*
 * marti_entry.c
 *
 *  Created on: 31 Jan 2024
 *      Author: billy
 */

#define ALLOCATE_MARTI_ENTRY_TAGS (1)
#include "marti_entry.h"
#include "memory_allocations.h"
#include "json_util.h"
#include "mongodb_util.h"
#include "lucene_tool.h"

#include "string_utils.h"
#include "time_util.h"


static bool AddRealToJSONArray (json_t *array_p, const double64 value);

static bool AddNonTrivialDateToJSON (json_t *json_p, const char * const key_s, const struct tm *date_p);

static bool GetNonTrivialDateFromJSON (json_t *json_p, const char * const key_s, const struct tm **time_pp);


MartiEntry *AllocateMartiEntry (bson_oid_t *id_p, User *user_p, PermissionsGroup *permissions_group_p, const bool owns_user_flag,
																const char *sample_name_s, const char *marti_id_s, const char *site_name_s,
																const char *description_s, double64 latitude, double64 longitude, const struct tm *time_p)
{
	if (marti_id_s)
		{
			char *copied_marti_id_s = EasyCopyToNewString (marti_id_s);

			if (copied_marti_id_s)
				{
					if (time_p)
						{
							struct tm *copied_start_p = DuplicateTime (time_p);

							if (copied_start_p)
								{
									char *copied_sample_name_s = NULL;

									if ((sample_name_s == NULL) || (copied_sample_name_s = EasyCopyToNewString (sample_name_s)))
										{
											char *copied_site_name_s = NULL;

											if ((site_name_s == NULL) || (copied_site_name_s = EasyCopyToNewString (site_name_s)))
												{
													char *copied_description_s = NULL;

													if ((description_s == NULL) || (copied_description_s = EasyCopyToNewString (description_s)))
														{
															bool alloc_perms_flag = false;

															if (!permissions_group_p)
																{
																	permissions_group_p = AllocatePermissionsGroup ();

																	if (permissions_group_p)
																		{
																			alloc_perms_flag = true;
																		}
																}


															if (permissions_group_p)
																{
																	MartiEntry *entry_p = (MartiEntry *) AllocMemory (sizeof (MartiEntry));

																	if (entry_p)
																		{
																			entry_p -> me_id_p = id_p;
																			entry_p -> me_user_p = user_p;
																			entry_p -> me_permissions_group_p = permissions_group_p;
																			entry_p -> me_owns_user_flag = owns_user_flag;
																			entry_p -> me_sample_name_s = copied_sample_name_s;
																			entry_p -> me_marti_id_s = copied_marti_id_s;
																			entry_p -> me_latitude = latitude;
																			entry_p -> me_longitude = longitude;
																			entry_p -> me_time_p = copied_start_p;
																			entry_p -> me_site_name_s = copied_site_name_s;
																			entry_p -> me_comments_s = copied_description_s;

																			return entry_p;
																		}


																	if (alloc_perms_flag)
																		{
																			FreePermissionsGroup (permissions_group_p);
																		}

																}

															if (copied_description_s)
																{
																	FreeCopiedString (copied_description_s);
																}
														}

													if (copied_site_name_s)
														{
															FreeCopiedString (copied_site_name_s);
														}

												}		/* if ((site_name_s == NULL) || (copied_site_name_s = EasyCopyToNewString (sample_name_s))) */

											if (copied_sample_name_s)
												{
													FreeCopiedString (copied_sample_name_s);
												}

										}		/* if ((name_s == NULL) || (copied_name_s = EasyCopyToNewString (name_s))) */

									FreeTime (copied_start_p);
								}		/* if (copied_start_p) */

						}		/* if (start_p) */

					FreeCopiedString (copied_marti_id_s);
				}		/* if (copied_marti_id_s) */

		}		/* if (marti_id_s) */


	return NULL;
}


void FreeMartiEntry (MartiEntry *marti_p)
{
	if (marti_p -> me_id_p)
		{
			FreeBSONOid (marti_p -> me_id_p);
		}

	if (marti_p -> me_permissions_group_p)
		{
			FreePermissionsGroup (marti_p -> me_permissions_group_p);
		}

	if ((marti_p -> me_user_p) && (marti_p -> me_owns_user_flag))
		{
			FreeUser (marti_p -> me_user_p);
		}

	if (marti_p -> me_sample_name_s)
		{
			FreeCopiedString (marti_p -> me_sample_name_s);
		}

	if (marti_p -> me_site_name_s)
		{
			FreeCopiedString (marti_p -> me_site_name_s);
		}

	if (marti_p -> me_comments_s)
		{
			FreeCopiedString (marti_p -> me_comments_s);
		}


	if (marti_p -> me_marti_id_s)
		{
			FreeCopiedString (marti_p -> me_marti_id_s);
		}

	if (marti_p -> me_time_p)
		{
			FreeTime (marti_p -> me_time_p);
		}

	FreeMemory (marti_p);
}


json_t *GetMartiEntryAsJSON (const MartiEntry *me_p, MartiServiceData *data_p)
{
	json_t *marti_json_p = json_object ();

	if (marti_json_p)
		{
			if (AddCompoundIdToJSON (marti_json_p, me_p -> me_id_p))
				{
					if (SetJSONString (marti_json_p, ME_NAME_S, me_p -> me_sample_name_s))
						{
							if (SetJSONString (marti_json_p, ME_MARTI_ID_S, me_p -> me_marti_id_s))
								{
									if (SetNonTrivialString (marti_json_p, ME_SITE_NAME_S, me_p -> me_site_name_s, true))
										{
											if (SetNonTrivialString (marti_json_p, ME_DESCRIPTION_S, me_p -> me_comments_s, true))
												{
													/*
													 * We're storing the location as a GeoJSON Point to allow for
													 * geosppatial queries. See https://www.mongodb.com/docs/manual/geospatial-queries/
													 *
													 */
													json_t *location_p = json_object ();

													if (location_p)
														{
															if (json_object_set_new (marti_json_p, ME_LOCATION_S, location_p) == 0)
																{
																	if (SetJSONString (location_p, "type", "Point"))
																		{
																			json_t *coords_p = json_array ();

																			if (coords_p)
																				{
																					if (json_object_set_new (location_p, ME_COORDINATES_S, coords_p) == 0)
																						{
																							/*
																							 * For GeoJSON objects, the longitude comes first
																							 */
																							if (AddRealToJSONArray (coords_p, me_p -> me_longitude))
																								{
																									if (AddRealToJSONArray (coords_p, me_p -> me_latitude))
																										{
																											if (AddNonTrivialDateToJSON (marti_json_p, ME_START_DATE_S, me_p -> me_time_p))
																												{
																													if (SetJSONString (marti_json_p, INDEXING_TYPE_S, "Grassroots:MARTiSample"))
																														{
																															if (SetJSONString (marti_json_p, INDEXING_TYPE_DESCRIPTION_S, "MARTi Sample"))
																																{
																																	return marti_json_p;
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
																		}
																}
															else
																{
																	json_decref (location_p);
																}

														}
												}

										}
								}
						}

				}		/*if (AddCompoundIdToJSON (marti_json_p, me_p -> me_id_p)) */



			json_decref (marti_json_p);
		}

	return NULL;
}


MartiEntry *GetMartiEntryFromJSON (const json_t *json_p, const MartiServiceData *data_p)
{
	MartiEntry *marti_p = NULL;
	bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

	if (id_p)
		{
			if (GetMongoIdFromJSON (json_p, id_p))
				{
					const char *name_s = GetJSONString (json_p, ME_NAME_S);

					if (name_s)
						{
							const char *marti_id_s = GetJSONString (json_p, ME_MARTI_ID_S);

							if (marti_id_s)
								{
									const json_t *location_p = json_object_get (json_p, ME_LOCATION_S);

									if (location_p)
										{
											const json_t *coords_p = json_object_get (location_p, ME_COORDINATES_S);

											if (coords_p)
												{
													if ((json_is_array (coords_p)) && (json_array_size (coords_p) == 2))
														{
															json_t *entry_p = json_array_get (coords_p, 0);

															if (json_is_number (entry_p))
																{
																	double64 longitude = json_real_value (entry_p);

																	entry_p = json_array_get (coords_p, 1);

																	if (json_is_number (entry_p))
																		{
																			double64 latitude = json_real_value (entry_p);
																			struct tm *start_p = NULL;

																			if (GetNonTrivialDateFromJSON (json_p, ME_START_DATE_S, &start_p))
																				{
																					const char *site_name_s = GetJSONString (json_p, ME_SITE_NAME_S);
																					const char *description_s = GetJSONString (json_p, ME_DESCRIPTION_S);

																					User *user_p = NULL;
																					PermissionsGroup *permissions_group_p = NULL;

																					marti_p = AllocateMartiEntry (id_p, user_p, permissions_group_p, true, name_s, marti_id_s, site_name_s, description_s, latitude, longitude, start_p);

																					if (start_p)
																						{
																							FreeTime (start_p);
																						}
																				}
																		}
																}
														}
												}
										}
								}
						}
				}

			if (!marti_p)
				{
					FreeBSONOid (id_p);
				}
		}

	return marti_p;
}


static bool AddRealToJSONArray (json_t *array_p, const double64 value)
{
	json_t *value_p = json_real (value);

	if (value_p)
		{
			if (json_array_append_new (array_p, value_p) == 0)
				{
					return true;
				}
			else
				{
					json_decref (value_p);
				}
		}

	return false;
}


static bool AddNonTrivialDateToJSON (json_t *json_p, const char * const key_s, const struct tm *date_p)
{
	if (date_p)
		{
			char *time_s = GetTimeAsString (date_p, true, NULL);

			if (time_s)
				{
					if (SetJSONString (json_p, key_s, time_s))
						{
							return true;
						}

					FreeTimeString (time_s);
				}

		}

	return false;
}


static bool GetNonTrivialDateFromJSON (json_t *json_p, const char * const key_s, const struct tm **time_pp)
{
	bool success_flag = false;
	char *time_s = GetJSONString (json_p, key_s);

	if (time_s)
		{
			struct tm *time_p = GetTimeFromString (time_s);

			if (time_p)
				{
					*time_pp = time_p;
					success_flag = true;
				}

		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}



OperationStatus SaveMartiEntry (MartiEntry *marti_p, ServiceJob *job_p, MartiServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (marti_p -> me_id_p), &selector_p);

	if (success_flag)
		{
			json_t *marti_json_p = GetMartiEntryAsJSON (marti_p, data_p);

			if (marti_json_p)
				{
					if (SaveMongoDataWithTimestamp (data_p -> msd_mongo_p, marti_json_p, data_p -> msd_collection_s,
																					selector_p, MONGO_TIMESTAMP_S))
						{
							bool success_flag = true;

							if (data_p -> msd_api_url_s)
								{
									char *url_s = ConcatenateStrings (data_p -> msd_api_url_s, marti_p -> me_marti_id_s);

									if (url_s)
										{
											if (!SetJSONString (marti_json_p, CONTEXT_PREFIX_SCHEMA_ORG_S "url", url_s))
												{
													success_flag = false;
												}
										}
									else
										{
											success_flag = false;
										}
								}

							if (success_flag)
								{
									if (IndexData (job_p, marti_json_p, NULL))
										{
											status = OS_SUCCEEDED;
										}
									else
										{
											status = OS_PARTIALLY_SUCCEEDED;
										}
								}
							else
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}
						}

					json_decref (marti_json_p);
				}		/* if (marti_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get MARTi Entry \"%s\" as JSON", marti_p -> me_sample_name_s);
					success_flag = false;
				}

		}		/* if (location_p -> lo_id_p) */

	SetServiceJobStatus (job_p, status);

	return status;
}

