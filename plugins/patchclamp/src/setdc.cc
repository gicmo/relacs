/*
  patchclamp/setdc.cc
  Interactively set the DC current.

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
#include <QGridLayout>
#include <relacs/patchclamp/setdc.h>
using namespace relacs;

namespace patchclamp {


const double SetDC::DCSteps[NDCSteps] = { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0 };


SetDC::SetDC( void )
  : RePro( "SetDC", "patchclamp", "Jan Benda", "1.6", "Sep 13, 2014" ),
    P( 2, 1 ),
    IUnit( "nA" )
{
  ShowStdev = true;

  // add some options:
  addSelection( "outcurrent", "Output trace", "Current-1" );
  addSelection( "dcamplitudesel", "Set DC amplitude", "to absolute value|to previous value|to a fraction of the threshold|relative to threshold" );
  addNumber( "dcamplitude", "DC amplitude", 0.0, -1000.0, 1000.0, 0.01 ).setActivation( "dcamplitudesel", "to absolute value" );
  addNumber( "dcamplitudefrac", "Fraction of threshold", 0.9, 0.0, 1.0, 0.01, "1", "%" ).setActivation( "dcamplitudesel", "to a fraction of the threshold" );
  addNumber( "dcamplitudedecr", "Decrement below threshold", 0.1, 0.0, 1000.0, 0.01 ).setActivation( "dcamplitudesel", "relative to threshold" );
  addBoolean( "interactive", "Set dc amplitude interactively", false );
  string dcsteps;
  for ( int k=0; k<NDCSteps; k++ ) {
    if ( k > 0 )
      dcsteps += '|';
    dcsteps += Str( DCSteps[k] );
  }
  addSelection( "dcamplitudestep", "Stepsize for entering dc", dcsteps );
  addBoolean( "showstep", "Show stepsize widget", true );
  addNumber( "duration", "Duration for analysis", 2.0, 0.0, 1000.0, 0.1, "seconds" );
  addNumber( "update", "Update time for analysis", 0.5, 0.0, 1000.0, 0.1, "seconds" );
  addBoolean( "showstdev", "Print standard deviation of voltage", ShowStdev );

  QHBoxLayout *hb = new QHBoxLayout;
  setLayout( hb );

  QVBoxLayout *vb = new QVBoxLayout;
  hb->addLayout( vb );

  QGridLayout *gl = new QGridLayout;
  vb->addLayout( gl );

  // edit:
  QLabel *label = new QLabel( "DC current" );
  label->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
				     QSizePolicy::MinimumExpanding ) );
  gl->addWidget( label, 0, 1 );
  DCStepIndex = 0;
  DCStep = DCSteps[DCStepIndex];
  EW = new DoubleSpinBox;
  EW->setRange( -1000.0, 1000.0 );
  EW->setValue( 0.0 );
  EW->setFormat( "%.3f" );
  EW->setSingleStep( DCStep );
  EW->setKeyboardTracking( false );
  gl->addWidget( EW, 0, 2 );
  CurrentUnitLabel1 = new QLabel( "nA" );
  gl->addWidget( CurrentUnitLabel1, 0, 3 );

  // stepsize:
  DCStepLabel = new QLabel( "DC current increment" );
  DCStepLabel->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
					   QSizePolicy::MinimumExpanding ) );
  gl->addWidget( DCStepLabel, 1, 1 );
  CW = new QComboBox;
  CW->setEditable( false );
  for ( int k=0; k<NDCSteps; k++ )
    CW->addItem( Str( DCSteps[k] ).c_str() );
  gl->addWidget( CW, 1, 2 );
  CurrentUnitLabel2 = new QLabel( "nA" );
  gl->addWidget( CurrentUnitLabel2, 1, 3 );

  // mean voltage:
  label = new QLabel( "Membrane potential" );
  label->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
				     QSizePolicy::MinimumExpanding ) );
  gl->addWidget( label, 2, 1 );
  VoltageLabel = new QLabel( "0.0" );
  VoltageLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  VoltageLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  VoltageLabel->setLineWidth( 2 );
  VoltageLabel->setFixedHeight( VoltageLabel->sizeHint().height() );
  QFont nf( widget()->font() );
  nf.setPointSizeF( 1.3 * widget()->fontInfo().pointSizeF() );
  nf.setBold( true );
  VoltageLabel->setFont( nf );
  VoltageLabel->setAutoFillBackground( true );
  QPalette qp( widget()->palette() );
  qp.setColor( QPalette::Window, Qt::black );
  qp.setColor( QPalette::WindowText, Qt::green );
  VoltageLabel->setPalette( qp );
  gl->addWidget( VoltageLabel, 2, 2 );
  VoltageUnitLabel = new QLabel( "mV" );
  gl->addWidget( VoltageUnitLabel, 2, 3 );

  // mean rate:
  label = new QLabel( "Firing rate" );
  label->setSizePolicy( QSizePolicy( QSizePolicy::Minimum,
				     QSizePolicy::MinimumExpanding ) );
  gl->addWidget( label, 3, 1 );
  RateLabel = new QLabel( "0.0" );
  RateLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  RateLabel->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  RateLabel->setLineWidth( 2 );
  RateLabel->setFixedHeight( RateLabel->sizeHint().height() );
  RateLabel->setFont( nf );
  RateLabel->setAutoFillBackground( true );
  RateLabel->setPalette( qp );
  gl->addWidget( RateLabel, 3, 2 );
  QLabel *ul = new QLabel( "Hz" );
  gl->addWidget( ul, 3, 3 );

  // buttons:
  QHBoxLayout *bb = new QHBoxLayout;
  vb->addLayout( bb );
  bb->setSpacing( 4 );

  // Ok button:
  OKButton = new QPushButton( "&Ok" );
  OKButton->setFixedHeight( OKButton->sizeHint().height() );
  bb->addWidget( OKButton );
  connect( OKButton, SIGNAL( clicked() ),
	   this, SLOT( setValue() ) );
  grabKey( Qt::ALT+Qt::Key_O );
  grabKey( Qt::Key_Return );
  grabKey( Qt::Key_Enter );
  
  // Cancel button:
  CancelButton = new QPushButton( "&Cancel" );
  CancelButton->setFixedHeight( OKButton->sizeHint().height() );
  bb->addWidget( CancelButton );
  connect( CancelButton, SIGNAL( clicked() ),
	   this, SLOT( keepValue() ) );
  grabKey( Qt::ALT+Qt::Key_C );
  grabKey( Qt::Key_Escape );
  
  // Reset button:
  ResetButton = new QPushButton( "Re&set" );
  ResetButton->setFixedHeight( OKButton->sizeHint().height() );
  bb->addWidget( ResetButton );
  connect( ResetButton, SIGNAL( clicked() ),
	   this, SLOT( resetDC() ) );
  grabKey( Qt::ALT+Qt::Key_S );
  
  // Zero button:
  ZeroButton = new QPushButton( "&Zero" );
  ZeroButton->setFixedHeight( OKButton->sizeHint().height() );
  bb->addWidget( ZeroButton );
  connect( ZeroButton, SIGNAL( clicked() ),
	   this, SLOT( zeroDC() ) );
  grabKey( Qt::ALT+Qt::Key_Z );

  // plots:
  hb->addWidget( &P );
}


void SetDC::preConfig( void )
{
  setText( "outcurrent", currentOutputNames() );
  setToDefault( "outcurrent" );
  if ( SpikeTrace[0] >= 0 )
    VoltageUnitLabel->setText( trace( SpikeTrace[0] ).unit().c_str() );
}


void SetDC::notify( void )
{
  int outcurrent = index( "outcurrent" );
  if ( outcurrent >= 0 && CurrentOutput[outcurrent] >= 0 ) {
    IUnit = outTrace( CurrentOutput[outcurrent] ).unit();
    setUnit( "dcamplitude", IUnit );
    setUnit( "dcamplitudedecr", IUnit );
    setUnit( "dcamplitudestep", IUnit );
    CurrentUnitLabel1->setText( IUnit.c_str() );
    CurrentUnitLabel2->setText( IUnit.c_str() );
  }
  DCStepIndex = index( "dcamplitudestep" );
  DCStep = DCSteps[DCStepIndex];
  postCustomEvent( 13 ); // setStep();
}


class SetDCEvent : public QEvent
{

public:

  SetDCEvent( double val, double min, double max )
    : QEvent( Type( User+14 ) ),
      Value( val ),
      Min( min ),
      Max( max )
  {
  }
  SetDCEvent( double val, double stdev )
    : QEvent( Type( User+15 ) ),
      Value( val ),
      StDev( stdev )
  {
  }
  SetDCEvent( double val )
    : QEvent( Type( User+16 ) ),
      Value( val )
  {
  }

  double Value;
  double StDev;
  double Min;
  double Max;
};


int SetDC::main( void )
{
  // get options:
  int outcurrent = index( "outcurrent" );
  if ( outcurrent >= 0 && CurrentOutput[outcurrent] >= 0 )
    outcurrent = CurrentOutput[outcurrent];
  int dcamplitudesel = index( "dcamplitudesel" );
  double dcamplitude = number( "dcamplitude" );
  double dcamplitudefrac = number( "dcamplitudefrac" );
  double dcamplitudedecr = number( "dcamplitudedecr" );
  bool interactive = boolean( "interactive" );
  double duration = number( "duration" );
  double update = number( "update" );
  if ( update < 1.0e-3 )
    update = duration;
  ShowStdev = boolean( "showstdev" );

  // don't print repro message:
  noMessage();

  // init:
  lockStimulusData();
  OrgDCAmplitude = stimulusData().number( outTraceName( outcurrent ) );
  unlockStimulusData();
  lockMetaData();
  DCAmplitude = metaData().number( "Cell>ithreshss" );
  unlockMetaData();
  if ( dcamplitudesel == 0 )
    DCAmplitude = dcamplitude;
  else if ( dcamplitudesel == 1 )
    DCAmplitude = OrgDCAmplitude;
  else if ( dcamplitudesel == 2 )
    DCAmplitude *= dcamplitudefrac;
  else
    DCAmplitude -= dcamplitudedecr;

  if ( interactive ) {
    // plot trace:
    tracePlotContinuous( 1.0 );

    // plot
    P.lock();
    P[0].setLMarg( 9.0 );
    P[0].setRMarg( 2.0 );
    P[0].setXLabel( trace( SpikeTrace[0] ).ident() + "(" + trace( SpikeTrace[0] ).unit() + ")"  );
    P[0].setYLabel( "N"  );
    P[0].setYLabelPos( 1.0, Plot::FirstMargin, 0.5, Plot::Graph,
		       Plot::Center, -90.0 );
    P[0].setYRange( 0.0, Plot::AutoScale );

    P[1].setLMarg( 9.0 );
    P[1].setRMarg( 2.0 );
    P[1].setXLabel( "Interspike interval [ms]" );
    P[1].setXRange( 0.0, Plot::AutoScale );
    P[1].setYLabel( "N"  );
    P[1].setYLabelPos( 3.3, Plot::FirstMargin, 0.5, Plot::Graph,
		       Plot::Center, -90.0 );
    P[1].setYRange( 0.0, Plot::AutoScale );
    P.unlock();

    keepFocus();
    analyze( duration );
    OutData dcsignal;
    dcsignal.setTrace( outcurrent );
    dcsignal.constWave( DCAmplitude );
    dcsignal.setIdent( "DC=" + Str( DCAmplitude ) + IUnit );
    directWrite( dcsignal );
    if ( dcsignal.overflow() ) {
      printlog( "Requested DC current I=" + Str( DCAmplitude ) + IUnit + "too high!" );
      DCAmplitude = dcsignal.maxValue();
    }
    else if ( dcsignal.underflow() ) {
      printlog( "Requested DC current I=" + Str( DCAmplitude ) + IUnit + "too small!" );
      DCAmplitude = dcsignal.minValue();
    }
    if ( dcsignal.failed() ) {
      dcsignal.constWave( DCAmplitude );
      dcsignal.setIdent( "DC=" + Str( DCAmplitude ) + IUnit );
      directWrite( dcsignal );
    }
    if ( dcsignal.failed() ) {
      warning( "Failed to write out DC current! " + dcsignal.errorText() );
      return Failed;
    }
    message( "DC=<b>" + Str( DCAmplitude ) + "</b> " + IUnit );
    QCoreApplication::postEvent( this, new SetDCEvent( DCAmplitude,
						       dcsignal.minValue(),
						       dcsignal.maxValue() ) );
    sleep( 0.01 );
    postCustomEvent( 11 ); // setFocus();
    // wait for input:
    SetValue = false;
    Finished = false;
    do {
      bool w = false;
      do {
	w = sleepWait( update );
	analyze( duration );
      } while ( ! w );
      if ( ! Finished || ! SetValue ) {
	if ( Finished )
	  DCAmplitude = OrgDCAmplitude;
	dcsignal.constWave( DCAmplitude );
	dcsignal.setIdent( "DC=" + Str( DCAmplitude ) + IUnit );
	directWrite( dcsignal );
	message( "DC=<b>" + Str( DCAmplitude ) + "</b> " + IUnit );
	sleep( 0.01 );
      }
    } while ( ! Finished && ! interrupt() );
    postCustomEvent( 12 ); // clearFocus();
    if ( interrupt() ) {
      selectText( "dcamplitudestep", DCStepIndex );
      setToDefault( "dcamplitudestep" );
      return Aborted;
    }
  }
  else
    SetValue = true;

  if ( SetValue ) {
    // DC signal:
    OutData dcsignal;
    dcsignal.setTrace( outcurrent );
    dcsignal.constWave( DCAmplitude );
    dcsignal.setIdent( "DC=" + Str( DCAmplitude ) + IUnit );
    directWrite( dcsignal );
    if ( dcsignal.failed() ) {
      DCAmplitude = OrgDCAmplitude;
      dcsignal.constWave( DCAmplitude );
      dcsignal.setIdent( "DC=" + Str( DCAmplitude ) + IUnit );
      directWrite( dcsignal );
    }
    message( "DC=<b>" + Str( DCAmplitude ) + "</b> " + IUnit );
  }

  sleep( 0.01 );
  selectText( "dcamplitudestep", DCStepIndex );
  setToDefault( "dcamplitudestep" );
  return Completed;
}


void SetDC::analyze( double duration )
{
  double ltime = currentTime()-duration;
  if ( ltime < signalTime() ) 
    ltime = signalTime();
  if ( SpikeTrace[0] >= 0 ) {
    const InData &data = trace( SpikeTrace[0] );
    if ( ltime < data.minTime() ) 
      ltime = data.minTime();
    double meanvoltage = data.mean( ltime, currentTime() );
    double stdvoltage = data.stdev( ltime, currentTime() );
    float min = 0;
    float max = 0.0;
    data.minMax( min, max, ltime, currentTime() );
    SampleDataD hist( min, max, 0.01*(max-min), 0.0 );
    double l = hist.rangeFront();
    double s = hist.stepsize();
    for ( int k=data.index( ltime ); k<data.size(); k++ ) {
      int b = (int)rint( ( data[k] - l ) / s );
      if ( b >= 0  && b < hist.size() )
	hist[b] += 1.0;
    }
    P.lock();
    P[0].clear();
    P[0].plot( hist, 1.0, Plot::Transparent, 0, Plot::Solid, Plot::Box, 0, Plot::Green, Plot::Green );
    P[0].draw();
    P.unlock();
    QCoreApplication::postEvent( this, new SetDCEvent( meanvoltage, stdvoltage ) );
  }
  if ( SpikeEvents[0] >= 0 ) {
    const EventData &spikes = events( SpikeEvents[0] );
    double meanrate = spikes.rate( ltime, currentTime() );
    double periodsd = 0.0;
    double period = spikes.interval( ltime, currentTime(), &periodsd );
    double count = spikes.count( ltime, currentTime() );
    double binwidth = count > 10.0 ? 6.0*periodsd/count/0.2 : 0.1*period;
    if ( count > 10.0 && binwidth < 0.02*periodsd )
      binwidth = 0.02*periodsd;
    if ( count > 10.0 && binwidth < 0.0001 )
      binwidth = 0.0001;
    SampleDataD isih( 0.0, 3.0*period, binwidth, 0.0 );
    spikes.intervalHistogram( ltime, currentTime(), isih );
    P.lock();
    P[1].clear();
    P[1].plot( isih, 1000.0, Plot::Transparent, 0, Plot::Solid, Plot::Box, 0, Plot::Yellow, Plot::Yellow );
    P[1].draw();
    P.unlock();
    QCoreApplication::postEvent( this, new SetDCEvent( meanrate ) );
  }
}


void SetDC::setValue( double value )
{
  lock();
  DCAmplitude = value;
  if ( ::fabs( DCAmplitude ) < 1e-5 )
    DCAmplitude = 0.0;
  unlock();
  wake();
}


void SetDC::setValue( void )
{
  lock();
  SetValue = true;
  DCAmplitude = EW->value();
  if ( ::fabs( DCAmplitude ) < 1e-5 )
    DCAmplitude = 0.0;
  Finished = true;
  unlock();
  wake();
}


void SetDC::keepValue( void )
{
  lock();
  SetValue = false;
  Finished = true;
  unlock();
  wake();
}


void SetDC::resetDC( void )
{
  lock();
  double ampl = OrgDCAmplitude;
  unlock();
  EW->setValue( ampl );
  wake();
}


void SetDC::zeroDC( void )
{
  EW->setValue( 0.0 );
  wake();
}


void SetDC::setStep( int index )
{
  DCStepIndex = index;
  DCStep = DCSteps[DCStepIndex];
  EW->setSingleStep( DCStep );
}


void SetDC::keyPressEvent( QKeyEvent *e )
{
  e->ignore();
  if ( e->key() == Qt::Key_Left && ( e->modifiers() & Qt::AltModifier ) ) {
    DCStepIndex++;
    if ( DCStepIndex >= NDCSteps )
      DCStepIndex = NDCSteps-1;
    DCStep = DCSteps[DCStepIndex];
    EW->setSingleStep( DCStep );
    CW->setCurrentIndex( DCStepIndex );
    e->accept();
  }
  else if ( e->key() == Qt::Key_Right && ( e->modifiers() & Qt::AltModifier ) ) {
    DCStepIndex--;
    if ( DCStepIndex < 0 )
      DCStepIndex = 0;
    DCStep = DCSteps[DCStepIndex];
    EW->setSingleStep( DCStep );
    CW->setCurrentIndex( DCStepIndex );
    e->accept();
  }
  else if ( e->key() == Qt::Key_O && ( e->modifiers() & Qt::AltModifier ) ) {
    OKButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_C && ( e->modifiers() & Qt::AltModifier ) ) {
    CancelButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_S && ( e->modifiers() & Qt::AltModifier ) ) {
    ResetButton->animateClick();
    e->accept();
  }
  else if ( e->key() == Qt::Key_Z && ( e->modifiers() & Qt::AltModifier ) ) {
    ZeroButton->animateClick();
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


void SetDC::customEvent( QEvent *qce )
{
  switch ( qce->type() - QEvent::User ) {
  case 11: {
    if ( boolean( "showstep" ) ) {
      DCStepLabel->show();
      CW->show();
      CurrentUnitLabel2->show();
    }
    else {
      DCStepLabel->hide();
      CW->hide();
      CurrentUnitLabel2->hide();
    }
    EW->setFocus( Qt::TabFocusReason );
    connect( EW, SIGNAL( valueChanged( double ) ),
	     this, SLOT( setValue( double ) ) );
    connect( CW, SIGNAL( currentIndexChanged( int ) ),
	     this, SLOT( setStep( int ) ) );
    break;
  }
  case 12: {
    removeFocus();
    disconnect( EW, SIGNAL( valueChanged( double ) ),
		this, SLOT( setValue( double ) ) );
    disconnect( CW, SIGNAL( currentIndexChanged( int ) ),
		this, SLOT( setStep( int ) ) );
    break;
  }
  case 13: {
    EW->setSingleStep( DCStep );
    break;
  }
  case 14: {
    SetDCEvent *sde = dynamic_cast<SetDCEvent*>( qce );
    EW->setMinimum( sde->Min );
    EW->setMaximum( sde->Max );
    EW->setValue( sde->Value );
    CW->setCurrentIndex( DCStepIndex );
    break;
  }
  case 15: {
    SetDCEvent *sde = dynamic_cast<SetDCEvent*>( qce );
    string s = Str( sde->Value, 0, 2, 'f' );
    if ( ShowStdev )
      s += " +/- " + Str( sde->StDev, 0, 2, 'f' );
    VoltageLabel->setText( s.c_str() );
    break;
  }
  case 16: {
    SetDCEvent *sde = dynamic_cast<SetDCEvent*>( qce );
    string s = Str( sde->Value, 0, 1, 'f' );
    RateLabel->setText( s.c_str() );
    break;
  }
  default:
    RePro::customEvent( qce );
  }
}


addRePro( SetDC, patchclamp );

}; /* namespace patchclamp */

#include "moc_setdc.cc"
