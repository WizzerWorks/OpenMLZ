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

#ifndef __INC_MLIMAGE_H__
#define __INC_MLIMAGE_H__  


#ifdef __cplusplus
extern "C" {
#endif

#include <ML/mldefs.h>
#include <ML/mlparam.h>

/* Image parameters specify how images are stored or how they should
 * be interpreted by SDK subsystems.
 */
enum MLpvImageClassEnum
{
  /* Buffer pointers are in different include files.  use 'I' to
   * distinguish "Image" BP.
   */
  ML_IMAGE_BUFFER_POINTER    = ML_PARAM_NAME( ML_CLASS_BUFFER,
					      ML_TYPE_BYTE_POINTER, 'I' ),
  ML_IMAGE_WIDTH_INT32       = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x02 ),
  ML_IMAGE_HEIGHT_1_INT32    = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x03 ),
  ML_IMAGE_HEIGHT_2_INT32    = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x04 ),
  ML_IMAGE_ROW_BYTES_INT32   = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x05 ),
  ML_IMAGE_SKIP_PIXELS_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x06 ),
  ML_IMAGE_SKIP_ROWS_INT32   = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x07 ),
  ML_IMAGE_ORIENTATION_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x08 ),
  ML_IMAGE_BUFFER_SIZE_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x09 ),

  /* Field/frame description
   */
  ML_IMAGE_TEMPORAL_SAMPLING_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
						    ML_TYPE_INT32, 0x10 ),
  ML_IMAGE_INTERLEAVE_MODE_INT32   = ML_PARAM_NAME( ML_CLASS_IMAGE,
						    ML_TYPE_INT32, 0x11 ),
  ML_IMAGE_DOMINANCE_INT32         = ML_PARAM_NAME( ML_CLASS_IMAGE,
						    ML_TYPE_INT32, 0x12 ),

  /* Pixel storage descriptions
   */
  ML_IMAGE_COMPRESSION_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x20 ),
  ML_IMAGE_SAMPLING_INT32    = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x22 ),
  ML_IMAGE_COLORSPACE_INT32  = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x23 ),
  ML_IMAGE_PACKING_INT32     = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x24 ),
  ML_IMAGE_SWAP_BYTES_INT32  = ML_PARAM_NAME( ML_CLASS_IMAGE,
					      ML_TYPE_INT32, 0x27 ),

  /* Gamma of the stored image (see also ML_JACK_GAMMA_REAL32).  the
   * difference between the gamma of the input image/jack and that of
   * the output image/jack determines the amount of gamma correction
   * (if any).
   *
   * FIXME: initial draft implementation of this parameter
   *        need detailed definition with equations and default value.
   */
  ML_IMAGE_GAMMA_REAL32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					 ML_TYPE_REAL32, 0x26 ),

  /* Desired compression factor, a value of 1 indicates no
   * compression, a value of 10 indicates that approximatly 10
   * compressed buffers require the same space as 1 uncompressed
   * buffer.
   *
   * The size of the uncompressed buffer depends on image width,
   * height, packing and sampling.
   */
  ML_IMAGE_COMPRESSION_FACTOR_REAL32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
						      ML_TYPE_REAL32, 0x33 ),

  /* Describes what happens to alpha when going xxx -> xxx4
   */
  ML_IMAGE_ALPHA_FILL_INT32 = ML_PARAM_NAME( ML_CLASS_IMAGE,
					     ML_TYPE_INT32, 0x40 )
};


/* For progressive scan images (ML_IMAGE_TEMPORAL_SAMPLING_INT32 is
 * progressive), the height is only set on the first buffer
 * (ML_IMAGE_HEIGHT_1_INT32). For convenience, this parameter is
 * aliased to ML_IMAGE_HEIGHT_INT32.
 */
enum MLpvImageClassDuplicatesEnum
{
  ML_IMAGE_HEIGHT_INT32 = ML_IMAGE_HEIGHT_1_INT32
};


/* ML_IMAGE_ORIENTATION_INT32 - Specifies how an image's scan lines
 * are stored in memory.
 */
enum mlOrientationEnum
{
  ML_ORIENTATION_TOP_TO_BOTTOM,
  ML_ORIENTATION_BOTTOM_TO_TOP
};


/* ML_IMAGE_TEMPORAL_SAMPLING_INT32 - Specifies whether images are
 * progressive scan or field-based.
 */
enum mlTemporalSamplingEnum
{
  ML_TEMPORAL_SAMPLING_FIELD_BASED,
  ML_TEMPORAL_SAMPLING_PROGRESSIVE
};


/* ML_IMAGE_INTERLEAVE_MODE - For field-based images, specifies
 * whether the images have been interleaved into a single image (and
 * reside in a single buffer) or are stored in two separate fields
 * (hence in two separate buffers).
 */
enum mlInterleaveModeEnum
{
  ML_INTERLEAVE_MODE_INTERLEAVED,
  ML_INTERLEAVE_MODE_SINGLE_FIELD
};


/* ML_IMAGE_DOMINANCE_INT32 enumerations.  For images with a
 * field-based temporal sampling, describes the field dominance.
 */
enum mlImageDominanceEnum
{
  ML_DOMINANCE_F1,
  ML_DOMINANCE_F2
};


/* Building blocks for making the colorspace values Legal mlSDK
 * colorspaces result from a combination of these values - see
 * ML_IMAGE_COLORSPACE below.
 */
enum mlColorspaceBuildingEnum
{
  ML_RANGE_FULL           = 0x0001,
  ML_RANGE_HEAD           = 0x0002,
  ML_RANGE_MASK           = 0x000f,

  ML_STANDARD_UNSPECIFIED = 0x0000,
  ML_STANDARD_601         = 0x0010,
  ML_STANDARD_240M        = 0x0020,
  ML_STANDARD_709         = 0x0030,
  ML_STANDARD_MASK        = 0x00f0,

  ML_REPRESENTATION_RGB   = 0x0100,
  ML_REPRESENTATION_CbYCr = 0x0200,
  ML_REPRESENTATION_MASK  = 0x0f00
};


/* ML_IMAGE_COLORSPACE_INT32 - specifies the colorspace
 * of the pixels in the image buffer.
 *
 * Key to decoding these is ML_COLORSPACE_c_s_r
 * Where c is either RGB or CbYCr
 *       s is the color standard 
 *       r is the range - full or headroom
 */
enum mlColorspaceEnum 
{
  /* Colorspaces based on ITU-R BT.601-5
   */
  ML_COLORSPACE_RGB_601_FULL   = ML_REPRESENTATION_RGB | ML_STANDARD_601 |
                                 ML_RANGE_FULL,
  ML_COLORSPACE_RGB_601_HEAD   = ML_REPRESENTATION_RGB | ML_STANDARD_601 |
                                 ML_RANGE_HEAD,
  ML_COLORSPACE_CbYCr_601_FULL = ML_REPRESENTATION_CbYCr | ML_STANDARD_601 |
                                 ML_RANGE_FULL,
  ML_COLORSPACE_CbYCr_601_HEAD = ML_REPRESENTATION_CbYCr | ML_STANDARD_601 |
                                 ML_RANGE_HEAD,

  /* Colorspaces based on SMPTE 240M
   */
  ML_COLORSPACE_RGB_240M_FULL   = ML_REPRESENTATION_RGB | ML_STANDARD_240M |
                                  ML_RANGE_FULL,
  ML_COLORSPACE_RGB_240M_HEAD   = ML_REPRESENTATION_RGB | ML_STANDARD_240M |
                                  ML_RANGE_HEAD,
  ML_COLORSPACE_CbYCr_240M_FULL = ML_REPRESENTATION_CbYCr | ML_STANDARD_240M |
                                  ML_RANGE_FULL,
  ML_COLORSPACE_CbYCr_240M_HEAD = ML_REPRESENTATION_CbYCr | ML_STANDARD_240M |
                                  ML_RANGE_HEAD,

  /* Colorspaces based on ITU-R BT.709-2 60Hz
   */
  ML_COLORSPACE_RGB_709_FULL    = ML_REPRESENTATION_RGB | ML_STANDARD_709 |
                                  ML_RANGE_FULL,
  ML_COLORSPACE_RGB_709_HEAD    = ML_REPRESENTATION_RGB | ML_STANDARD_709 |
                                  ML_RANGE_HEAD,
  ML_COLORSPACE_CbYCr_709_FULL  = ML_REPRESENTATION_CbYCr | ML_STANDARD_709 |
                                  ML_RANGE_FULL,
  ML_COLORSPACE_CbYCr_709_HEAD  = ML_REPRESENTATION_CbYCr | ML_STANDARD_709 |
                                  ML_RANGE_HEAD
};


/* Some useful macros for checking colorspace
 */

#define ML_GET_COLORSPACE_RANGE(cs)          ((cs)&ML_RANGE_MASK)
#define ML_GET_COLORSPACE_STANDARD(cs)       ((cs)&ML_STANDARD_MASK)
#define ML_GET_COLORSPACE_REPRESENTATION(cs) ((cs)&ML_REPRESENTATION_MASK)

#define ML_IS_COLORSPACE_FULL(cs)  \
(ML_GET_COLORSPACE_RANGE(cs) == ML_RANGE_FULL )
#define ML_IS_COLORSPACE_HEAD(cs)  \
(ML_GET_COLORSPACE_RANGE(cs) == ML_RANGE_HEAD )

#define ML_IS_COLORSPACE_601(cs)   \
(ML_GET_COLORSPACE_STANDARD(cs) == ML_STANDARD_601 )
#define ML_IS_COLORSPACE_240M(cs)  \
(ML_GET_COLORSPACE_STANDARD(cs) == ML_STANDARD_240M )
#define ML_IS_COLORSPACE_709(cs)   \
(ML_GET_COLORSPACE_STANDARD(cs) == ML_STANDARD_709 )

#define ML_IS_COLORSPACE_RGB(cs)   \
(ML_GET_COLORSPACE_REPRESENTATION(cs) == ML_REPRESENTATION_RGB )
#define ML_IS_COLORSPACE_CbYCr(cs) \
(ML_GET_COLORSPACE_REPRESENTATION(cs) == ML_REPRESENTATION_CbYCr )


/* ML_IMAGE_SAMPLING_INT32 - specifies the sampling of the pixels in
 * the image buffer.
 */
enum mlSamplingEnum 
{
  ML_SAMPLING_4444        =0x004444,
  ML_SAMPLING_4224        =0x004224,
  ML_SAMPLING_444         =0x004440,
  ML_SAMPLING_422         =0x004220,
  ML_SAMPLING_420_MPEG1   =0x104200,
  ML_SAMPLING_420_MPEG2   =0x204200,
  ML_SAMPLING_420_DVC625  =0x304200, 
  ML_SAMPLING_411_DVC525  =0x404110,
  ML_SAMPLING_4004        =0x004004,
  ML_SAMPLING_400         =0x004000,
  ML_SAMPLING_0004        =0x000004   /* linear alpha */
};

/* Backward compatibility
 */
#define ML_SAMPLING_411_DVC ML_SAMPLING_411_DVC525


/* Some useful macros for checking sampling
 */
#define ML_IS_SAMPLING_422x(s) (((s)&0x00fff0)==0x004220)
#define ML_IS_SAMPLING_444x(s) (((s)&0x00fff0)==0x004440)
#define ML_IS_SAMPLING_xxx4(s) (((s)&0x00000f)==0x000004)


/* A useful macro for defining component packing order
 */
#define ML_PACKING_FIELD_OFFSET 12
#define ML_PACKING_ORDER(A,B,C,D) ((((A-1)&3)<<6|(((B-1)&3)<<4)|(((C-1)&3)<<2)|((D-1)&3)) << ML_PACKING_FIELD_OFFSET)


/* Building blocks for making packings
 *
 * Legal mlSDK packings require a combination of these, see image
 * packing below.
 */
enum mlPackingBuildingEnum
{
  /* The packing detail is the number of information bits per
   * component for simple and padded packings.  For complex packings,
   * each packing has a unique detail number.
   */
  ML_PACKING_PAD_FROM_MS   = 0x00000000, /* pad by repeating the most
					  * significant part of value
					  */
  ML_PACKING_PAD_ZEROS     = 0x00400000, /* pad with zeros */
  ML_PACKING_PAD_FROM_LS   = 0x00800000, /* pad with repeating the
					  * least significant bit
					  */
  ML_PACKING_DETAIL_MASK   = 0x000000ff,

  ML_PACKING_UNSIGNED      = 0x00000100,
  ML_PACKING_SIGNED        = 0x00000200,
  ML_PACKING_REAL          = 0x00000300,
  ML_PACKING_TYPE_MASK     = 0x00000f00,


  /* Packing order definitions adhere to the following bit assignment
   * for component position.
   *
   * b7b6 << 12 = comp1 position (0=first,1=second,2=third,3=forth)
   * b5b4 << 12 = comp2 position (0=first,1=second,2=third,3=forth)
   * b3b2 << 12 = comp3 position (0=first,1=second,2=third,3=forth)
   * b1b0 << 12 = comp4 position (0=first,1=second,2=third,3=forth)
   */
  ML_PACKING_1234          = ML_PACKING_ORDER(1,2,3,4),  
  ML_PACKING_1243          = ML_PACKING_ORDER(1,2,4,3),
  ML_PACKING_1324          = ML_PACKING_ORDER(1,3,2,4),
  ML_PACKING_1342          = ML_PACKING_ORDER(1,3,4,2),
  ML_PACKING_1423          = ML_PACKING_ORDER(1,4,2,3),
  ML_PACKING_1432          = ML_PACKING_ORDER(1,4,3,2),
  ML_PACKING_2134          = ML_PACKING_ORDER(2,1,3,4),
  ML_PACKING_2143          = ML_PACKING_ORDER(2,1,4,3),
  ML_PACKING_2314          = ML_PACKING_ORDER(2,3,1,4),
  ML_PACKING_2341          = ML_PACKING_ORDER(2,3,4,1),
  ML_PACKING_2413          = ML_PACKING_ORDER(2,4,1,3),
  ML_PACKING_2431          = ML_PACKING_ORDER(2,4,3,1),
  ML_PACKING_3124          = ML_PACKING_ORDER(3,1,2,4),
  ML_PACKING_3142          = ML_PACKING_ORDER(3,1,4,2),
  ML_PACKING_3214          = ML_PACKING_ORDER(3,2,1,4),
  ML_PACKING_3241          = ML_PACKING_ORDER(3,2,4,1),
  ML_PACKING_3412          = ML_PACKING_ORDER(3,4,1,2),
  ML_PACKING_3421          = ML_PACKING_ORDER(3,4,2,1),
  ML_PACKING_4123          = ML_PACKING_ORDER(4,1,2,3),
  ML_PACKING_4132          = ML_PACKING_ORDER(4,1,3,2),
  ML_PACKING_4213          = ML_PACKING_ORDER(4,2,1,3),
  ML_PACKING_4231          = ML_PACKING_ORDER(4,2,3,1),
  ML_PACKING_4312          = ML_PACKING_ORDER(4,3,1,2),
  ML_PACKING_4321          = ML_PACKING_ORDER(4,3,2,1),

  ML_PACKING_ORDER_MASK    = 0x000ff000,  

  ML_PACKING_SIMPLE        = 0x10000000,
  ML_PACKING_IN16L         = 0x21100000,
  ML_PACKING_IN16R         = 0x21200000,
  ML_PACKING_COMPLEX       = 0x40000000,
  ML_PACKING_CLASS_MASK    = 0x70000000
};


/* ML_IMAGE_PACKING_INT32 - specifies the packing
 * of the pixels in the image buffer.
 *
 * ML_PACKING_{signed}{size}{order} 
 *
 * {signed} 'S' if signed, or blank if unsigned component values
 *
 * {size} is the number of bits in each component.  May be a single
 * number (if all components are the same size).  May end 'in16L' if
 * each component is left-shifted in a 16-bit word or 'in16R' if
 * component is right-shifted in a 16-bit word.  For complex packings
 * (where all components are packed into a single integer) specify the
 * size of each of the 4 components separated by '_'.
 *
 * {order} is the order in which components are packed. Empty to
 * indicate normal component ordering (1,2,3,4), 'R' to indicate
 * reversed component ordering (4,3,2,1) or a numeric sequence to
 * indicate an unusual component order (e.g. 1324 is RBGA).
 */

enum mlPackingEnum 
{
  /* Simple packings
   * - all components the same size
   * - no padding between components
   * - treat as a stream of components
   */
  ML_PACKING_8       = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 8  |
                       ML_PACKING_1234,
  ML_PACKING_8_R     = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 8  |
                       ML_PACKING_4321,
  ML_PACKING_8_4123  = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 8  |
                       ML_PACKING_4123,
  ML_PACKING_8_3214  = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 8  |
                       ML_PACKING_3214,
  ML_PACKING_8_2134  = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 8  |
                       ML_PACKING_2134,
  ML_PACKING_10      = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 10 |
                       ML_PACKING_1234,
  ML_PACKING_10_R    = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 10 |
                       ML_PACKING_4321,
  ML_PACKING_10_2134 = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 10 |
                       ML_PACKING_2134,
  ML_PACKING_10_3214 = ML_PACKING_SIMPLE | ML_PACKING_UNSIGNED | 10 |
                       ML_PACKING_3214,
  ML_PACKING_S12     = ML_PACKING_SIMPLE | ML_PACKING_SIGNED   | 12 |
                       ML_PACKING_1234,

  /* Padded packings
   * - all components the same size
   * - each component padded to 16-bits
   * - treat as a stream of 16-bit shorts
   */
  ML_PACKING_10in16L       = ML_PACKING_IN16L | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_1234,
  ML_PACKING_10in16L_R     = ML_PACKING_IN16L | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_4321,
  ML_PACKING_10in16R       = ML_PACKING_IN16R | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_1234,
  ML_PACKING_10in16R_R     = ML_PACKING_IN16R | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_4321,
  ML_PACKING_10in16L_3214  = ML_PACKING_IN16L | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_3214,
  ML_PACKING_10in16R_3214  = ML_PACKING_IN16R | ML_PACKING_UNSIGNED | 10 |
                             ML_PACKING_3214,

  ML_PACKING_12in16L       = ML_PACKING_IN16L | ML_PACKING_UNSIGNED | 12 |
                             ML_PACKING_1234,
  ML_PACKING_12in16L0_3214 = ML_PACKING_IN16L | ML_PACKING_UNSIGNED | 12 |
                             ML_PACKING_3214 | ML_PACKING_PAD_ZEROS,
  ML_PACKING_S12in16L      = ML_PACKING_IN16L | ML_PACKING_SIGNED   | 12 |
                             ML_PACKING_1234,
  ML_PACKING_S12in16R      = ML_PACKING_IN16R | ML_PACKING_SIGNED   | 12 |
                             ML_PACKING_1234,
  ML_PACKING_S13in16L      = ML_PACKING_IN16L | ML_PACKING_SIGNED   | 13 |
                             ML_PACKING_1234,
  ML_PACKING_S13in16R      = ML_PACKING_IN16R | ML_PACKING_SIGNED   | 13 |
                             ML_PACKING_1234,

  /* Complex packings
   * - all components packed together into a single 32-bit int
   * - treat as a stream of 32-bit integers
   */
  ML_PACKING_10_10_10_2      = ML_PACKING_COMPLEX | 1 | ML_PACKING_1234,
  ML_PACKING_10_10_10_2_R    = ML_PACKING_COMPLEX | 1 | ML_PACKING_4321,
  ML_PACKING_10_10_10_2_3214 = ML_PACKING_COMPLEX | 1 | ML_PACKING_3214,

  /* - all components packed together into a single 16-bit int
   * - treat as a stream of 16-bit integers
   */
  ML_PACKING_1_5_5_5_4123    = ML_PACKING_COMPLEX | 2 | ML_PACKING_4123,

  /* - all components packed together into a single 32-bit int
   * - treat as a stream of 32-bit integers
   */
  ML_PACKING_10_10_10in32L   = ML_PACKING_COMPLEX | 3 | ML_PACKING_1234
};


/* Predefined image sizes (in bytes)
 *
 * due to the nature of DV encoding, image sizes are fixed these
 * defines provide a global definition of those sizes note that
 * suffixes match "ML_COMPRESSION" suffixes.
 */

enum mlImageSizeDefinitions {
    ML_IMAGE_SIZE_DV_625       = 144000,
    ML_IMAGE_SIZE_DV_525       = 120000,
    ML_IMAGE_SIZE_DVCPRO_625   = 144000,
    ML_IMAGE_SIZE_DVCPRO_525   = 120000,
    ML_IMAGE_SIZE_DVCPRO50_625 = 288000,
    ML_IMAGE_SIZE_DVCPRO50_525 = 240000
};


/* Equates to conform to "upper case only" macro standards
 */

#define	ML_REPRESENTATION_CBYCR		ML_REPRESENTATION_CbYCr
#define	ML_COLORSPACE_CBYCR_601_FULL	ML_COLORSPACE_CbYCr_601_FULL
#define	ML_COLORSPACE_CBYCR_601_HEAD	ML_COLORSPACE_CbYCr_601_HEAD
#define	ML_COLORSPACE_CBYCR_240M_FULL	ML_COLORSPACE_CbYCr_240M_FULL
#define	ML_COLORSPACE_CBYCR_240M_HEAD	ML_COLORSPACE_CbYCr_240M_HEAD
#define	ML_COLORSPACE_CBYCR_709_FULL	ML_COLORSPACE_CbYCr_709_FULL
#define	ML_COLORSPACE_CBYCR_709_HEAD	ML_COLORSPACE_CbYCr_709_HEAD
#define	ML_PACKING_10IN16L		ML_PACKING_10in16L
#define	ML_PACKING_10IN16L_R		ML_PACKING_10in16L_R
#define	ML_PACKING_10IN16R		ML_PACKING_10in16R
#define	ML_PACKING_10IN16R_R		ML_PACKING_10in16R_R
#define	ML_PACKING_10IN16L_3214		ML_PACKING_10in16L_3214
#define	ML_PACKING_10IN16R_3214		ML_PACKING_10in16R_3214
#define	ML_PACKING_12IN16L		ML_PACKING_12in16L
#define	ML_PACKING_12IN16L0_3214	ML_PACKING_12in16L0_3214
#define	ML_PACKING_S12IN16L		ML_PACKING_S12in16L
#define	ML_PACKING_S12IN16R		ML_PACKING_S12in16R
#define	ML_PACKING_S13IN16L		ML_PACKING_S13in16L
#define	ML_PACKING_S13IN16R		ML_PACKING_S13in16R
#define ML_PACKING_10_10_10IN32L        ML_PACKING_10_10_10in32L

#ifdef __cplusplus
}
#endif

#endif

