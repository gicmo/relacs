/*
  ephys/setleak.cc
  Set leak resistance and reversal potential of a dynamic clamp model.

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2015 Jan Benda <jan.benda@uni-tuebingen.de>

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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <relacs/ephys/setleak.h>
using namespace relacs;

namespace ephys {


SetLeak::SetLeak( void )
  : RePro( "SetLeak", "ephys", "Jan Benda", "1.0", "Mar 21, 2009" )
{
  // add some options:
  addBoolean( "interactive", "Set values interactively", true ).setFlags( 1 );
  addSelection( "preset", "Set g and E to", "previous|zero|custom" ).setUnit( "values" ).setFlags( 1 );
  addNumber( "g", "New value of leak conductance", 0.0, -100000.0, 100000.0, 0.1, "nS" ).setActivation( "preset", "custom" ).setFlags( 1 );
  addNumber( "E", "New value of leak reversal-potential", 0.0, -10000.0, 10000.0, 1.0, "mV" ).setActivation( "preset", "custom" ).setFlags( 1 );
  addBoolean( "reversaltorest", "Set leak reversal-potential to resting potential", true ).setActivation( "preset", "zero", false ).setFlags( 1 );
  addSelection( "involtage", "Input voltage trace for measuring resting potential", "V-1" ).setFlags( 1 );
  addNumber( "duration", "Duration of resting potential measurement", 0.1, 0.001, 1000.0, 0.001, "sec", "ms" ).setFlags( 1 );
  setConfigSelectMask( 1 );
  setDialogSelectMask( 1 );

  // dialog:
  newSection( "Passive membrane properties of the cell:" ).setFlags( 2 );
  addNumber( "Rm", "Resistance R_m", 0.0, 0.0, 1.0e8, 1.0, "MOhm", "MOhm", "%.1f" ).setFlags( 2+4 );
  addNumber( "Taum", "Time constant tau_m", 0.0, 0.0, 1.0e6, 0.001, "s", "ms", "%.1f" ).setFlags( 2+4 );
  newSection( "Injected current I=g(E-V):" ).setFlags( 2 );
  addNumber( "gdc", "Additional leak conductance g", 0.0, -1.0e8, 1.0e8, 1.0, "nS" ).setFlags( 2 );
  addNumber( "Edc", "Reversal potential E", 0.0, -1000.0, 1000.0, 1.0, "mV" ).setFlags( 2 );
  newSection( "Resulting membrane properties:" ).setFlags( 2 );
  addNumber( "Rdc", "New membrane resistance 1/R=1/R_m+g", 0.0, 0.0, 1.0e8, 1.0, "MOhm" ).setFlags( 2 );
  addNumber( "taudc", "New membrane time constant", 0.0, 0.0, 1.0e6, 0.001, "s", "ms" ).setFlags( 2 );

  // layout:
  QVBoxLayout *vb = new QVBoxLayout;
  setLayout( vb );

  // display values:
  STW.assign( (Options*)this, 2, 4, true, 0, mutex() );
  STW.setVerticalSpacing( 2 );
  STW.setMargins( 4 );
  vb->addWidget( &STW );

  QHBoxLayout *bb = new QHBoxLayout;
  bb->setSpacing( 4 );
  vb->addLayout( bb );

  // Ok button:
  OKButton = new QPushButton( "&Ok" );
  bb->addWidget( OKButton );
  connect( OKButton, SIGNAL( clicked() ),
	   this, SLOT( setValues() ) );
  grabKey( Qt::ALT+Qt::Key_O );
  grabKey( Qt::Key_Return );
  grabKey( Qt::Key_Enter );

  // Cancel button:
  CancelButton = new QPushButton( "&Cancel" );
  bb->addWidget( CancelButton );
  connect( CancelButton, SIGNAL( clicked() ),
	   this, SLOT( keepValues() ) );
  grabKey( Qt::ALT+Qt::Key_C );
  grabKey( Qt::Key_Escape );

  // Reset button:
  ResetButton = new QPushButton( "&Reset" );
  bb->addWidget( ResetButton );
  connect( ResetButton, SIGNAL( clicked() ),
	   this, SLOT( resetValues() ) );
  grabKey( Qt::ALT+Qt::Key_R );

  // E to VRest button:
  VRestButton = new QPushButton( "&E to VRest" );
  bb->addWidget( VRestButton );
  connect( VRestButton, SIGNAL( clicked() ),
	   this, SLOT( measureVRest() ) );
  grabKey( Qt::ALT+Qt::Key_E );

  OKButton->setFixedHeight( OKButton->sizeHint().height() );
  CancelButton->setFixedHeight( OKButton->sizeHint().height() );
  ResetButton->setFixedHeight( OKButton->sizeHint().height() );
  VRestButton->setFixedHeight( OKButton->sizeHint().height() );
  bb->setSpacing( 4 );
}


void SetLeak::preConfig( void )
{
  setText( "involtage", traceNames() );
  setToDefault( "involtage" );
}


void SetLeak::notify( void )
{
  double rm = number( "Rm", 0.0, "MOhm" );
  if ( rm > 1.0e-6 ) {
    bool update = true;
    lockMetaData();
    double cm = metaData().number( "Cell>cm", 0.0, "pF" );
    unlockMetaData();
    bool sn = unsetNotify();
    if ( changed( "gdc" ) ) {
      double rdc = 1.0/(0.001*number( "gdc" )+1.0/rm);
      setNumber( "Rdc", rdc );
      setNumber( "taudc", 1.0e-6*rdc*cm );
    }
    else if ( changed( "Rdc" ) ) {
      double rdc = number( "Rdc" );
      if ( rdc > 1.0e-6 ) {
	setNumber( "gdc", 1000.0/rdc-1000.0/rm );
	setNumber( "taudc", 1.0e-6*rdc*cm );
      }
    }
    else if ( changed( "taudc" ) ) {
      double taudc = number( "taudc" );
      if ( cm > 1.0e-6 ) {
	double rdc = 1.0e6*taudc/cm;
	setNumber( "Rdc", rdc );
	setNumber( "gdc", 1000.0/rdc-1000.0/rm );
      }
    }
    else if ( ! changed( "Edc" ) )
      update = false;
    setNotify( sn );
    if ( update ) {
      delFlags( Parameter::changedFlag() );
      STW.updateValues();
    }
  }
}


void SetLeak::setValues( void )
{
  Change = true;
  Reset = false;
  STW.accept();
  wake();
}


void SetLeak::keepValues( void )
{
  Change = false;
  Reset = false;
  wake();
}


void SetLeak::resetValues( void )
{
  Change = true;
  Reset = true;
  wake();
}


void SetLeak::measureVRest( void )
{
  int involtage = traceIndex( settings().text( "involtage", 0 ) );
  if ( involtage < 0 ) 
    return;
  lock();
  double duration = settings().number( "duration" );
  const InData &data = trace( involtage );
  double vrest = data.mean( currentTime()-duration, currentTime() );
  setNumber( "Edc", vrest );
  STW.updateValue( "Edc" );  
  unlock();
}


int SetLeak::main( void )
{
  // get options:
  int preset = index( "preset" );
  bool interactive = boolean( "interactive" );
  if ( ! interactive )
    preset = 2;
  double g = number( "g" );
  double E = number( "E" );
  bool reversaltorest = boolean( "reversaltorest" );

  noMessage();

  if ( preset == 0 ) {
    // previous values:
    lockStimulusData();
    E = stimulusData().number( "E", 0.0, "mV" );
    g = stimulusData().number( "g", 0.0, "nS" );
    unlockStimulusData();
  }
  else if ( preset == 1 ) {
    // zero:
    E = 0.0;
    g = 0.0;
  }
  lockMetaData();
  if ( reversaltorest && preset != 1 )
    E = metaData().number( "Cell>vrest", 0.0, "mV" );

  unsetNotify();
  setNumber( "Rm", metaData().number( "Cell>rm", 0.0, "MOhm" ) );
  setNumber( "Taum", metaData().number( "Cell>taum", 0.0, "s" ) );
  setNumber( "Edc", E );
  setNumber( "gdc", g );
  delFlags( Parameter::changedFlag() );
  addFlags( "gdc", Parameter::changedFlag() );
  unlockMetaData();
  notify();  // calls already STW.updateValues()
  setNotify();

  if ( interactive ) {
    keepFocus();
    postCustomEvent( 11 ); // STW.setFocus();
    // wait for input:
    Change = false;
    Reset = false;
    sleepWait();
    postCustomEvent( 12 ); // clearFocus();
    // set new values:
    if ( Change ) {
      g = Reset ? 0.0 : number( "gdc" );
      E = Reset ? 0.0 : number( "Edc" );
    }
    else {
      setDefaults();   // calls STW.updateValues() via notify
      return Aborted;
    }
  }

  // set the requested values:
  message( "set <b>g=" + Str( g ) + "nS</b> and <b>E=" + Str( E ) + "mV</b>" );
  OutList signal;
  signal.resize( 2 );
  signal[0].setTraceName( "g" );
  signal[0].constWave( g );
  signal[0].setIdent( "g=" + Str( g ) + "nS" );
  signal[1].setTraceName( "E" );
  signal[1].constWave( E );
  signal[1].setIdent( "E=" + Str( E ) + "mV" );
  directWrite( signal );
  if ( signal.failed() ) {
    warning( "Failed to write new values: " + signal.errorText() );
    return Failed;
  }
  unsetNotify();
  setNumber( "gdc", g );
  setNumber( "Edc", E );
  setToDefaults();
  notify();  // calls already STW.updateValues()
  setNotify();
  
  sleep( 0.01 );
  return Completed;
}


void SetLeak::keyPressEvent( QKeyEvent *e )
{
  e->ignore();
  if ( e->key() == Qt::Key_O && ( e->modifiers() & Qt::AltModifier ) ) {
    OKButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_C && ( e->modifiers() & Qt::AltModifier ) ) {
    CancelButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_R && ( e->modifiers() & Qt::AltModifier ) ) {
    ResetButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_E && ( e->modifiers() & Qt::AltModifier ) ) {
    VRestButton->animateClick();
    e->accept();
  }
  else if ( ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter ) && ( e->modifiers() == Qt::NoModifier ) ) {
    OKButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_Escape && ( e->modifiers() == Qt::NoModifier ) ) {
    CancelButton->animateClick();
    e->accept();
  }
  else
    RePro::keyPressEvent( e );
}


void SetLeak::customEvent( QEvent *qce )
{
  switch ( qce->type() - QEvent::User ) {
  case 11: {
    if ( STW.firstWidget() != 0 )
      STW.firstWidget()->setFocus( Qt::TabFocusReason );
    break;
  }
  case 12: {
    removeFocus();
    break;
  }
  default:
    RePro::customEvent( qce );
  }
}


addRePro( SetLeak, ephys );

}; /* namespace ephys */

#include "moc_setleak.cc"
