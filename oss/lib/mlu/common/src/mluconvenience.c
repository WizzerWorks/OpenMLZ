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
#include <stdlib.h>

#include <ML/ml.h>
#include <ML/mlu.h>
#include <ML/ml_didd.h>
#include <ML/ml_private.h>

/* ---------------------------------------------------------------mluPvPrintMsg
 *
 * Print a message (list of param/value pairs) as it would be
 * interpreted by a particular device.
 *
 * The last element in the list must be ML_END.
 */
MLstatus MLAPI mluPvPrintMsg( MLint64 deviceId, MLpv* pvs )
{
  char buffer[1024];
  MLopenid thisDevice = deviceId;
  int thisDeviceSelected = 1;

  if ( pvs==NULL ) {
    return ML_STATUS_INVALID_ARGUMENT;
  }

  while ( pvs->param != ML_END ) {
    MLint32 size = sizeof( buffer );
    MLstatus ret = 0;
    if ( pvs->param == ML_SELECT_ID_INT64 ) {
      ret = mlPvToString( deviceId, pvs, buffer, &size );
      mlDIprocessSelectIdParam( pvs->value.int64, deviceId, &thisDevice );
      thisDeviceSelected = (thisDevice != 0);

    } else {
      if ( thisDeviceSelected ) {
	ret = mlPvToString( thisDevice, pvs, buffer, &size );
      } else {
	sprintf( buffer, "(deselected parameter 0x%" FORMAT_LLX ")",
		 pvs->param );
      }
    }
    if ( ret ) {
      return ret;
    }
    printf( "%s\n", buffer );
    pvs++;
  }
  printf( "ML_END\n" );

  return ML_STATUS_NO_ERROR;
}


