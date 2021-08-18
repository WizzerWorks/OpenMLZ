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

/* ml stuff
 */
#include <ML/ml.h>
#include <ML/mlu.h>

#include<stdlib.h>
#include<stdio.h>
#include<stdarg.h>
#include<string.h>
#include <ctype.h>

#ifdef ML_OS_NT
#include <ML/getopt.h>
#else
#include <getopt.h>
#endif

char *argv0;

#ifdef ML_OS_NT
#define STRCASECMP( str1, str2 ) stricmp( str1, str2 )
#ifdef _MSC_VER
#define snprintf _snprintf
#define strncasecmp _strnicmp
#endif
#else /* UNIX */
#define STRCASECMP( str1, str2 ) strcasecmp( str1, str2 )
#endif

void leave( int code )
{
  exit( code );
}

enum Qmode
{
  NONE=0, BRIEF, STANDARD, ENHANCED
};

typedef struct
{
  Qmode mode;
  MLint32 printSystem;
  MLint32 printDevice;
  MLint32 printJack;
  MLint32 printPath;
  MLint32 printXcode;
  MLint32 printPipe;
  MLint32 printParam;
  MLint32 printUST;
  char* devName;
  char* jackName;
  char* pathName;
  char* xcodeName;
  char* pipeName;
  char* paramName;
  char* ustName;
} Qdetails;

void errorMsg( void );
void printSystem( MLpv* cap, Qdetails* details );
void printDevices( MLpv* cap, Qdetails* details );
void printDevice( MLpv* cap, Qdetails* details );
void printPaths( MLpv* cap, Qdetails* details );
void printPath( MLpv* cap, Qdetails* details );
void printJacks( MLpv* cap, Qdetails* details );
void printJack( MLpv* cap, Qdetails* details );
void printXcodes( MLpv* cap, Qdetails* details );
void printXcode( MLpv* cap, Qdetails* details );
void printPipes(MLpv* cap, Qdetails* details );
void printPipe(MLpv* cap, Qdetails* details );
void printParams( MLpv* cap, Qdetails* details );
void printParam( MLpv* cap, Qdetails* details );
void printUSTSources( MLpv* cap, Qdetails* details );
void printUSTSource( MLpv* cap, Qdetails* details );

#define TABS 16


/* ----------------------------------------------------------printCapParamValue
 */
void printCapParamValue( MLpv* capMsg, MLint64 paramid, const char* preText,
			 const char* postText )
{
  MLbyte buffer[256];
  MLint32 size = sizeof( buffer );
  MLpv* pv = mlPvFind( capMsg, paramid );

  if ( pv == NULL ) {
    return;
  }

  buffer[0] = '\0';

  if ( paramid == ML_PARENT_ID_INT64 ) {
    MLpv* cap;
    if ( mlGetCapabilities( pv->value.int64, &cap ) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }

    printCapParamValue( cap, ML_NAME_BYTE_ARRAY, preText, postText );

    mlFreeCapabilities( cap );
    return;
  }

  if ( paramid == ML_UST_SELECTED_SOURCE_ID_INT64 ) {
    if ( pv->value.int64 == ML_BUILTIN_UST_SOURCE ) {
      /* Can't actually query this UST source -- supply a default name
       */
      sprintf( (char*) buffer, "(default software UST source)" );
    } else {
      MLpv* cap;

      if ( mlGetCapabilities( pv->value.int64, &cap ) ) {
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }

      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, preText, postText);

      mlFreeCapabilities( cap );
      return;
    }
  }

  /* It is possible that we already have our value (see above, for UST
   * source handling), so check before attempting to write to the
   * buffer.
   */
  if ( buffer[0] == '\0' ) {
    if ( mlPvValueToString( 0, pv, (char*) buffer, &size ) ) {
      return;
    }
  }

  if ( *preText == '\t' ) {
    printf( "%*s ", TABS, preText+1 );
  } else {
    printf( preText );
  }
      
  if ( *buffer == '\"' && buffer[size-3] == '\\' && buffer[size-2] == '0' ) {
    /* print the raw string without quoation marks
     */
    buffer[size-3] = '\0';
    printf( "%s%s",((char*)buffer)+1, postText );
  } else {
    printf( "%s%s", (char*) buffer, postText );
  }
}


/* ---------------------------------------------------------printCapParamValue4
 */
void printCapParamValue4( MLpv* capMsg, MLint64 paramid, const char* preText,
			  const char* postText )
{
  char buffer[256], *b = buffer, *e = &buffer[sizeof( buffer )-1];
  MLint32 size = sizeof( buffer );
  MLpv* pv = mlPvFind( capMsg, paramid );
  MLuint32 rev_value;

  if ( pv == NULL ) {
    return;
  }

  if ( paramid == ML_PARENT_ID_INT64 ) {
    MLpv* cap;
    if ( mlGetCapabilities( pv->value.int64, &cap) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }

    printCapParamValue( cap, ML_NAME_BYTE_ARRAY, preText, postText );

    mlFreeCapabilities( cap );
    return;
  }

  rev_value = pv->value.int32;
  {
    MLint32 i, j;
    for ( i = 24; i >= 0; i -= 8 ) {
      if ( ( j = (( rev_value >> i ) & 0xff )) || i==0 ) {
	b += snprintf( b, e - b, "%d%c", j, i?'.':'\0' );
      }
    }
  }
  size = (MLint32) (b - buffer);

  if ( *preText == '\t' ) {
    printf( "%*s ", TABS, preText+1 );
  } else {
    printf( preText );
  }

  if ( *buffer == '\"' && buffer[size-3] == '\\' && buffer[size-2] == '0' ) {
    /* print the raw string without quoation marks
     */
    buffer[size-3] = '\0';
    printf( "%s%s", ((char*) buffer)+1, postText );
  } else {
    printf( "%s%s", buffer, postText );
  }
}


/* -------------------------------------------------------------printCapIdArray
 */
void printCapIdArray( MLpv* capMsg, MLint64 paramid, const char* preText )
{
  MLint32 i;
  MLpv* childCap;
  MLpv* pv = mlPvFind( capMsg, paramid );
  if ( pv == NULL ) {
    return;
  }

  if ( pv->length <= 0 ) {
    return;
  }

  if ( *preText == '\t' ) {
    printf( "%*s ", TABS, preText+1 );
  } else {
    printf( preText );
  }
      
  for ( i=0; i< pv->length; i++ ) {
    if ( mlGetCapabilities( pv->value.pInt64[ i ],&childCap ) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }

    printCapParamValue( childCap, ML_NAME_BYTE_ARRAY, "", "" );

    mlFreeCapabilities( childCap );

    if ( i < pv->length-1 ) {
      printf( ", " );
      if ( *preText == '\t' ) {
	printf( "\n%*s ", TABS,"" );
      }
    } else {
      printf( "\n" );
    }
  }
}


/* --------------------------------------------------------printCapParamIdArray
 */
void printCapParamIdArray( MLpv* capMsg, const char* preText,
			   MLint64 typeParams )
{
  MLint32 i;
  MLpv* childCap;
  MLpv* id = mlPvFind( capMsg, ML_ID_INT64 );
  MLpv* pv = mlPvFind( capMsg, typeParams );
  if ( pv == NULL || id == NULL ) {
    return;
  }

  if ( pv->length <= 0 ) {
    return;
  }

  if ( *preText == '\t' ) {
    printf( "%*s ", TABS, preText+1 );
  } else {
    printf( preText );
  }

  for ( i=0; i< pv->length; i++ ) {
    if ( mlPvGetCapabilities( id->value.int64,
			      pv->value.pInt64[ i ],&childCap ) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }

    printCapParamValue( childCap, ML_NAME_BYTE_ARRAY, "", "" );

    mlFreeCapabilities( childCap );

    if ( i < pv->length-1 ) {
      printf( ", " );
      if ( *preText == '\t' ) {
	printf( "\n%*s ", TABS, "" );
      }
    } else {
      printf( "\n" );
    }
  }
}


#ifdef	ML_OS_IRIX
#include <invent.h>
#include <sys/syssgi.h>

#ifndef INV_VIDEO_DVS_PCI_SD
#define INV_VIDEO_DVS_PCI_SD    15      /* DVS PCI-Video Std. Def. */
#define INV_VIDEO_DVS_PCI_HD    16      /* DVS PCI-Video High Def. */
#endif


/* ----------------------------------------------------------------display_item
 */
/* ARGSUSED */
int display_item( inventory_t *pinvent, void *ptr )
{
  switch ( pinvent->inv_class ) {
  case INV_VIDEO:
    switch ( pinvent->inv_type ) {

    case INV_VIDEO_XTDIGVID: {
      printf( "XT-DIGVID Multi-standard Digital Video: "
	      "controller %d, unit %d, version 0x%x\n", 
	      pinvent->inv_controller, pinvent->inv_unit,
	      pinvent->inv_state );
      break;
    }

    case INV_VIDEO_DVS_PCI_SD: {
      printf( "SD-DIGVID Standard Definition Digital Video: "
	      "unit %d, revision %d.%d.%d\n", 
	      pinvent->inv_unit, 
	      (pinvent->inv_state >> 16) & 0xff,
	      (pinvent->inv_state >>  8) & 0xff,
	      pinvent->inv_state        & 0xff );
      break;
    }

    default:
      fprintf( stderr, "%s: Unknown type %d class %d\n",
	       argv0, pinvent->inv_type, pinvent->inv_class );
    }
    break;

  }
  return 0;
}


/* -------------------------------------------------------------print_inventory
 */
void print_inventory( void )
{
  scaninvent( display_item, (char *) NULL );
}
#endif /* ML_OS_IRIX */


/* The following string compares compares two ML values for equality.
 * Basically, it allows you to have both "broken shift key" enum
 * labels as well as standard UPPERCASE only labels.
 *
 * This allows both the following to be treated the same:
 *
 *   ImageColorspace
 *   ML_IMAGE_COLORSPACE_INT32
 *
 *   ColorspaceCbycr_601Head
 *   ML_COLORSPACE_CBYCR_601_HEAD
 *
 * note: 'len' is length of string 'a'
 */

static const char *suffices[] = {
  "_BYTE", "_INT32", "_INT64", "_REAL32", "_REAL64",
  "_POINTER", "_ARRAY", NULL
};
static const char *string_tokens = "= \t\n";


/* -------------------------------------------------------------------stringcmp
 *
 * compare string 'a' and string 'b' for equality
 * string a is a normal null terminated string
 * string b has possible tokens that can terminate it
 * returns 1 when they compare (unlike strcmp)
 */
static int stringcmp( const char *sa, const char *sb, MLint32 len )
{
  MLint32 la = 0, lb = 0;

  if ( strncasecmp( sa, "ML_", 3 ) == 0 ) {
    sa += 3; la += 3;
  }
  if ( strncasecmp( sb, "ML_", 3 ) == 0 ) {
    sb += 3; lb += 3;
  }

  while ( len > 0 ) {
    register char ca = *sa;
    register char cb = *sb;
    if ( ca ) {
      sa++;
    }
    if ( cb ) {
      sb++;
    }

    if ( toupper( ca ) == toupper( cb ) ) {
      if ( ca == '\0' ) {
	return lb;
      }
      la++; 
      lb++;
      len--;
      continue;
    }

    if ( ca == '\0' ) {
      const char **p;
      int found = 0;
      if ( strchr( string_tokens, cb ) != NULL ) {
	return lb;
      }
      for ( p = suffices; *p; p++ ) {
	MLint32 l = (MLint32) strlen( *p );
	if ( strncasecmp( sb-1, *p, l ) == 0 ) {
	  lb += l;
	  sb += l-1;
	  found = 1;
	  break;
	}
      }
      if ( found ) {
	continue;
      }
      return 0;
    }

    if ( cb == '\0' || strchr( string_tokens, cb ) != NULL ) {
      const char **p;
      int found = 0;
      for ( p = suffices; *p; p++ ) {
	MLint32 l = (MLint32) strlen( *p );
	if ( strncasecmp( sa-1, *p, l ) == 0 ) {
	  la += l;
	  sa += l-1;
	  found = 1;
	  break;
	}
      }
      if ( found ) {
	if ( cb != '\0' ) {
	  sb--;
	}
	continue;
      }
      return 0;
    }

    if ( ca == '_' && lb > 0 ) {
      sb--;
      lb--;
      len--;
      continue;
    }

    if ( cb == '_' && la > 0 ) {
      sa--;
      la--;
      continue;
    }
    return 0;
  }
  return 1;
}


/* -------------------------------------------------------------------isCapName
 */
int isCapName( MLpv* capMsg, const char* name )
{
  MLpv* pv = mlPvFind( capMsg, ML_NAME_BYTE_ARRAY );
  MLint32 len = (MLint32) strlen( name );

  if ( pv && stringcmp( (char*) name, (char*) (pv->value.pByte), len ) ) {
    return 1;
  }
  pv = mlPvFind( capMsg, ML_DEVICE_LOCATION_BYTE_ARRAY );
  if( pv && stringcmp( (char*) name, (char*) (pv->value.pByte), len ) ) {
    return 1;
  }
  return 0;
}


/* -------------------------------------------------------------------isDevName
 */
int isDevName( MLpv* capMsg, const char* name )
{
  MLpv* pv = mlPvFind( capMsg, ML_NAME_BYTE_ARRAY );
  MLint32 len = (MLint32) strlen( name );
  const char *dn = strchr( name, ':' );

  if ( dn ) {
    MLpv* ipv = mlPvFind( capMsg, ML_DEVICE_INDEX_INT32 );
    if ( ipv && ipv->value.int32 != atoi( &dn[1] ) ) {
      return 0;
    }
    len = (MLint32) ( dn - name );
  }

  if ( pv && stringcmp( (char*) name, (char*) (pv->value.pByte), len ) ) {
    return 1;
  }
  pv = mlPvFind( capMsg, ML_DEVICE_LOCATION_BYTE_ARRAY );
  if ( pv && stringcmp( (char*) name, (char*) (pv->value.pByte), len ) ) {
    return 1;
  }
  return 0;
}


/* -------------------------------------------------------------------isUSTName
 */
int isUSTName( MLpv* capMsg, const char* name )
{
  MLpv* pv = mlPvFind( capMsg, ML_NAME_BYTE_ARRAY );
  if ( strcmp( (char*) (pv->value.pByte), name ) == 0 ) {
    return 1;
  }
  return 0;
}


#ifdef ML_OS_NT
#define ML_MAIN_DECL __cdecl
#else
#define ML_MAIN_DECL
#endif

int ML_MAIN_DECL main( int argc, char * argv[] ) {
#ifndef ML_OS_NT
  extern char *optarg;
#endif
  MLint32 i = 0;
  MLpv *sys;

  Qdetails details;

  argv0 = argv[0];

  details.mode = NONE;
  details.printSystem = 1;
  details.printDevice = 1;
  details.printJack = 0;
  details.printPath = 0;
  details.printXcode = 0;
  details.printPipe = 0;
  details.printParam = 0;
  details.printUST = 0;
  details.devName = NULL;
  details.jackName = NULL;
  details.pathName = NULL;
  details.xcodeName = NULL;
  details.pipeName = NULL;
  details.paramName = NULL;
  details.ustName = NULL;
  
  while ( ( i = getopt( argc, argv, "d:e:hij:m:p:x:v:u:" ) ) != -1 ) {
    switch ( i ) {
    case 'd':
      details.printSystem = 0;
      details.printDevice = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.devName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.devName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

#ifdef	ML_OS_IRIX
    case 'i':
      print_inventory();
      return 0;
#endif /* ML_OS_IRIX */

    case 'j':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printJack = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.jackName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.jackName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

    case 'p':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printPath = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.pathName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.pathName = NULL;
      }
      if ( details.mode == 0 ) {
	    details.mode = STANDARD;
      }
      break;

    case 'x':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printXcode = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.xcodeName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.xcodeName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

    case 'e':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printXcode  = 0;
      details.printPipe = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.pipeName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.pipeName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

    case 'v':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printJack   = 0;
      details.printPath   = 0;
      details.printXcode  = 0;
      details.printPipe   = 0;
      details.printParam  = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.paramName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.paramName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

    case 'u':
      details.printSystem = 0;
      details.printDevice = 0;
      details.printUST    = 1;
      if ( STRCASECMP( "all", optarg ) != 0 ) {
	if ( (details.ustName = strdup( optarg )) == NULL ) {
	  fprintf( stderr, "Not enough memory\n" );
	  leave( 1 );
	}
      } else {
	details.ustName = NULL;
      }
      if ( details.mode == 0 ) {
	details.mode = STANDARD;
      }
      break;

      /* The level of output details
       */
    case 'm':
      if ( STRCASECMP( "brief", optarg ) == 0 ) {	
	details.mode = BRIEF;
      } else if ( STRCASECMP( "standard", optarg ) == 0 ) {	
	details.mode = STANDARD;
      } else if ( STRCASECMP( "enhanced", optarg ) == 0 ) {	
	details.mode = ENHANCED;
      } else {
	errorMsg();
	return -1;
      }
      break;

    case 'h':
      errorMsg();
      return 0;

    default:
      errorMsg();
      return -1;
    } /* switch i */
  } /* while i=getopt() ... */

  if ( details.mode == 0 ) {
    details.mode = BRIEF;
  }

  if ( mlGetCapabilities( ML_SYSTEM_LOCALHOST, &sys ) ) {
    fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
    leave( 1 );
  }

  printf( "\n" );
  printSystem( sys, &details );

  if ( mlFreeCapabilities( sys ) != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "mlFreeCapabilities has failed\n" );
    leave( 1 );
  }

  if ( details.devName != NULL ) {
    free( details.devName ); 
  }
  if ( details.jackName != NULL ) {
    free( details.jackName );
  }
  if ( details.pathName != NULL ) {
    free( details.pathName );
  }
  if ( details.xcodeName != NULL ) {
    free( details.xcodeName );    
  }
  if ( details.pipeName != NULL ) {
    free( details.pipeName );
  }
  if ( details.paramName != NULL ) {
    free( details.paramName );    
  }

  return 0;
}


/* -----------------------------------------------------------------printSystem
 */
void printSystem( MLpv* cap, Qdetails* details )
{
  if ( details->printSystem ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tSYSTEM:", "\n" );
      printCapParamValue( cap, ML_UST_SELECTED_SOURCE_ID_INT64,
			  "\tactive UST:", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tSYSTEM:","\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:","\n" );
      printCapIdArray( cap, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY,"\tdevices:" );
      printCapParamValue( cap, ML_UST_SELECTED_SOURCE_ID_INT64,
			  "\tactive UST:", "\n" );
      printCapIdArray( cap, ML_UST_SOURCE_IDS_INT64_ARRAY,
		       "\tregistered UST:" );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "SYSTEM:" );
      mluPvPrintMsg( 0, cap );
      break;
    }
  }
  printDevices( cap, details );
  printUSTSources( cap, details );
  return;
}


/* ----------------------------------------------------------------printDevices
 */
void printDevices( MLpv* parentCap, Qdetails* details )
{
  MLpv* childIds = mlPvFind( parentCap, ML_SYSTEM_DEVICE_IDS_INT64_ARRAY );
  MLpv* childCap;
  MLint32 x;

  if ( childIds == NULL ) {
    fprintf( stderr, "unable to find system device ids\n" );
    leave( 1 );
  }

  if ( details->mode == BRIEF && details->printDevice ) {
    if ( childIds->length == 0 ) {
      printf( "\n%*s\n", TABS, "(no devices installed on this system)\n" );
    } else if ( childIds->length == 1 ) {
      printf( "\n%*s\n", TABS, "DEVICE:" );
    } else {
      printf( "\n%*s\n", TABS, "DEVICES:" );
    }
  }

  for ( x = 0; x < childIds->length; x++ ) {
    if ( mlGetCapabilities( childIds->value.pInt64[ x ],&childCap ) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }

    printDevice( childCap, details );

    mlFreeCapabilities( childCap );
  }
  return;
}


/* -----------------------------------------------------------------printDevice
 */
void printDevice( MLpv* cap, Qdetails* details )
{
  if ( details->printDevice && (details->devName == NULL ||
				isDevName( cap, details->devName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t", "" );
      printCapParamValue( cap, ML_DEVICE_INDEX_INT32, ":", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tDEVICE:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY,
			  "\tdescription:","\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      printCapParamValue( cap, ML_DEVICE_INDEX_INT32, "\tindex:", "\n" );
      printCapParamValue4( cap, ML_DEVICE_VERSION_INT32, "\tversion:", "\n" );
      printCapParamValue( cap, ML_DEVICE_LOCATION_BYTE_ARRAY, 
			  "\tlocation:", "\n" );
      printCapIdArray( cap, ML_DEVICE_JACK_IDS_INT64_ARRAY, "\tjacks:" );
      printCapIdArray( cap, ML_DEVICE_PATH_IDS_INT64_ARRAY, "\tpaths:" );
      printCapIdArray( cap, ML_DEVICE_XCODE_IDS_INT64_ARRAY, "\txcodes:" );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "DEVICE:\n" );
      mluPvPrintMsg( 0, cap );
      printf( "\n" );
      break;    
    }
  }

  if ( (details->devName == NULL || isDevName( cap, details->devName )) ) {
    printJacks( cap, details );
    printPaths( cap, details );
    printXcodes( cap, details );
  }
}


/* ------------------------------------------------------------------printJacks
 */
void printJacks( MLpv* parentCap, Qdetails* details )
{
  MLpv* childIds = mlPvFind( parentCap, ML_DEVICE_JACK_IDS_INT64_ARRAY );
  MLpv* childCap;
  MLint32 x;

  if ( childIds != NULL ) {
    for ( x = 0; x < childIds->length; x++ ) {
      if ( mlGetCapabilities( childIds->value.pInt64[ x ],&childCap ) ) {
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }
      printJack( childCap, details );
      mlFreeCapabilities( childCap );
    }
  }
  return;
}


/* -------------------------------------------------------------------printJack
 */
void printJack( MLpv* cap, Qdetails* details )
{
  if ( details->printJack && (details->jackName == NULL ||
			      isCapName( cap, details->jackName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t","\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tJACK:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );

      {
	MLpv* direction = mlPvFind( cap, ML_JACK_DIRECTION_INT32 );
	printf( "%*s", TABS, "direction:" );
	if ( direction->value.int32 == ML_DIRECTION_IN ) {
	  printf( " input\n" );
	} else {
	  printf( " output\n" );
	}
      }
      {
	MLpv* type = mlPvFind( cap, ML_JACK_TYPE_INT32 );
	printf( "%*s", TABS, "type:" );
	switch ( type->value.int32 ) {
	case ML_JACK_TYPE_COMPOSITE:
	  printf( " Composite\n" );
	  break;
	case ML_JACK_TYPE_SVIDEO:
	  printf( " S-video\n" );
	  break;
	case ML_JACK_TYPE_SDI:
	  printf( " SDI\n" );
	  break;
	case ML_JACK_TYPE_GENLOCK:
	  printf( " Genlock\n" );
	  break;
	case ML_JACK_TYPE_GPI:
	  printf( " Gpi\n" );
	  break;
	case ML_JACK_TYPE_SERIAL:
	  printf( " Serial\n" );
	  break;
	case ML_JACK_TYPE_ANALOG_AUDIO:
	  printf( " Analog Audio\n" );
	  break;
	case ML_JACK_TYPE_AES:
	  printf( " AES Digital Audio\n" );
	  break;
	case ML_JACK_TYPE_ADAT:
	  printf( " ADAT Digital Audio\n" );
	  break;
	case ML_JACK_TYPE_GFX:
	  printf( " Graphics\n" );
	  break; 
	case ML_JACK_TYPE_AUX:
	  printf( " Auxiliary\n" );
	  break;
	case ML_JACK_TYPE_VIDEO:
	  printf( " Video\n" );
	  break;
	case ML_JACK_TYPE_AUDIO:
	  printf( " Audio\n" );
	  break;
	default:
	  printf( " Unknown\n" );
	  fprintf( stderr, "unknown jack type 0x%x\n", type->value.int32 );
	}
      }
      printCapParamIdArray( cap, "\tparams:", ML_PARAM_IDS_INT64_ARRAY );
      printCapParamIdArray( cap, "\topen options:",
			    ML_OPEN_OPTION_IDS_INT64_ARRAY );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "JACK:" );
      mluPvPrintMsg( 0, cap );
      break;  
    }
  }
}


/* ------------------------------------------------------------------printPaths
 */
void printPaths( MLpv* parentCap, Qdetails* details )
{
  MLpv* childIds = mlPvFind( parentCap, ML_DEVICE_PATH_IDS_INT64_ARRAY );
  MLpv* childCap;
  MLint32 x;

  if ( childIds != NULL ) {
    for ( x = 0; x < childIds->length; x++ ) {
      if ( mlGetCapabilities( childIds->value.pInt64[ x ],&childCap ) ) {
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }
      printPath( childCap, details );
      mlFreeCapabilities( childCap );
    }
  }
  return;
}


/* -------------------------------------------------------------------printPath
 */
void printPath( MLpv* cap, Qdetails* details )
{
  if ( details->printPath && (details->pathName == NULL ||
			      isCapName( cap, details->pathName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tPATH:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      {
	MLpv* type = mlPvFind( cap, ML_PATH_TYPE_INT32 );
	printf( "%*s", TABS, "type:" );
	if ( type->value.int32 == ML_PATH_TYPE_DEV_TO_MEM ) {
	  printf( " input to memory\n" );
	} else {
	  printf( " memory to output\n" );
	}
      }
      printCapParamIdArray( cap, "\tparams:", ML_PARAM_IDS_INT64_ARRAY );
      printCapParamIdArray( cap, "\topen options:",
			    ML_OPEN_OPTION_IDS_INT64_ARRAY );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "PATH:" );
      mluPvPrintMsg( 0, cap );
      break;    
    }
  }
  if ( details->pathName == NULL || isCapName( cap, details->pathName ) ) {
    printParams( cap, details );
  }
}


/* -----------------------------------------------------------------printXcodes
 */
void printXcodes( MLpv* parentCap, Qdetails* details )
{
  MLpv* childIds = mlPvFind( parentCap, ML_DEVICE_XCODE_IDS_INT64_ARRAY );
  MLpv* childCap;
  MLint32 x;

  if ( childIds != NULL ) {
    for ( x = 0; x < childIds->length; x++ ) {
      if ( mlGetCapabilities( childIds->value.pInt64[ x ],&childCap ) ) {
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }
      printXcode( childCap, details );
      mlFreeCapabilities( childCap );
    }
  }
  return;
}


/* ------------------------------------------------------------------printXcode
 */
void printXcode( MLpv* cap, Qdetails* details )
{
  if ( details->printXcode && (details->xcodeName == NULL ||
			       isCapName( cap, details->xcodeName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tXCODE:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      printCapIdArray( cap, ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY,
		       "\tsrc pipe(s):" );
      printCapIdArray( cap, ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY,
		       "\tdst pipe(s):" );
      printCapParamIdArray( cap, "\tparams:", ML_PARAM_IDS_INT64_ARRAY );
      printCapParamIdArray( cap, "\topen options:",
			    ML_OPEN_OPTION_IDS_INT64_ARRAY );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "XCODE:" );
      mluPvPrintMsg( 0, cap );
      break;    
    }
  }
  if ( details->xcodeName == NULL || isCapName( cap, details->xcodeName ) ) {
    printParams( cap, details );
    printPipes( cap, details );
  }
}


/* ------------------------------------------------------------------printPipes
 */
void printPipes( MLpv* xcodeEngineCap, Qdetails* details )
{
  MLpv* xcodeSrcPipeIds =
    mlPvFind( xcodeEngineCap, ML_XCODE_SRC_PIPE_IDS_INT64_ARRAY );
  MLpv* xcodeDstPipeIds =
    mlPvFind( xcodeEngineCap, ML_XCODE_DEST_PIPE_IDS_INT64_ARRAY );
  MLpv* xcodePipeCap;
  MLint32 x;

  if ( xcodeSrcPipeIds == NULL && xcodeDstPipeIds == NULL ) {
    fprintf( stderr, "mlquery error: mlGetCap on device lacks xcode ids?\n" );
    leave( 1 );
  }

  if ( xcodeSrcPipeIds ) {
    for ( x = 0; x < xcodeSrcPipeIds->length; x++ ) {
      if ( mlGetCapabilities( xcodeSrcPipeIds->value.pInt64[ x ],
			      &xcodePipeCap ) )	{
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }
      printPipe( xcodePipeCap, details );
      mlFreeCapabilities( xcodePipeCap );
    }
  }

  if ( xcodeDstPipeIds ) {
    for ( x = 0; x < xcodeDstPipeIds->length; x++ ) {
      if ( mlGetCapabilities( xcodeDstPipeIds->value.pInt64[ x ],
			      &xcodePipeCap ) ) {
	fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
	leave( 1 );
      }
      printPipe( xcodePipeCap, details );
      mlFreeCapabilities( xcodePipeCap );
    }
  }
}


/* -------------------------------------------------------------------printPipe
 */
void printPipe( MLpv* cap, Qdetails* details )
{
  if ( details->printPipe && (details->pipeName == NULL ||
			      isCapName( cap, details->pipeName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tPIPE:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      printCapParamIdArray( cap, "\tparams:", ML_PARAM_IDS_INT64_ARRAY );
      printCapParamIdArray( cap, "\topen options:",
			    ML_OPEN_OPTION_IDS_INT64_ARRAY );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "PIPE:" );
      mluPvPrintMsg( 0, cap );
      break;    
    }
  }
  if ( details->pipeName == NULL || isCapName( cap, details->pipeName ) ) {
    printParams( cap, details );
  }
}


/* -----------------------------------------------------------------printParams
 */
void printParams( MLpv* parentCap, Qdetails* details )
{
  MLpv* parentId = mlPvFind( parentCap, ML_ID_INT64 );
  MLpv* childIds;
  MLpv* childCap;
  MLint32 x;
  MLint64 pts[2] = { ML_PARAM_IDS_INT64_ARRAY, ML_OPEN_OPTION_IDS_INT64_ARRAY};
  int pt;

  for ( pt = 0; pt < 2; pt++ ) {
    childIds = mlPvFind( parentCap, pts[pt] );
    if ( parentId == NULL || childIds == NULL ) {
      return;
    }

    for ( x = 0; x < childIds->length; x++ ) {
      if ( mlPvGetCapabilities( parentId->value.int64,
				childIds->value.pInt64[ x ],
				&childCap) ) {
	fprintf( stderr, "mlPvGetCapabilities has failed\n" );
	leave( 1 );
      }
      printParam( childCap, details );
      mlFreeCapabilities( childCap );
    }
  }
  return;
}


/* ------------------------------------------------------------------printParam
 */
void printParam( MLpv* cap, Qdetails* details )
{
  if ( details->printParam && (details->paramName == NULL ||
			       isCapName( cap, details->paramName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tPARAM:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      {
	MLpv* id = mlPvFind( cap, ML_ID_INT64 );
	printf( "%*s", TABS, "type:" );
	switch ( ML_PARAM_GET_TYPE_ELEM( id->value.int64 ) ) {
	case ML_TYPE_ELEM_BYTE:
	  printf( " byte" );
	  break;
	case ML_TYPE_ELEM_INT32:
	  printf( " int32" );
	  break;
	case ML_TYPE_ELEM_INT64:
	  printf( " int64" );
	  break;
	case ML_TYPE_ELEM_REAL32:
	  printf( " real32" );
	  break;
	case ML_TYPE_ELEM_REAL64:
	  printf( " real64" );
	  break;
	default:
	  printf( " unknown" );
	  break;
	}

	switch ( ML_PARAM_GET_TYPE_VAL( id->value.int64 ) ) {
	case ML_TYPE_VAL_SCALAR:
	  break;
	case ML_TYPE_VAL_ARRAY:
	  printf( " array" );
	  break;
	case ML_TYPE_VAL_POINTER:
	  printf( " pointer" );
	  break;
	}
	printf( "\n" );

	switch ( ML_PARAM_GET_TYPE( id->value.int64 ) ) {
	case ML_TYPE_INT32:
	case ML_TYPE_INT32_ARRAY: {
	  MLpv* enumv = mlPvFind( cap, ML_PARAM_ENUM_VALUES_INT32_ARRAY );
	  MLpv* enumn = mlPvFind( cap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );
	  if ( enumv != NULL && enumv->length > 0 ) {
	    if ( enumn != NULL ) {
	      char* ename = (char*) enumn->value.pByte;
	      int i;
	      for ( i=0; i< enumv->length; i++ ) {
		if ( i == 0 ) {
		  printf( "%*s %s", TABS, "enum:", ename );
		} else {
		  printf( "%*s %s", TABS, "", ename );
		}
		if ( i != enumv->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
		ename += strlen( ename )+1;
		if ( i < enumv->length-1 && (ename == NULL || *ename == '\0')){
		  fprintf( stderr, "[mlquery] enum name/value "
			   "insufficient names\n" );
		  leave( 1 );
		}
	      }

	    } else {
	      int i;
	      for ( i=0; i< enumv->length; i++ ) {
		if ( i== 0 ) {
		  printf( "%*s %d (0x%x)", TABS, "enum:", 
			  enumv->value.pInt32[i], enumv->value.pInt32[i] );
		} else {
		  printf( "%*s %d (0x%x)", TABS, "", 
			  enumv->value.pInt32[i], enumv->value.pInt32[i] );
		}
		if ( i != enumn->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
	      }
	    }
	  } /* if enumv != NULL && ... */
	}
	  {
	    MLpv* mins = mlPvFind( cap, ML_PARAM_MINS_INT32_ARRAY );
	    MLpv* maxs = mlPvFind( cap, ML_PARAM_MAXS_INT32_ARRAY );
	    if ( mins != NULL && mins->length > 0 ) {
	      int i;
	      if ( mins->length != maxs->length ) {
		fprintf( stderr, "[mlquery] min/max mismatch\n" );
		leave( 1 );
	      }

	      for ( i=0; i< mins->length; i++ ) {
		if ( i==0 ) {
		  printf( "%*s", TABS, "min-max:" );
		} else {
		  printf( "%*s", TABS, "" );
		}
		printf( " [%d - %d]", mins->value.pInt32[i],
			maxs->value.pInt32[i] );
		if ( i != mins->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
	      }
	    }
	  }
	  {
	    MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_INT32 );
	    if ( inc != NULL && inc->length == 1 ) {
	      printf( "%*s %d\n", TABS, "increment:", inc->value.int32 );
	    }
	  }
	  {
	    MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_INT32_ARRAY );
	    if ( inc != NULL && inc->length > 0 ) {
	      int i;
	      for ( i=0; i < inc->length; i++ ) {
		if ( i==0 ) {
		  printf( "%*s", TABS, "increment:" );
		} else {
		  printf( "%*s", TABS, "" );
		}
		printf( " [%d]", inc->value.pInt32[i] );
		if ( i != inc->length -1 ) {
		  printf(",\n");
		} else {
		  printf("\n");
		}
	      }
	    }
	  }
	break;

	case ML_TYPE_INT64:
	case ML_TYPE_INT64_ARRAY: {
	  MLpv* enumv = mlPvFind( cap, ML_PARAM_ENUM_VALUES_INT64_ARRAY );
	  MLpv* enumn = mlPvFind( cap, ML_PARAM_ENUM_NAMES_BYTE_ARRAY );
	  if ( enumv != NULL && enumv->length > 0 ) {
	    if ( enumn != NULL ) {
	      char* ename = (char*) enumn->value.pByte;
	      int i;
	      for ( i=0; i < enumv->length; i++ ) {
		if ( i==0 ) {
		  printf( "%*s %s", TABS, "enum:", ename );
		} else {
		  printf( "%*s %s", TABS, "", ename );
		}
		if ( i != enumv->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
		ename += strlen( ename )+1;
		if ( i < enumv->length-1 && (ename == NULL || *ename == '\0')){
		  fprintf( stderr,
			   "[mlquery] enum name/value insufficient names\n" );
		  leave( 1 );
		}
	      }

	    } else {
	      int i;
	      for ( i=0; i < enumv->length; i++ ) {
		if ( i== 0 ) {
		  printf( "%*s (0x%" FORMAT_LLX ")", TABS,
			  "enum:", enumv->value.pInt64[i] );
		} else {
		  printf( "%*s (0x%" FORMAT_LLX ")", TABS, "", 
			  enumv->value.pInt64[i] );
		}
		if ( i != enumn->length -1 ) {
		  printf(",\n");
		} else {
		  printf( "\n" );
		}
	      }
	    }
	  }
	}
	  {
	    MLpv* mins = mlPvFind( cap, ML_PARAM_MINS_INT64_ARRAY );
	    MLpv* maxs = mlPvFind( cap, ML_PARAM_MAXS_INT64_ARRAY );
	    if ( mins != NULL && mins->length > 0 ) {
	      int i;
	      if ( mins->length != maxs->length ) {
		fprintf( stderr, "[mlquery] min/max mismatch\n" );
		leave( 1 );
	      }

	      for ( i=0; i < mins->length; i++ ) {
		if ( i==0 ) {
		  printf( "%*s ", TABS, "min-max:" );
		} else {
		  printf( "%*s", TABS, "" );
		}
		printf( "[%" FORMAT_LLD " - %" FORMAT_LLD "]",
			mins->value.pInt64[i], maxs->value.pInt64[i] );
		if ( i!= mins->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
	      }
	    }
	  }
	{
	  MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_INT64 );
	  if ( inc != NULL && inc->length == 1 ) {
	    printf( "%*s %" FORMAT_LLD "", TABS, "increment:",
		    inc->value.int64 );
	  }
	}
	  {
	    MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_INT64_ARRAY );
	    if ( inc != NULL && inc->length > 0 ) {
	      int i;
	      for ( i=0; i < inc->length; i++ ) {
		if ( i==0 ) {
		  printf( "%*s", TABS, "increment:" );
		} else {
		  printf( "%*s", TABS, "" );
		}
		printf( " [%" FORMAT_LLD "]", inc->value.pInt64[i] );
		if ( i != inc->length -1 ) {
		  printf( ",\n" );
		} else {
		  printf( "\n" );
		}
	      }
	    }
	  }
	break;

	case ML_TYPE_REAL32: {
	  MLpv* mins = mlPvFind( cap, ML_PARAM_MINS_REAL32_ARRAY );
	  MLpv* maxs = mlPvFind( cap, ML_PARAM_MAXS_REAL32_ARRAY );

	  if ( mins != NULL && mins->length > 0 ) {
	    int i;
	    if ( mins->length != maxs->length ) {
	      fprintf( stderr, "[mlquery] min/max mismatch\n" );
	      leave( 1 );
	    }

	    for ( i=0; i < mins->length; i++ ) {
	      if ( i==0 ) {
		printf( "%*s", TABS, "ranges:" );
	      } else {
		printf( "%*s", TABS, "" );
	      }
	      printf( " [%f-%f]", (double) (mins->value.pReal32[i]),
		      (double) (maxs->value.pReal32[i]) );
	      if ( i != mins->length -1 ) {
		printf( ",\n" );
	      } else {
		printf( "\n" );
	      }
	    }
	  }
	}
	  {
	    MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_REAL32 );
	    if ( inc != NULL && inc->length == 1 ) {
	      printf( "%*s %f", TABS, "increment:", inc->value.real32 );
	    }
	  }
	break;

	case ML_TYPE_REAL64: {
	  MLpv* mins = mlPvFind( cap, ML_PARAM_MINS_REAL64_ARRAY );
	  MLpv* maxs = mlPvFind( cap, ML_PARAM_MAXS_REAL64_ARRAY );

	  if ( mins != NULL && mins->length > 0 ) {
	    int i;
	    if ( mins->length != maxs->length ) {
	      fprintf( stderr, "[mlquery] min/max mismatch\n" );
	      leave( 1 );
	    }

	    for ( i=0; i < mins->length; i++ ) {
	      if ( i==0 ) {
		printf( "%*s", TABS, "ranges:" );
	      } else {
		printf( "%*s", TABS, "" );
	      }
	      printf( " [%f-%f]", mins->value.pReal64[i],
		      maxs->value.pReal64[i] );
	      if ( i != mins->length -1 ) {
		printf( ",\n" );
	      } else {
		printf( "\n" );
	      }
	    }
	  }
	}
	  {
	    MLpv* inc = mlPvFind( cap, ML_PARAM_INCREMENT_REAL64 );
	    if ( inc != NULL && inc->length == 1 ) {
	      printf( "%*s %f", TABS, "increment:", inc->value.real64 );
	    }
	  }
	break;

	default:
	  break;
	} /* switch */

	{
	  MLpv* access = mlPvFind( cap, ML_PARAM_ACCESS_INT32 );
	  if ( access != NULL && access->length == 1 ) {
	    int x = 0;
	    printf( "%*s ", TABS, "usage:" );
	    if ( access->value.int32 & ML_ACCESS_OPEN_OPTION ) {
	      printf( "open option " );
	    }
	    if ( access->value.int32 & ML_ACCESS_IMMEDIATE ) {
	      switch ( access->value.int32 & ML_ACCESS_RW ) {
	      case ML_ACCESS_READ:
		printf( "getControls" );
		x = 1;
		break;

	      case ML_ACCESS_WRITE:
		printf( "setControls" );
		x = 1;
		break;

	      case ML_ACCESS_RW:
		printf( "get/setControls" );
		x = 1;
		break;
	      }
	    }

	    if ( (access->value.int32 & ML_ACCESS_RW) &&
		 ! (access->value.int32 & ML_ACCESS_DURING_TRANSFER) ) {
	      printf( " (but not during a transfer)" );
	    }

	    if ( access->value.int32 & ML_ACCESS_QUEUED ) {
	      printf( "\n%*s %s", TABS, "", "sendControls " );
	    }

	    if ( access->value.int32 & ML_ACCESS_SEND_BUFFER ) {
	      if ( x ) {
		printf( "\n%*s %s", TABS, "", "sendBuffers " );
	      } else {
		printf( "sendBuffers " );
	      }
	    }
	    printf( "\n" );
	  }
	}
      }
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "PARAM:" );
      mluPvPrintMsg( 0, cap );
      printf( "\n" );
      break;    
    }
  }
}


/* -------------------------------------------------------------printUSTSources
 */
void printUSTSources( MLpv* parentCap, Qdetails* details )
{
  MLpv* childIds = mlPvFind( parentCap, ML_UST_SOURCE_IDS_INT64_ARRAY );
  MLpv* childCap;
  MLint32 x;

  if ( childIds == NULL ) {
    fprintf( stderr, "unable to find UST source ids\n" );
    leave( 1 );
  }

  if ( details->printUST && childIds->length == 0 ) {
    printf( "\n%*s\n", TABS, "(no UST sources registered on this system)\n" );
  }

  if ( details->mode == BRIEF && details->printUST ) {
    if ( childIds->length > 1 ) {
      printf( "\n%*s\n", TABS, "UST SOURCES:" );
    } else if ( childIds->length == 1 ) {
      printf( "\n%*s\n", TABS, "UST SOURCE:" );
    }
  }

  for ( x = 0; x < childIds->length; x++ ) {
    if ( mlGetCapabilities( childIds->value.pInt64[ x ],&childCap ) ) {
      fprintf( stderr, "mlquery error: mlGetCapabilities has failed\n" );
      leave( 1 );
    }
    printUSTSource( childCap, details );
    mlFreeCapabilities( childCap );
  }
  return;
}


/* --------------------------------------------------------------printUSTSource
 */
void printUSTSource( MLpv* cap, Qdetails* details )
{
  if ( details->printUST && (details->ustName == NULL ||
			     isUSTName( cap, details->ustName )) ) {
    switch ( details->mode ) {
    case NONE:
      break;

    case BRIEF:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\t", "\n" );
      break;

    case STANDARD:
      printCapParamValue( cap, ML_NAME_BYTE_ARRAY, "\tUST SOURCE:", "\n" );
      printCapParamValue( cap, ML_DESCRIPTION_BYTE_ARRAY, 
			  "\tdescription:", "\n" );
      printCapParamValue( cap, ML_PARENT_ID_INT64, "\tparent:", "\n" );
      printCapParamValue( cap, ML_UST_SOURCE_UPDATE_PERIOD_INT32,
			  "\tupdate period:", " ns\n" );
      printCapParamValue( cap, ML_UST_SOURCE_LATENCY_VARIATION_INT32,
			  "\tlatency var.:", " ns\n" );
      printf( "\n" );
      break;

    case ENHANCED:
      printf( "UST SOURCE:\n" );
      mluPvPrintMsg( 0, cap );
      printf( "\n" );
      break;    
    }
  }
}


/* --------------------------------------------------------------------errorMsg
 */
void errorMsg( void )
{
  fprintf(stderr, 
	  "usage: mlquery: [-d name] [-j name] [-p name] [-x name] [-e name]\n"
	  "                [-v name] [-u name] [-m brief|standard|enhanced]"
	  " [-h]\n\n"
	  "\t-d deviceName - select a particular device (\"all\" for all)\n"
	  "\t-j jackName   - select a particular jack (\"all\" for all)\n"
	  "\t-p pathName   - select a particular path (\"all\" for all)\n"
	  "\t-x xcodeName  - select a particular xcode (\"all\" for all)\n"
	  "\t-e pipeName   - select a particular pipe (\"all\" for all)\n"
	  "\t-v paramName  - select a particular parameter (\"all\" for all)\n"
	  "\t-u USTName    - select a particular UST source (\"all\" for all)"
	  "\n\n"
	  "\t-m brief      - print brief descriptions\n"
	  "\t-m standard   - standard descriptions\n"
	  "\t-m enhanced   - detailed capability descriptions\n"
#ifdef	ML_OS_IRIX
	  "\t-i            - print (hardware) inventory\n"
#endif
	  "\t-h            - print this message\n" );
}

