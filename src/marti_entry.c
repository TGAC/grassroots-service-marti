/*
 * marti_entry.c
 *
 *  Created on: 31 Jan 2024
 *      Author: billy
 */

#include "marti_entry.h"
#include "memory_allocations.h"
#include "json_util.h"

#include "string_utils.h"
#include "time_util.h"


static bool AddRealToJSONArray (json_t *array_p, const double64 value);

static bool AddNonTrivialDateToJSON (json_t *json_p, const char * const key_s, const struct tm *date_p);



MartiEntry *AllocateMartiEntry (bson_oid_t *id_p, User *user_p, PermissionsGroup *permissions_group_p, const bool owns_user_flag,
																										const char *name_s, const char *marti_id_s, double64 latitutde, double64 longitutde,
																										struct tm *start_p, struct tm *end_p)
{
	if (marti_id_s)
		{
			char *copied_marti_id_s = EasyCopyToNewString (marti_id_s);

			if (copied_marti_id_s)
				{
					if (start_p)
						{
							struct tm *copied_start_p = DuplicateTime (start_p);

							if (copied_start_p)
								{

									char *copied_name_s = NULL;

									if ((name_s == NULL) || (copied_name_s = EasyCopyToNewString (name_s)))
										{
											struct tm *copied_end_p = NULL;

											if ((end_p == NULL) || ((copied_end_p = DuplicateTime (start_p)) != NULL))
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
																	entry_p -> me_name_s = copied_name_s;
																	entry_p -> me_marti_id_s = copied_marti_id_s;
																	entry_p -> me_latitude = latitutde;
																	entry_p -> me_longitude = longitutde;
																	entry_p -> me_start_p = copied_start_p;
																	entry_p -> me_end_p = copied_end_p;

																	return entry_p;
																}


															if (alloc_perms_flag)
																{
																	FreePermissionsGroup (permissions_group_p);
																}

														}

													if (copied_end_p)
														{
															FreeTime (copied_end_p);
														}		/* if (copied_end_p) */

												}

											if (copied_name_s)
												{
													FreeCopiedString (copied_name_s);
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

	if (marti_p -> me_name_s)
		{
			FreeCopiedString (marti_p -> me_name_s);
		}

	if (marti_p -> me_marti_id_s)
		{
			FreeCopiedString (marti_p -> me_marti_id_s);
		}

	if (marti_p -> me_start_p)
		{
			FreeTime (marti_p -> me_start_p);
		}

	if (marti_p -> me_end_p)
		{
			FreeTime (marti_p -> me_end_p);
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
					if (SetJSONString (marti_json_p, ME_NAME_S, me_p -> me_name_s))
						{
							if (SetNonTrivialString (marti_json_p, ME_MARTI_ID_S, me_p -> me_marti_id_s, true))
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
													if (SetJSONString (location_p, "@type", "Point"))
														{
															json_t *coords_p = json_array ();

															if (coords_p)
																{
																	if (json_object_set_new (location_p, "coordinates", coords_p) == 0)
																		{
																			/*
																			 * For GeoJSON objects, the longitude comes first
																			 */
																			if (AddRealToJSONArray (coords_p, me_p -> me_longitude))
																				{
																					if (AddRealToJSONArray (coords_p, me_p -> me_latitude))
																						{
																							if (AddNonTrivialDateToJSON (marti_json_p, ME_START_DATE_S, me_p -> me_start_p))
																								{
																									if (AddNonTrivialDateToJSON (marti_json_p, ME_END_DATE_S, me_p -> me_end_p))
																										{
																											return marti_json_p;
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

				}		/*if (AddCompoundIdToJSON (marti_json_p, me_p -> me_id_p)) */



			json_decref (marti_json_p);
		}

	return NULL;
}


MartiEntry *GetMartiEntryFromJSON (const json_t *json_p, const MartiServiceData *data_p)
{
	bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

	if (id_p)
		{
			if (GetMongoIdFromJSON (json_p, id_p))
				{
					const char *name_s = GetJSONString (json_p, ME_NAME_S);
				}

			FreeBSONOid (id_p);
		}

	return NULL;
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
			char *time_s = GetTimeAsString (date_p, false, NULL);

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

