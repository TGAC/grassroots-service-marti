/*
 * marti_entry.c
 *
 *  Created on: 31 Jan 2024
 *      Author: billy
 */

#include "marti_entry.h"


static const char * const S_NAME_S = CONTEXT_PREFIX_SCHEMA_ORG_S "name";


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
																	entry_p -> me_longiitude = longitutde;
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


json_t *GetMartiEntryAsJSON (MartiEntry *me_p, MartiServiceData *data_p)
{

}


MartiEntry *GetMartiEntryFromJSON (const json_t *json_p, const MartiServiceData *data_p)
{

}
