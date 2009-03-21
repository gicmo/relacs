/*
  base/setoutput.cc
  Set an output to a specified value

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2009 Jan Benda <j.benda@biologie.hu-berlin.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  
  RELACS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <qhbox.h>
#include <qpushbutton.h>
#include <relacs/base/setoutput.h>
using namespace relacs;

namespace base {


SetOutput::SetOutput( void )
  : RePro( "SetOutput", "SetOutput", "Base",
	   "Jan Benda", "0.0", "Mar 20, 2009" ),
    STW( (QWidget*)this )
{
  // add some options:
  addSelection( "outtrace", "Output trace", "V-1" );
  addNumber( "value", "Value to be writen to output trace", 0.0, -100000.0, 100000.0, 0.1 );
  addBoolean( "interactive", "Set values interactively", false );

  // display output variables:
  STW.setSpacing( 2 );
  STW.setMargin( 4 );

  QHBox *bb = new QHBox( this );
  bb->setSpacing( 4 );

  // Ok button:
  QPushButton *okbutton = new QPushButton( "&Ok", bb, "OkButton" );
  connect( okbutton, SIGNAL( clicked() ),
	   this, SLOT( setValues() ) );

  // Cancel button:
  QPushButton *cancelbutton = new QPushButton( "&Cancel", bb, "CancelButton" );
  connect( cancelbutton, SIGNAL( clicked() ),
	   this, SLOT( keepValues() ) );

  bb->setFixedHeight( okbutton->sizeHint().height() );

  setTabOrder( this, &STW );
  setTabOrder( &STW, okbutton );
  setTabOrder( okbutton, cancelbutton );
}


void SetOutput::config( void )
{
  // assemble output traces:
  string ots = "";
  for ( int k=0; k<outTracesSize(); k++ ) {
    if ( k > 0 )
      ots += '|';
    ots += outTraceName( k );
  }
  setText( "outtrace", ots );
  setToDefault( "outtrace" );

  // assemble output traces to OutOpts:
  OutOpts.clear();
  for ( int k=0; k<outTracesSize(); k++ ) {
    int flag = outTrace( k ).channel() < 1000 ? ChannelFlag : ParameterFlag;
    OutOpts.addNumber( outTraceName( k ), 0.0, outTrace( k ).unit() ).setFlags( flag );
  }

  // display values:
  STW.assign( &OutOpts, ParameterFlag, 0, false, 0, mutex() );
  updateGeometry();
}


void SetOutput::notify( void )
{
  int outtrace = index( "outtrace" );
  setUnit( "value", outTrace( outtrace ).unit() );
}


void SetOutput::setValues( void )
{
  Change = true;
  STW.accept( false );
  Wait.wakeAll();
}


void SetOutput::keepValues( void )
{
  Change = false;
  Wait.wakeAll();
}


int SetOutput::main( void )
{
  // get options:
  int outtrace = index( "outtrace" );
  double value = number( "value" );
  bool interactive = boolean( "interactive" );

  noMessage();

  if ( interactive ) {
    OutOpts.delFlags( Parameter::changedFlag() );
    STW.setFocus();
    // wait for input:
    Change = false;
    unlockAll();
    Wait.wait();
    lockAll();
    STW.clearFocus();
    // set new values:
    if ( Change ) {
      OutList sigs;
      for ( int k=0; k<OutOpts.size(); k++ ) {
	if ( OutOpts[k].changed() ) {
	  double value = OutOpts[k].number();
	  OutData sig( value );
	  sig.setTraceName( OutOpts[k].ident() );
	  sig.setIdent( "value=" + Str( value ) );
	  sigs.push( sig );
	}
      }
      if ( sigs.size() > 0 ) {
	directWrite( sigs );
	if ( sigs.failed() ) {
	  warning( sigs.errorText() );
	  return Failed;
	}
      }
    }
    else {
      OutOpts.setDefaults();
      STW.updateValues();
    }
  }
  else {
    // set the single requested value:
    OutData signal( value );
    signal.setIdent( "const" );
    signal.setTrace( outtrace );
    directWrite( signal );
    if ( signal.failed() ) {
      warning( signal.errorText() );
      return Failed;
    }
    OutOpts[outtrace].setNumber( value );
    STW.updateValues();
  }

  sleep( 0.01 );
  return Completed;
}


const Options &SetOutput::outTraces( void ) const
{
  return OutOpts;
}


addRePro( SetOutput );

}; /* namespace base */

#include "moc_setoutput.cc"

