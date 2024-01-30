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

/**
 * @file
 * @brief
 */
/*
 * parental_genotype_service_data.h
 *
 *  Created on: 18 Nov 2018
 *      Author: tyrrells
 */

#ifndef MARTI_SERVICE_DATA_H_
#define MARTI_SERVICE_DATA_H_

#include "marti_service_library.h"
#include "jansson.h"

#include "service.h"
#include "mongodb_tool.h"



/**
 * The configuration data used by the Marti Service.
 *
 * @extends ServiceData
 */
typedef struct /*MARTI_SERVICE_LOCAL*/ MartiServiceData
{
	/** The base ServiceData. */
	ServiceData msd_base_data;


	/**
	 * @private
	 *
	 * The MongoTool to connect to the database where our data is stored.
	 */
	MongoTool *msd_mongo_p;


	/**
	 * @private
	 *
	 * The name of the database to use.
	 */
	const char *msd_database_s;


	/**
	 * @private
	 *
	 * The collection name of the population parentl-cross data use.
	 */
	const char *msd_populations_collection_s;


} MartiServiceData;



#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_MARTI_SERVICE_TAGS
	#define MARTI_PREFIX MARTI_SERVICE_LOCAL
	#define MARTI_VAL(x)	= x
	#define MARTI_CONCAT_VAL(x,y) = x y
	#define MARTI_STRUCT_VAL(x,y)	= { x, y}
#else
	#define MARTI_PREFIX extern
	#define MARTI_VAL(x)
	#define MARTI_CONCAT_VAL(x,y) = x y
	#define MARTI_STRUCT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */

/** The prefix to use for Field Trial Service aliases. */
#define GT_GROUP_ALIAS_PREFIX_S "marti"




MARTI_PREFIX NamedParameterType MA_NAME MARTI_STRUCT_VAL ("Name", PT_STRING );
MARTI_PREFIX NamedParameterType MA_MARTI_ID MARTI_STRUCT_VAL ("MARTi Id", PT_STRING );
MARTI_PREFIX NamedParameterType MA_LATITUDE MARTI_STRUCT_VAL ("Latitude", PT_SIGNED_REAL );
MARTI_PREFIX NamedParameterType MA_LONGITUDE MARTI_STRUCT_VAL ("Longitude", PT_SIGNED_REAL );
MARTI_PREFIX NamedParameterType MA_START_DATE MARTI_STRUCT_VAL ("Start Date", PT_TIME );
MARTI_PREFIX NamedParameterType MA_END_DATE MARTI_STRUCT_VAL ("End Date", PT_TIME );



#ifdef __cplusplus
extern "C"
{
#endif

MARTI_SERVICE_LOCAL MartiServiceData *AllocateMartiServiceData (void);


MARTI_SERVICE_LOCAL void FreeMartiServiceData (MartiServiceData *data_p);


MARTI_SERVICE_LOCAL bool ConfigureMartiService (MartiServiceData *data_p, GrassrootsServer *grassroots_p);


MARTI_SERVICE_LOCAL bool AddCommonParameters (ParameterSet *param_set_p, ParameterGroup *param_group_p, ServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* MARTI_SERVICE_DATA_H_ */
