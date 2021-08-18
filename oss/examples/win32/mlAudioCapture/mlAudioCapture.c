/***************************************************************************
 * License Applicability. Except to the extent portions of this file are 
 * made subject to an alternative license as permitted in the Khronos 
 * Free Software License Version 1.0 (the "License"), the contents of 
 * this file are subject only to the provisions of the License. You may 
 * not use this file except in compliance with the License. You may obtain 
 * a copy of the License at 
 * 
 * The Khronos Group Inc.:  PO Box 1019, Clearlake Park CA 95424 USA or at
 *  
 * http://www.Khronos.org/licenses/KhronosOpenSourceLicense1_0.pdf
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


#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "resource.h"
#include "mlAudioCapture.h"
#include "mlOperations.h"


// ============================================================================
//
// Local variables
//
// ============================================================================


// Transfer parameters:
//
//   Filled-in by the main dialog proc, when the user clicks "Next",
//   and used by the capture dialog proc to set up the appropriate
//   transfer parameters, both in the UI and in the actual transfer
//   routines.
static TransferParams transferParams;


// ============================================================================
//
// Local functions
//
// ============================================================================


// ----------------------------------------------------------------------------
//
// Set the UI controls in the main dialog box to match the currently
// selected device/path combination.
//
// For instance, if the path does not support stereo transfers,
// disable the 'stereo' radio button.
static void setPathCaps( HWND hwnd, const CurSysInfo* sysInfo )
{
  HWND hDlgItem;
  int devIdx, pathIdx;
  const PathInfo* pathInfo;

  // Find currently selected device & path
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVICE );
  devIdx = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVPATH );
  pathIdx = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );

  if ( (devIdx == CB_ERR) || (pathIdx == CB_ERR) ) {
    return;
  }
  assert( devIdx < sysInfo->numDevs );
  assert( pathIdx < sysInfo->devs[devIdx].numPaths );

  pathInfo = &(sysInfo->devs[devIdx].paths[pathIdx]);

  // Enable or disable MONO & STEREO according to path caps
  hDlgItem = GetDlgItem( hwnd, IDC_RADIO_MONO );
  if ( pathInfo->monoOK ) {
    EnableWindow( hDlgItem, TRUE );
  } else {
    EnableWindow( hDlgItem, FALSE );
  }

  hDlgItem = GetDlgItem( hwnd, IDC_RADIO_STEREO );
  if ( pathInfo->stereoOK ) {
    EnableWindow( hDlgItem, TRUE );
    CheckRadioButton( hwnd, IDC_RADIO_MONO, IDC_RADIO_STEREO,
		      IDC_RADIO_STEREO );
  } else {
    EnableWindow( hDlgItem, FALSE );
    CheckRadioButton( hwnd, IDC_RADIO_MONO, IDC_RADIO_STEREO,
		      IDC_RADIO_MONO );
  }

  // Path information does not tell us what rates are valid, so assume
  // they all are.
  //
  // Set default rate to 44.1 KHz. 
  CheckRadioButton( hwnd, IDC_RADIO_44KHz, IDC_RADIO_96KHz,
		    IDC_RADIO_44KHz );
  return;
}


// ----------------------------------------------------------------------------
//
// Set the entries in the path combo-box according to the
// currently-selected device.
static void setPathChoices( HWND hwnd, const CurSysInfo* sysInfo )
{
  HWND hDlgItem;
  int devIdx;
  static int lastDevIdx = CB_ERR;

  // Find out which device is currently selected
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVICE );
  devIdx = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );

  // If the device selection hasn't actually changed since we last set
  // the path choices, don't do anything
  if ( devIdx == lastDevIdx ) {
    return;
  }

  lastDevIdx = devIdx;

  // Now get Path combo box, and clear it
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVPATH );
  SendMessage( hDlgItem, CB_RESETCONTENT, 0, 0 );

  if ( devIdx != CB_ERR ) {
    int numPaths;
    int i;

    assert( devIdx < sysInfo->numDevs );
    numPaths = sysInfo->devs[devIdx].numPaths;
    for ( i=0; i < numPaths; ++i ) {
      int idx =
	SendMessage( hDlgItem, CB_ADDSTRING, 0,
		     (LPARAM) sysInfo->devs[devIdx].paths[i].pathName );
      assert( idx == i );
    }

    // Force selection to first item in the list
    SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 );
    setPathCaps( hwnd, sysInfo );
  }
  return;
}

// ----------------------------------------------------------------------------
//
// Get the ML system information, and set the entries in the device
// combo-box to reflect the available devices on this system.
//
// Returns NULL on error.
static const CurSysInfo* setSysInfo( HWND hwnd )
{
  HWND hDlgItem;
  int i;
  const CurSysInfo* sysInfo;

  sysInfo = getSysInfo();
  if ( sysInfo == NULL ) {
    // This is an error. Show an error box.
    MessageBox( hwnd, getMLError(), "Error", MB_OK | MB_ICONERROR );
    return NULL;
  }

  SetDlgItemText( hwnd, IDC_SYS_NAME, sysInfo->sysName );

  // Add all device names to the combo box. We assume that the
  // position in the combo box will match the index into the
  // CurSysInfo struct
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVICE );
  for ( i=0; i < sysInfo->numDevs; ++i ) {
    int idx = SendMessage( hDlgItem, CB_ADDSTRING,
			   0, (LPARAM) sysInfo->devs[i].devName );
    assert( idx == i );
  }

  // Force selection of first device, to get things going
  SendMessage( hDlgItem, CB_SETCURSEL, 0, 0 );
  setPathChoices( hwnd, sysInfo );
  return sysInfo;
}


// ----------------------------------------------------------------------------
//
// Set the contents of the transferParams global variable according to
// the UI settings of the main dialog box.
//
// Should be called before creating the capture dialog box (as it will
// look in this global variable for the settings).
static void setTransferParams( HWND hwnd, const CurSysInfo* sysInfo )
{
  HWND hDlgItem;
  int devIdx, pathIdx;

  // Get device and path ML IDs
  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVICE );
  devIdx = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
  assert( (devIdx != CB_ERR) && (devIdx < sysInfo->numDevs) );
  transferParams.devID = sysInfo->devs[devIdx].devID;

  hDlgItem = GetDlgItem( hwnd, IDC_COMBO_DEVPATH );
  pathIdx = SendMessage( hDlgItem, CB_GETCURSEL, 0, 0 );
  assert( (pathIdx != CB_ERR) &&
	  (pathIdx < sysInfo->devs[devIdx].numPaths) );
  transferParams.pathID = sysInfo->devs[devIdx].paths[pathIdx].pathID;

  // Get number of channels and rate
  if ( IsDlgButtonChecked( hwnd, IDC_RADIO_MONO ) == BST_CHECKED ) {
    transferParams.numChannels = 1;
  } else {
    // Assume stereo
    transferParams.numChannels = 2;
  }

  if ( IsDlgButtonChecked( hwnd, IDC_RADIO_44KHz ) == BST_CHECKED ) {
    transferParams.rate = 44100.0;
  } else if ( IsDlgButtonChecked( hwnd, IDC_RADIO_48KHz ) == BST_CHECKED ) {
    transferParams.rate = 48000.0;
  } else {
    // Assume 96KHz
    transferParams.rate = 96000.0;
  }

  // Get output file name
  GetDlgItemText( hwnd, IDC_EDIT_FILENAME, transferParams.outFile,
		  sizeof(transferParams.outFile) );

  // For now, number of buffers is not user-selectable...
  transferParams.numBuffers = 10;
  return;
}


// ----------------------------------------------------------------------------
//
// Format a timestamp (expressed in milli-seconds) into a printable
// string in the form:
//   hh:mm:ss.d
static const char* formatTimeStr( long milliSecs )
{
  static char timeStr[16]; // format: 00:00:00.0
  int hrs, mins, secs, tenths;

  hrs = milliSecs / (1000*60*60);
  milliSecs %= (1000*60*60);
  mins = milliSecs / (1000*60);
  milliSecs %= (1000*60);
  secs = milliSecs / 1000;
  milliSecs %= 1000;
  tenths = milliSecs / 100;

  sprintf( timeStr, "%02d:%02d:%02d.%d", hrs, mins, secs, tenths );

  return timeStr;
}


// ----------------------------------------------------------------------------
//
// Draw signal strength meter.
//
// Params:
//  hwnd: window in which to draw meter. Meter fills the window.
//  left: if true, this is the left-channel meter. This determines
//        whether the 'max strength' meter is on the left or the right
//        of the current strength meter.
//  strength: current signal strength
//  maxStrength: max signal strength (accumulated over many buffers).
static void drawSignalMeter( HWND hwnd, BOOL left,
			     float strength, float maxStrength )
{
  static HBRUSH green = 0, red = 0, bkgrnd = 0;
  HDC hdc;
  RECT clientRect;
  RECT widgetRect;
  int widgetHeight;
  int i;
  int level, maxLevel;

  if ( green == 0 ) {
    // First-time init: create color brushes
    green = CreateSolidBrush( 0x0000ff00 );
    red = CreateSolidBrush( 0x000000ff );
    bkgrnd = CreateSolidBrush( 0x007f7f7f );
  }

  hdc = GetDC( hwnd );
  GetClientRect( hwnd, &clientRect );

  // Start by blanking the window, to erase any previously-drawn meter
  FillRect( hdc, &clientRect, bkgrnd );

  // Determine bounding boxes for the current signal meter -- in the
  // right half of the window for the left channel meter, and in the
  // left half for the right channel.
  if ( left ) {
    widgetRect.left = clientRect.right / 2 + 1;
    widgetRect.right = clientRect.right;
  } else {
    widgetRect.left = 0;
    widgetRect.right = clientRect.right / 2 - 1;
  }
  widgetHeight = clientRect.bottom / 16;

  // For now, hard-code to use 16 levels (of which last 2 are red)
  // Convert normalized strength to an integer level value
  level = (int)(strength * 15.0f);
  maxLevel = (int)(maxStrength * 15.0f);

  for ( i=0; i < 16; ++i ) {
    widgetRect.bottom = clientRect.bottom - (widgetHeight * i);
    widgetRect.top = widgetRect.bottom - widgetHeight + 1;
    if ( i <= level ) {
      FillRect( hdc, &widgetRect, (i < 14 ? green : red) );
    } else {
      FrameRect( hdc, &widgetRect, (i < 14 ? green : red) );
    }
  }

  // Determine bounding boxes for the max strength mirror -- use the
  // other half of the window, the one not used by the current
  // strength.
  if ( left ) {
    widgetRect.left = 0;
    widgetRect.right = clientRect.right / 2 - 1;
  } else {
    widgetRect.left = clientRect.right / 2 + 1;
    widgetRect.right = clientRect.right;
  }
  widgetRect.bottom = clientRect.bottom - (widgetHeight * maxLevel);
  widgetRect.top = widgetRect.bottom - widgetHeight + 1;
  FillRect( hdc, &widgetRect, (maxLevel < 14 ? green : red) );

  ReleaseDC( hwnd, hdc );
  return;
}


// ----------------------------------------------------------------------------
//
// Load the app's icons into the specified window
void loadIcons( HWND hwnd )
{
  HANDLE hIconBig, hIconSmall;
  HINSTANCE hInst;

  hInst = GetModuleHandle( NULL );
  hIconBig = LoadImage( hInst, MAKEINTRESOURCE( IDI_ICON_LARGE ), IMAGE_ICON,
			32, 32, LR_SHARED );
  hIconSmall = LoadImage( hInst, MAKEINTRESOURCE( IDI_ICON_LARGE ), IMAGE_ICON,
			16, 16, LR_SHARED );
  SendMessage( hwnd, WM_SETICON, ICON_BIG, (LPARAM) hIconBig );
  SendMessage( hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIconSmall );
}


// ============================================================================
//
// Public functions
//
// ============================================================================


// ----------------------------------------------------------------------------
//
// Message-processing function for the 'capture' dialog box
BOOL CALLBACK CaptureDlgProc( HWND hwnd, UINT Message,
			      WPARAM wParam, LPARAM lParam )
{
  BOOL weAreDone = FALSE;

  // Current signal strength and max signal strength for left and
  // right channels.
  //
  // Updated in response to application-private messages sent by the
  // child thread, and used to redraw the signal meters.
#define LEFT_CHANNEL 0
#define RIGHT_CHANNEL 1
  static float sigStrength[2], maxStrength[2];

  // Shortcut to LEFT and RIGHT channel meter resource IDs
  const static int chanDrawID[2] = { IDC_LCHAN_DRAW, IDC_RCHAN_DRAW };

  switch ( Message ) {

  case WM_INITDIALOG: {
    HWND hDlgItem;
    int i;

    loadIcons( hwnd );

    // Prepare for the ML transfer -- allocate resources, create
    // thread, etc.
    if ( ! prepareTransfer( &transferParams, hwnd ) ) {
      MessageBox( hwnd, getMLError(), "Error", MB_OK | MB_ICONERROR );
      weAreDone = TRUE;
      break;
    }

    // Set up initial capture time and dropped buffers strings
    SetDlgItemText( hwnd, IDC_CAPTURE_TIME, "00:00:00.0" );
    SetDlgItemText( hwnd, IDC_DROPPED_BUFFS, "0" );
    SetDlgItemText( hwnd, IDC_EFFECTIVE_RATE, "0.0" );

    hDlgItem = GetDlgItem( hwnd, IDC_LIST_DROPPED_BUFFS );
    SendMessage( hDlgItem, LB_RESETCONTENT, 0, 0 );

    hDlgItem = GetDlgItem( hwnd, IDC_STOP );
    EnableWindow( hDlgItem, FALSE );

    // Setup signal meters
    for ( i=0; i < 2; ++i ) {
      LONG wStyle;

      sigStrength[i] = 0.0f;
      maxStrength[i] = 0.0f;

      hDlgItem = GetDlgItem( hwnd, chanDrawID[i] );
      wStyle = GetWindowLong( hDlgItem, GWL_STYLE );
      wStyle |= SS_OWNERDRAW;
      SetWindowLong( hDlgItem, GWL_STYLE, wStyle );
      drawSignalMeter( hDlgItem, TRUE, sigStrength[i], maxStrength[i] );
    } // for i=0..2
  } break; // case WM_INITDIALOG

  case WM_DRAWITEM:
    // Called by children window with the OWNERDRAW property -- in our
    // case, one of the signal meter windows.
    if ( (wParam == IDC_LCHAN_DRAW) || (wParam == IDC_RCHAN_DRAW) ) {
      HWND hDlgItem;

      hDlgItem = GetDlgItem( hwnd, wParam );
      if ( wParam == IDC_LCHAN_DRAW ) {
	drawSignalMeter( hDlgItem, TRUE, sigStrength[LEFT_CHANNEL],
			 maxStrength[LEFT_CHANNEL] );
      } else {
	drawSignalMeter( hDlgItem, FALSE, sigStrength[RIGHT_CHANNEL],
			 maxStrength[RIGHT_CHANNEL] );
      }
    } else {
      // Not a signal meter window? Don't know how to handle it then.
      return FALSE;
    }
    break;

  case WM_COMMAND:
    switch ( LOWORD(wParam) ) {
    case IDC_DONE:
      weAreDone = TRUE;
      break;

    case IDC_START: {
      HWND hDlgItem;

      // Disable the start button, enable the stop button
      hDlgItem = GetDlgItem( hwnd, IDC_START );
      EnableWindow( hDlgItem, FALSE );
      hDlgItem = GetDlgItem( hwnd, IDC_STOP );
      EnableWindow( hDlgItem, TRUE );

      // Start actual transfer
      if ( ! startTransfer() ) {
	MessageBox( hwnd, getMLError(), "Error", MB_OK | MB_ICONERROR );
	weAreDone = TRUE;
      }
    } break; // case IDC_START

    case IDC_STOP: {
      HWND hDlgItem;

      // Disable the stop button, enable the start button
      hDlgItem = GetDlgItem( hwnd, IDC_START );
      EnableWindow( hDlgItem, TRUE );
      hDlgItem = GetDlgItem( hwnd, IDC_STOP );
      EnableWindow( hDlgItem, FALSE );

      // Stop the transfer
      if ( ! stopTransfer() ) {
	MessageBox( hwnd, getMLError(), "Error", MB_OK | MB_ICONERROR );
	weAreDone = TRUE;
      }
    } break; // case IDC_STOP

    case IDC_BUTTON_RESET_MAX: {
      int i;

      // Reset max signal strengths, and request a redraw
      for ( i=0; i < 2; ++i ) {
	maxStrength[i] = 0.0f;
	RedrawWindow( GetDlgItem( hwnd, chanDrawID[i] ),
		      NULL, NULL, RDW_INVALIDATE );
      }
    } break; // case IDC_BUTTON_RESET_MAX

    case IDC_CHECK_MONITOR: {
      if ( IsDlgButtonChecked( hwnd, IDC_CHECK_MONITOR ) ) {
	startMonitoring();

      } else {
	stopMonitoring();

	// We could also reset the meters to zero, but the child
	// thread will send a zero-strength message when it registers
	// the stop-monitor command, so there is no need to do
	// anything here.
      } // if IsDlgButtonChecked
    } break; // case IDC_CHECK_MONITOR
		
    }; // switch LOWORD(wParam)
    break; // case WM_COMMAND

  case WM_CLOSE:
    weAreDone = TRUE;
    break;

  //
  // Application-private messages
  //
  case WMA_MLAC_ERROR:
    MessageBox( hwnd, (const char*) lParam, "Error", MB_OK | MB_ICONERROR );
    free( (char*) lParam );
    weAreDone = TRUE;
    break;

  case WMA_MLAC_DROPPED_BUFF: {
    static const unsigned int MaxDroppedListed = 10;
    HWND hDlgItem;

    hDlgItem = GetDlgItem( hwnd, IDC_LIST_DROPPED_BUFFS );
    SetDlgItemInt( hwnd, IDC_DROPPED_BUFFS, (UINT) wParam, TRUE );
    SendMessage( hDlgItem, LB_INSERTSTRING, 0,
		 (LPARAM) formatTimeStr( (long) lParam ) );
    if ( wParam > MaxDroppedListed ) {
      // Trim the list of timestamps, dropping the oldest timestamp
      SendMessage( hDlgItem, LB_DELETESTRING, MaxDroppedListed, (LPARAM) 0 );
    }
  } break; // case WMA_MLAC_DROPPED_BUFF

  case WMA_MLAC_TOTAL_TIME:
    SetDlgItemText( hwnd, IDC_CAPTURE_TIME, formatTimeStr( (long) lParam ) );
    break;

  case WMA_MLAC_RSIGNAL_STRENGTH:
  case WMA_MLAC_LSIGNAL_STRENGTH: {
    int idx = (Message == WMA_MLAC_RSIGNAL_STRENGTH ?
	       RIGHT_CHANNEL : LEFT_CHANNEL);
    sigStrength[idx] = (float)wParam / (float)lParam;
    if ( sigStrength[idx] > maxStrength[idx] ) {
      maxStrength[idx] = sigStrength[idx];
    }
    RedrawWindow( GetDlgItem( hwnd, chanDrawID[idx] ),
		  NULL, NULL, RDW_INVALIDATE );
  } break; // case WMA_MLAC_RSIGNAL_STRENGTH and WMA_MLAC_LSIGNAL_STRENGTH

  case WMA_MLAC_EFFECTIVE_RATE: {
    char rateStr[8]; // format: 00.0
    float rateKHz = (float)wParam / 1000.0f;
    sprintf( rateStr, "%3.1f", rateKHz );
    SetDlgItemText( hwnd, IDC_EFFECTIVE_RATE, rateStr );
  } break;

  default:
    return FALSE;

  } // switch Message

  if ( weAreDone == TRUE ) {
    finishTransfer();
    EndDialog( hwnd, 0 );
  }

  return TRUE;
}


// ----------------------------------------------------------------------------
//
// Message-processing function for the main dialog box
BOOL CALLBACK MainDlgProc( HWND hwnd, UINT Message,
			   WPARAM wParam, LPARAM lParam )
{
  static const CurSysInfo* sysInfo = NULL;
  BOOL weAreDone = FALSE;

  switch ( Message ) {
  case WM_INITDIALOG:
    loadIcons( hwnd );

    sysInfo = setSysInfo( hwnd );
    if ( sysInfo == NULL ) {
      // Something went wrong, shut down
      weAreDone = TRUE;
    }
    // Make sure file name EDIT control is initially empty
    SetDlgItemText( hwnd, IDC_EDIT_FILENAME, "" );
    break;

  case WM_COMMAND:
    switch ( LOWORD(wParam) ) {
    case IDC_EXIT:
      weAreDone = TRUE;
      break;

    case IDC_NEXT: {
      // Make sure a file name has been entered
      int nameLen =
	GetWindowTextLength( GetDlgItem( hwnd, IDC_EDIT_FILENAME ) );
      if ( nameLen == 0 ) {
	MessageBox( hwnd, "Please specify a file name", "Notice",
		    MB_OK | MB_ICONINFORMATION );

      } else {
	// Set transfer params according to current UI settings, and
	// create the 'capture' dialog box (which will retrieve the
	// settings from the global variable)
	setTransferParams( hwnd, sysInfo );
	DialogBox( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_CAPTURE),
		   hwnd, CaptureDlgProc );
      }
    } break; // case IDC_NEXT

    case IDC_COMBO_DEVICE:
      if ( HIWORD(wParam) == CBN_CLOSEUP ) {
	// Selection change -- set Path info
	setPathChoices( hwnd, sysInfo );
      }
      break;

    case IDC_COMBO_DEVPATH:
      if ( HIWORD(wParam) == CBN_CLOSEUP ) {
	// Selection change -- set Path capabilities
	setPathCaps( hwnd, sysInfo );
      }
      break;
    } // switch LOWORD(wParam)
    break; // case WM_COMMAND

  case WM_CLOSE:
    weAreDone = TRUE;
    break;

  default:
    return FALSE;
  } // switch Message

  if ( weAreDone ) {
    if ( sysInfo != NULL ) {
      freeSysInfo( sysInfo );
    }
    EndDialog( hwnd, 0 );
  }

  return TRUE;
}


// ----------------------------------------------------------------------------
//
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpCmdLine, int nCmdShow )
{
  return DialogBox( hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN),
		    NULL, MainDlgProc );
}
