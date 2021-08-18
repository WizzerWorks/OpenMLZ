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

/****************************************************************************
 *
 * Sample ML controls query & set application
 * 
 ****************************************************************************/

#include <ML/ml.h>
#include <ML/mlu.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#ifdef	ML_OS_NT
#include <ML/getopt.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "utils.h"

int debug = 0;
char *Usage =	"\n"
"usage: %s <-d device> <-j jack> [-r file] [-w file] [-gD] "
"[control] [control=value] ...\n"
"where:\n"
"\t-d <device name>\n"
"\t-j <jack name>\n"
"\t-r <file>        read control settings from <file>\n"
"\t-w <file>        write control settings to <file>\n"
"\t-g               get all control values after the set operation\n"
"\t-D               turn on debugging\n"
"\tcontrol          control name:  run mlquery -d device -j jack\n"
"\tvalue            control value: run mlquery -d device -j jack -v control\n"
"\t                 (control by itself displays that control value)\n"
"\t                 (control=value sets control to that value)\n\n"
"\tfor jack name: run mlquery to find device names\n"
"\tthen run mlquery -d device to get jack names\n"
"\tdevice name syntax is: device[:unit]\n"
"\tjack name syntax is:  [device[:unit].]jack\n\n"
;


int print_controls( MLint64 openPath, FILE *f );
void flush_cmds( MLopenid openPath, char* rw_mode, char **cmds, int count );


/*-------------------------------------------------------------------------main
 */
int main( int argc, char **argv )
{
  char*	devName = NULL;
  char*	jackName = NULL;
  char*	readFile = NULL;
  char*	writeFile = NULL;
  int c;
  int get = 0;
  MLint64 devId=0;
  MLint64 jackId=0;
  MLint64 pathId=0;
  MLopenid openPath;
  MLstatus status = ML_STATUS_NO_ERROR;

  /* Get command line args --- */
  while ( (c = getopt( argc, argv, "Dd:ghj:r:w:" )) != EOF ) {
    switch ( c ) {
    case 'D':
      debug++;
      break;

    case 'd':
      devName = optarg;
      break;

    case 'g':
      get++;
      break;

    case 'j':
      jackName = optarg;
      break;

    case 'r':
      readFile = optarg;
      break;

    case 'w':
      writeFile = optarg;
      break;

    case 'h':
    default:
      fprintf( stderr, Usage, argv[0] );
      exit( 1 );
    } /* switch */
  } /* while c = getopt()... */

  if ( !devName && !jackName ) {
    fprintf( stderr, "Need at least a device or jack name!\n" );
    fprintf( stderr, Usage, argv[0] );
    exit( 1 );
  }

  if ( devName ) {
    status = mluFindDeviceByName( ML_SYSTEM_LOCALHOST, devName, &devId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find device '%s': %s\n", devName,
	       mlStatusName( status ) );
      exit( 1 );
    }
  }

  if ( jackName ) {
    status = mluFindJackByName( devId, jackName, &jackId );
    if ( status != ML_STATUS_NO_ERROR ) {
      fprintf( stderr, "Cannot find jack '%s': %s\n", jackName,
	       mlStatusName( status ) );
      exit( 1 );
    }

  } else if ( mluFindFirstInputJack( devId, &jackId ) &&
	      mluFindFirstOutputJack( devId, &jackId ) ) {
    fprintf( stderr, "Cannot find an input or output jack on %s\n",
	     devName? devName : "this system" );
    exit( 1 );
  }

  if ( debug ) {
    fprintf( stderr, " devId 0x%" FORMAT_LLX "\njackId 0x%" FORMAT_LLX "\n",
	     devId, jackId );
  }

  if ( mluFindPathFromJack( jackId, &pathId, NULL ) &&
       mluFindPathToJack(   jackId, &pathId, NULL )) {
    fprintf( stderr, "Cannot find a path to/from %s\n", jackName );
    exit( 1 );
  }

  status = mlOpen( pathId, NULL, &openPath );
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Cannot open path: %s\n", mlStatusName( status ) );
    exit( 1 );
  }

  { /* local scope */
    char *read_cmds[ 300 ];
    char *write_cmds[ 300 ];
    int read_cmd_index = 0;
    int write_cmd_index = 0;

    if ( readFile ) {
      FILE *f = fopen( readFile, "ro" );
      if ( f ) {
	char line[ 200 ];
	if ( debug ) {
	  fprintf( stderr, "File %s Controls\n", readFile );
	}

	while ( fgets( line, sizeof( line ), f ) != 0 ) {
	  if ( debug ) {
	    fprintf( stderr, "%s", line );
	  }
	  if ( (write_cmds[ write_cmd_index++ ] = strdup( line )) == NULL ) {
	    fprintf( stderr, "strdup error\n" );
	    exit( 2 );
	  }
	}
	fclose( f );

      } else {
	fprintf( stderr, "can't open %s\n", readFile );
      }
    } /* if readFile */

    while ( optind < argc ) {
      char *p = argv[ optind++ ];

      if ( strchr( p, '=' ) ) {
	/* Write (put) value */
	if ( read_cmd_index ) {
	  flush_cmds( openPath, "read", read_cmds, read_cmd_index );
	  read_cmd_index = 0;
	}

	if ( (write_cmds[ write_cmd_index++ ] = strdup( p )) == NULL ) {
	  fprintf( stderr, "strdup error\n" );
	  exit( 2 );
	}

      } else {
	/* Read (get) value */
	if ( write_cmd_index ) {
	  flush_cmds( openPath, "write", write_cmds, write_cmd_index );
	  write_cmd_index = 0;
	}

	if ( (read_cmds[ read_cmd_index++ ] = strdup( p )) == NULL ) {
	  fprintf( stderr, "strdup error\n" );
	  exit( 2 );
	}
      }
    } /* while optind < argc */

    /* Flush remaining controls */
    if ( write_cmd_index ) {
      flush_cmds( openPath, "write", write_cmds, write_cmd_index );
    }
    if ( read_cmd_index ) {
      flush_cmds( openPath, "read", read_cmds, read_cmd_index );
    }

    /* Display or write control values to a file? */
    if ( get ) {
      print_controls( openPath, stdout );
    }
    if ( writeFile ) {
      FILE *f = fopen( writeFile, "wo" );
      if ( f ) {
	print_controls( openPath, f );
	if ( debug ) {
	  fprintf( stderr, "Controls written to %s\n", writeFile );
	}
	fclose( f );

      } else {
	fprintf( stderr, "can't open %s\n", writeFile );
      }
    }
  } /* local scope */

  return 0;
}


/*-------------------------------------------------------------------flush_cmds
 */
void flush_cmds( MLopenid openPath, char * rw_mode, char **cmds, int count )
{
  MLpv msg[ 300 ], *mp = msg;
  int i = 0;

  while ( i < count ) {
    char *p = cmds[ i++ ];
    MLint32 size = (MLint32) strlen( p );
    MLint64 data[ 512 ];

    if ( debug > 1 ) {
      fprintf( stderr, "process %s\n", p );
    }

    mlPvFromString( openPath, p, &size, mp++, (MLbyte*)data, sizeof( data ) );
  } /* while i < count */

  mp->param = ML_END;
  if ( strcmp( rw_mode, "write" ) == 0 ) {
    MLstatus status = mlSetControls( openPath, msg );
    if (  status != ML_STATUS_NO_ERROR || debug ) {
      fprintf( stdout, "Set Controls: %s\n", mlStatusName( status ) );
      printParams( openPath, msg, stdout );
    }

  } else {
    mlGetControls( openPath, msg );
    fprintf( stdout, "Get Controls:\n" );
    printParams( openPath, msg, stdout );
  }
}


/*---------------------------------------------------------------print_controls
 */
int print_controls( MLint64 openPath, FILE *f )
{
  /* First make a list of all the parameters we need to write */
  MLpv* devCap;
  MLpv* paramIds;
  char paramBuffer[60], valueBuffer[60];
  int i;
  void* array = 0;
  MLstatus status = ML_STATUS_NO_ERROR;

  status = mlGetCapabilities(openPath, &devCap);
  if ( status != ML_STATUS_NO_ERROR ) {
    fprintf( stderr, "Unable to get device capabilities: %s\n",
	     mlStatusName( status ) );
    return -1;
  }

  paramIds = mlPvFind( devCap, ML_PARAM_IDS_INT64_ARRAY );
  if( paramIds == NULL ) {
    fprintf( stderr, "Unable to find param id list\n");
    return -1;
  }

  for ( i=0; i < paramIds->length; i++ ) {
    MLint32 class = ML_PARAM_GET_CLASS( paramIds->value.pInt64[i] );

    if( class == ML_CLASS_VIDEO || class == ML_CLASS_IMAGE ||
	class == ML_CLASS_AUDIO ) {
      int vs, ps;
      MLpv* paramCap;
      MLpv controls[2];

      if( paramIds->value.pInt64[i] == ML_VIDEO_UST_INT64 ||
	  paramIds->value.pInt64[i] == ML_VIDEO_MSC_INT64 ||
	  paramIds->value.pInt64[i] == ML_VIDEO_ASC_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_UST_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_MSC_INT64 ||
	  paramIds->value.pInt64[i] == ML_AUDIO_ASC_INT64 ) {

	continue;
      }

      controls[0].param = paramIds->value.pInt64[i];
      /* Init all fields of the param/value pair, to help debugging.
       */
      controls[0].value.pByte = 0;
      controls[0].length = 0;
      controls[0].maxLength = 0;
      controls[1].param = ML_END;

      /* If we're writing to a file, we should only save those params
       * that can actually be set...
       */
      if ( f != stdout ) {
	status = mlPvGetCapabilities( openPath, controls[0].param, &paramCap );
	if ( status != ML_STATUS_NO_ERROR ) {
	  fprintf( stderr, "Unable to get param 0x%" FORMAT_LLX
		   " capabilities: %s\n", controls[0].param,
		   mlStatusName( status ) );
	  return -1;
	}

	{
	  MLpv *pv = mlPvFind( paramCap, ML_PARAM_ACCESS_INT32 );
	  if( !pv || !( pv->value.int32 & ML_ACCESS_W )) {
	    continue;
	  }
	}
      } /* if f != stdout */

      status = mlGetControls( openPath, controls );
      if ( status != ML_STATUS_NO_ERROR ) {
	if ( debug ) {
	  char pname[64] = "";
	  int size = sizeof( pname );
	  mlPvParamToString( openPath, controls, pname, &size );
	  fprintf( stderr, "Unable to get param %s: %s\n",
		   pname, mlStatusName( status ) );
	  continue;
	}
      }

      /* If the param is an array, we didn't actually get the result
       * -- we didn't supply an array to write the results (0 pointer,
       * and maxLength = 0). A well-behaved module will have set
       * maxLength to the size of the array required for the full
       * result, so we can allocate that and try again...
       */
      if ( (controls[0].param & ML_TYPE_VAL_ARRAY) != 0 ) {
	if ( debug ) {
	  fprintf( stdout, "(parameter is an array)\n" );
	}
	if ( controls[0].maxLength > 0 ) {
	  /* Figure out size of array elements
	   */
	  int size = 0;

	  switch ( ML_PARAM_GET_TYPE_ELEM( controls[0].param ) ) {
	  case ML_TYPE_ELEM_BYTE:
	    size = sizeof( MLbyte );
	    break;

	  case ML_TYPE_ELEM_INT32:
	    size = sizeof( MLint32 );
	    break;

	  case ML_TYPE_ELEM_INT64:
	    size = sizeof( MLint64 );
	    break;

	  case ML_TYPE_ELEM_REAL32:
	    size = sizeof( MLreal32 );
	    break;

	  case ML_TYPE_ELEM_REAL64:
	    size = sizeof( MLreal64 );
	    break;

	  default:
	    /* don't know how to deal with these, leave size at 0
	     */
	    break;
	  } /* switch */

	  if ( size > 0 ) {
	    array = malloc( controls[0].maxLength * size );
	    controls[0].value.pByte = array;

	    /* And repeat query... (assume it works)
	     */
	    mlGetControls( openPath, controls );
	  } else if ( debug ) {
	    fprintf( stdout,
		     "(can not query array: unsupported elem type)\n" );
	  }

	} else if ( debug ) {
	  fprintf( stdout, "(can not query array: maxLength = 0)\n" );
	}
      } /* if controls[0].param & ML_TYPE_VAL_ARRAY */

      ps = sizeof( paramBuffer ) - 1;
      if ( mlPvParamToString( openPath, controls, paramBuffer, &ps ) ) {
	fprintf( stderr, "Unable to convert param 0x%" FORMAT_LLX
		 " to string\n", controls[0].param);
	continue;
      }

      vs = sizeof( valueBuffer ) - 1;
      if ( mlPvValueToString( openPath, controls, valueBuffer, &vs ) ) {
	fprintf( stderr, "Unable to convert %s value %d (0x%x) to string\n",
		 paramBuffer, controls[0].value.int32,
		 controls[0].value.int32 );
	continue;
      }

      if ( ps + vs < 72 ) {
	fprintf( f, "\t%s = %s\n", paramBuffer, valueBuffer );
      } else {
	fprintf( f, "\t%s =\n\t\t%s", paramBuffer, valueBuffer );
      }
    } /* if class == ML_CLASS.... */
  } /* for i=0..paramIds->length */

  mlFreeCapabilities( devCap );

  if ( array != 0 ) {
    free( array );
  }
  return 0;
}
