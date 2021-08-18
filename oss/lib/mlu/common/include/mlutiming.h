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

#ifndef MLU_TIMING_H
#define MLU_TIMING_H


#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mltypes.h>
#include <ML/ml.h>

#define	MLU_TIMING_IGNORE_INVALID	0x0001
#define	MLU_TIMING_FULL_RASTER		0x0002	/* not yet */
#define	MLU_TIMING_SQUARE_TIMING	0x0004	/* not yet */
#define	MLU_TIMING_HANC_AREA		0x0008	/* not yet */
#define	MLU_TIMING_VANC_AREA		0x0010	/* not yet */

MLstatus MLAPI mluComputePathParamsFromTiming( MLint32 timing, MLpv* pv,
					       MLint32 flags );
MLstatus MLAPI mluComputeImagePixelSize( MLpv *params,
					 MLint32 *bytesPerPixelNumRet,
					 MLint32 *bytesPerPixelDenomRet );
MLstatus MLAPI mluGetImagePixelSize( MLopenid openPathOrPipe,
				     MLint32 *bytesPerPixelNumRet,
				     MLint32 *bytesPerPixelDenomRet );
MLstatus MLAPI mluGetImageBufferSize( MLopenid openPathOrPipe,
				      MLint32* bufferSize );
MLstatus MLAPI mluComputeImageBufferSize( MLpv* imageParams,
					  MLint32* bufferSize );
MLstatus MLAPI mluGetAudioBufferSize( MLint64 openDeviceHandle,
				      int frameCount, int* result );
#ifdef __cplusplus
}
#endif

#endif /* MLU_TIMING_H */
