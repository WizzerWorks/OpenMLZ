/***************************************************************************
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free 
 * Software License C, Version 1.0 (the "License"), the contents of this 
 * file are subject only to the provisions of the License. You may not use 
 * this file except in compliance with the License. You may obtain a copy 
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 
 * 1500 Crittenden Lane, Mountain View, CA 94043, or at: 
 *   
 * http://oss.sgi.com/projects/FreeC 
 *
 * Note that, as provided in the License, the Software is distributed 
 * on an "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND 
 * CONDITIONS DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED 
 * WARRANTIES AND CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT. 
 * 
 * Original Code. The Original Code is: OpenML ML Library, 1.1, 12/13/2001,
 * developed by Silicon Graphics, Inc. 
 * ML1.1 is Copyright (c) 2001 Silicon Graphics, Inc. 
 * Copyright in any portions created by third parties is as indicated 
 * elsewhere herein. All Rights Reserved. 
 *
 ***************************************************************************/

#include <stdio.h>
#include <ML/ml.h>
#include <ML/mlutiming.h>

#define	MAX_ROWS(t,c)	(sizeof(t)/sizeof(t[0][0])/(c))

#define MLU_MAX_TT_COLUMNS 21

static double pathTimingData[][MLU_MAX_TT_COLUMNS] =  {

  /* the timing table is generated from the following text file - it
   * has MLU_MAX_TT_COLUMNS - the first row has param ids, the first
   * column has possible values for ML_JACK_TIMING, each entry in the
   * table corresponds to the default value for that row's param-id
   * and that column's timing.  Entries are separated by commas.
   */
   #include "mlutimings.txt"
};

#define MLU_MAX_TT_ROWS MAX_ROWS( pathTimingData, MLU_MAX_TT_COLUMNS )


/* ----------------------------------------------------------------mluSetParams
 */
static MLstatus MLAPI
mluSetParams( double* paramIdRow, double* paramValueRow, int max,
	      MLpv* pv, MLint32 flags )
{
  MLstatus error = ML_STATUS_NO_ERROR;

  /* Now set each parameter from it's value in the table
   */
  for ( ; pv->param != ML_END; pv++ ) {
    int col;

    for ( col = 0; col < max; col++ ) {
      MLint64 paramRow = (MLint64) paramIdRow[ col ];

      if ( paramRow == pv->param ) {
	pv->length = 1;

	switch ( ML_PARAM_GET_TYPE( paramIdRow[ col ] ) ) {

	case ML_TYPE_INT32:
	  pv->value.int32 = (MLint32)paramValueRow[ col ];
	  if( pv->value.int32 == -1 ) {
	    pv->length = -1;
	  }
	  break;

	case ML_TYPE_INT64:
	  pv->value.int64 = (MLint64)paramValueRow[ col ];
	  if( pv->value.int64 == -1 ) {
	    pv->length = -1;
	  }
	  break;

	case ML_TYPE_REAL32:
	  pv->value.real32 = (MLreal32)paramValueRow[ col ];
	  if( pv->value.real32 == -1.0 ) {
	    pv->length = -1;
	  }
	  break;

	case ML_TYPE_REAL64:
	  pv->value.real64 = (MLreal64)paramValueRow[ col ];
	  if( pv->value.real64 == -1.0 ) {
	    pv->length = -1;
	  }
	  break;

	default:
	  pv->length = -1;
	  fprintf( stderr, "[mluGetPathDefaultsFrom* mluSetParams] "
		   "param type %d not recognized.\n",
		   ML_PARAM_GET_TYPE( paramIdRow[ col ] ) );
	  error = ML_STATUS_INTERNAL_ERROR;
	  break;
	}
	break;
      }
    }
    if ( ( flags & MLU_TIMING_IGNORE_INVALID ) == 0 &&
	 ( col == max ) ) {
      pv->length = -1;
      fprintf( stderr, "[mluGetPathDefaultsFrom* mluSetParams] param"
	       " not recognized.\n" );
      error = ML_STATUS_INVALID_PARAMETER;
    }
  }
  return error;
}


/* ----------------------------------------------mluComputePathParamsFromTiming
 */
MLstatus MLAPI
mluComputePathParamsFromTiming( MLint32 timing, MLpv* pv, MLint32 flags )
{
  int row;

  /* First check that the specified timing actually exists - timing is
   * the first entry of each row (except the very first row).
   */
  for ( row = 1; row < MLU_MAX_TT_ROWS; row++ ) {
    if ( pathTimingData[ row ][ 0 ] == timing ) {
      break;
    }
  }

  if ( row == MLU_MAX_TT_ROWS ) {
    fprintf( stderr,
	     "[mluComputePathParamsFromTiming] Timing value of "
	     "%d not recognized.\n", timing );

    return ML_STATUS_INVALID_VALUE;
  }

  /* Now set each parameter from it's value in the table
   */
  return mluSetParams( &(pathTimingData[0][0]), 
		       &(pathTimingData[row][0]),
		       MLU_MAX_TT_COLUMNS,
		       pv,
		       flags );
}





