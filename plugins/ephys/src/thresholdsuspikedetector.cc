/*
  ephys/thresholdsuspikedetector.cc
  Spike detection based on an absolute voltage threshold.

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

#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPolygon>
#include <QPushButton>
#include <relacs/map.h>
#include <relacs/tablekey.h>
#include <relacs/ephys/thresholdsuspikedetector.h>
using namespace relacs;

namespace ephys {


ThresholdSUSpikeDetector::ThresholdSUSpikeDetector( const string &ident, int mode )
  : Filter( ident, mode, SingleAnalogDetector, 1,
	    "ThresholdSUSpikeDetector", "ephys",
	    "Jan Benda", "1.6", "July 25, 2017" )
{
  // parameter:
  Threshold = 1.0;
  DetectPeaks = true;
  AbsThresh = false;
  TestMaxSize = false;
  MaxSize = 1.0;
  TestWidth = false;
  MaxWidth = 0.001;
  MinWidth = 0.0;
  TestSymmetry = false;
  MaxSymmetry = 1.0;
  MinSymmetry = -1.0;
  TestInterval = false;
  MinInterval = 0.001;
  NSnippets = 100;
  SnippetsWidth = 0.001;
  UpdateTime = 1.0;
  SizeResolution = 0.5;
  ThreshFac = 6.0;
  Unit = "mV";
  Data = 0;

  // options:
  int strongstyle = OptWidget::ValueLarge + OptWidget::ValueBold + OptWidget::ValueGreen + OptWidget::ValueBackBlack;
  newSection( "Detector", 8 );
  addNumber( "threshold", "Detection threshold", Threshold, -2000.0, 2000.0, SizeResolution, Unit, Unit, "%.1f", 2+8 );
  addNumber( "resolution", "Step size for threshold", SizeResolution, 0.0, 1000.0, 0.01, Unit, Unit, "%.3f", 0+8 );
  addNumber( "threshfac", "Factor for estimating detection threshold", ThreshFac, 0.5, 100.0, 0.5, "", "", "%.1f", 0+8 );
  addBoolean( "detectpeaks", "Detect peaks (or troughs if unchecked)", DetectPeaks, 8 );
  addBoolean( "absthresh", "Threshold is absolute voltage (or relative if unchecked)", AbsThresh, 8 );
  newSection( "Tests", 8 );
  addBoolean( "testmaxsize", "Use maximum size", TestMaxSize, 0+8 );
  addNumber( "maxsize", "Maximum size", MaxSize, -10000.0, 10000.0, SizeResolution, Unit, Unit, "%.1f", (TestMaxSize ? 2 : 0)+8 ).setActivation( "testmaxsize", "true" );
  addBoolean( "testwidth", "Use spike-width thresholds", TestWidth, 0+8 );
  addNumber( "maxwidth", "Maximum width of spikes", MaxWidth, 0.0, 0.1, 0.0001, "s", "ms" ).setFlags( (TestWidth ? 2 : 0)+8 ).setActivation( "testwidth", "true" );
  addNumber( "minwidth", "Minimum width of spikes", MinWidth, 0.0, 0.1, 0.0001, "s", "ms" ).setFlags( (TestWidth ? 2 : 0)+8 ).setActivation( "testwidth", "true" );
  addBoolean( "testsymmetry", "Use symmetry thresholds", TestSymmetry, 0+8 );
  addNumber( "maxsymmetry", "Maximum symmetry", MaxSymmetry, -1.0, 1.0, 0.05 ).setFlags( (TestSymmetry ? 2 : 0)+8 ).setActivation( "testsymmetry", "true" );
  addNumber( "minsymmetry", "Minimum symmetry", MinSymmetry, -1.0, 1.0, 0.05 ).setFlags( (TestSymmetry ? 2 : 0)+8 ).setActivation( "testsymmetry", "true" );
  addBoolean( "testisi", "Test interspike interval", TestInterval ).setFlags( 0+8 );
  addNumber( "minisi", "Minimum interspike interval", MinInterval, 0.0, 0.1, 0.0002, "sec", "ms", "%.1f", 0+8 ).setActivation( "testisi", "true" );
  newSection( "Analysis", 8 );
  addNumber( "update", "Update time interval", UpdateTime, 0.2, 1000.0, 0.2, "sec", "sec", "%.1f", 0+8 );
  addInteger( "nsnippets", "Number of spike snippets to be analyzed", NSnippets, 0, 100000, 50 ).setFlags( 0+8 );
  addNumber( "snippetswidth", "Width of spike snippet", SnippetsWidth, 0.0, 1.0, 0.0005, "sec", "ms", "%.1f", 0+8 );
  addNumber( "rate", "Rate", 0.0, 0.0, 100000.0, 0.1, "Hz", "Hz", "%.0f", 0+4 );
  addNumber( "size", "Spike size", 0.0, 0.0, 10000.0, 0.1, Unit, Unit, "%.1f", 2+4, strongstyle );

  setDialogSelectMask( 8 );
  setConfigSelectMask( -8 );

  // main layout:
  QGridLayout *gb = new QGridLayout;
  gb->setContentsMargins( 0, 0, 0, 0 );
  gb->setSpacing( 0 );
  setLayout( gb );

  QVBoxLayout *vbox = new QVBoxLayout;
  gb->addLayout( vbox, 0, 1 );

  // parameter widgets:
  SDW.assign( ((Options*)this), 2, 4, true, 0, mutex() );
  SDW.setMargins( 4, 2, 4, 0 );
  SDW.setVerticalSpacing( 1 );
  vbox->addWidget( &SDW );

  QHBoxLayout *hbox = new QHBoxLayout;
  vbox->addLayout( hbox );
  hbox->addWidget( new QLabel( "" ) );

  // dialog button:
  QPushButton *pb = new QPushButton( "Dialog" );
  hbox->addWidget( pb );
  connect( pb, SIGNAL( clicked( void ) ), this, SLOT( dialog( void ) ) );
  connect( pb, SIGNAL( clicked( void ) ), this, SLOT( removeFocus( void ) ) );
  hbox->addWidget( new QLabel( "" ) );

  // auto configure button:
  pb = new QPushButton( "Auto" );
  hbox->addWidget( pb );
  connect( pb, SIGNAL( clicked( void ) ), this, SLOT( autoConfigure( void ) ) );
  connect( pb, SIGNAL( clicked( void ) ), this, SLOT( removeFocus( void ) ) );
  hbox->addWidget( new QLabel( "" ) );

  LastSize = 0;
  LastTime = 0.0;
  Update.start();

  SP = new Plot( Plot::Copy );
  SP->lock();
  SP->noGrid();
  SP->setTMarg( 4 );
  SP->setBMarg( 2.5 );
  SP->setLMarg( 5 );
  SP->setRMarg( 1 );
  SP->setXLabel( "ms" );
  SP->setXLabelPos( 1.0, Plot::FirstMargin, 0.0, Plot::FirstAxis, Plot::Left, 0.0 );
  SP->setXTics();
  SP->setXRange( -1000.0*SnippetsWidth, 1000.0*SnippetsWidth );
  SP->setYRange( Plot::AutoScale, Plot::AutoScale );
  SP->setYLabel( Unit );
  SP->unlock();
  gb->addWidget( SP, 1, 1 );

  PP1 = new Plot( Plot::Copy );
  PP1->lock();
  PP1->noGrid();
  PP1->setTMarg( 4 );
  PP1->setBMarg( 2.5 );
  PP1->setLMarg( 5 );
  PP1->setRMarg( 1 );
  PP1->setXLabel( Unit );
  PP1->setXLabelPos( 1.0, Plot::FirstMargin, 0.0, Plot::FirstAxis, Plot::Left, 0.0 );
  PP1->setXTics();
  PP1->setYLabel( "Spike symmetry" );
  PP1->setYRange( -1.0, 1.0 );
  PP1->unlock();
  gb->addWidget( PP1, 0, 0 );

  PP2 = new Plot( Plot::Copy );
  PP2->lock();
  PP2->noGrid();
  PP2->setTMarg( 4 );
  PP2->setBMarg( 2.5 );
  PP2->setLMarg( 5 );
  PP2->setRMarg( 1 );
  PP2->setXLabel( Unit );
  PP2->setXLabelPos( 1.0, Plot::FirstMargin, 0.0, Plot::FirstAxis, Plot::Left, 0.0 );
  PP2->setXTics();
  PP2->setYLabel( "Spike width [ms]" );
  PP2->unlock();
  gb->addWidget( PP2, 1, 0 );
}


int ThresholdSUSpikeDetector::init( const InData &data, EventData &outevents,
				    const EventList &other, const EventData &stimuli )
{
  Data = &data;
  Unit = data.unit();
  setOutUnit( "threshold", Unit );
  setOutUnit( "maxsize", Unit );
  setOutUnit( "size", Unit );
  setOutUnit( "resolution", Unit );
  setMinMax( "resolution", 0.0, 1000.0, 0.01, Unit );
  setNotify();
  notify();
  SP->lock();
  SP->setYLabel( Unit );
  SP->unlock();
  PP1->lock();
  PP1->setXLabel( Unit );
  PP1->unlock();
  PP2->lock();
  PP2->setXLabel( Unit );
  PP2->unlock();
  outevents.setSizeScale( 1.0 );
  outevents.setSizeUnit( Unit );
  outevents.setSizeFormat( "%5.1f" );
  outevents.setWidthScale( 1000.0 );
  outevents.setWidthUnit( "ms" );
  outevents.setWidthFormat( "%4.2f" );
  LastTime = 0.0;
  StimulusEnd = 0.0;
  IntervalStart = 0.0;
  IntervalEnd = 0.0;
  IntervalWidth = 0.0;
  D.init( data.begin(), data.end(), data.timeBegin() );
  SDW.updateSettings();
  SDW.updateValues();
  return 0;
}


void ThresholdSUSpikeDetector::readConfig( StrQueue &sq )
{
  unsetNotify(); // we have no unit of the input trace yet!
  Options::read( sq, 0, ":" );
}


void ThresholdSUSpikeDetector::notify( void )
{
  bool tms = boolean( "testmaxsize" );
  if ( tms != TestMaxSize ) {
    TestMaxSize = tms;
    if ( TestMaxSize )
      addFlags( "maxsize", 2 );
    else
      delFlags( "maxsize", 2 );
    postCustomEvent( 12 );
  }
  tms = boolean( "testwidth" );
  if ( tms != TestWidth ) {
    TestWidth = tms;
    if ( TestWidth ) {
      addFlags( "maxwidth", 2 );
      addFlags( "minwidth", 2 );
    }
    else {
      delFlags( "maxwidth", 2 );
      delFlags( "minwidth", 2 );
    }
    postCustomEvent( 12 );
  }
  tms = boolean( "testsymmetry" );
  if ( tms != TestSymmetry ) {
    TestSymmetry = tms;
    if ( TestSymmetry ) {
      addFlags( "maxsymmetry", 2 );
      addFlags( "minsymmetry", 2 );
    }
    else {
      delFlags( "maxsymmetry", 2 );
      delFlags( "minsymmetry", 2 );
    }
    postCustomEvent( 12 );
  }
  Threshold = number( "threshold", Unit );
  MaxSize = number( "maxsize", Unit );
  MaxWidth = number( "maxwidth" );
  MinWidth = number( "minwidth" );
  MaxSymmetry = number( "maxsymmetry" );
  MinSymmetry = number( "minsymmetry" );
  DetectPeaks = boolean( "detectpeaks" );
  AbsThresh = boolean( "absthresh" );
  TestInterval = boolean( "testinterval" );
  MinInterval = number( "minisi" );
  NSnippets = integer( "nsnippets" );
  SpikeTime.clear();
  SpikeTime.free( NSnippets );
  SpikeLeftSize.clear();
  SpikeLeftSize.free( NSnippets );
  SpikeRightSize.clear();
  SpikeRightSize.free( NSnippets );
  SpikeSize.clear();
  SpikeSize.free( NSnippets );
  SpikeSymmetry.clear();
  SpikeSymmetry.free( NSnippets );
  SpikeWidth.clear();
  SpikeWidth.free( NSnippets );
  SpikeAccepted.clear();
  SpikeAccepted.free( NSnippets );
  SnippetsWidth = number( "snippetswidth" );
  SP->lock();
  SP->setXRange( -1000.0*SnippetsWidth, 1000.0*SnippetsWidth );
  SP->unlock();
  UpdateTime = number( "update" );
  ThreshFac = number( "threshfac" );
  double resolution = number( "resolution", Unit );

  if ( changed( "resolution" ) && resolution > 0.0 ) {
    if ( resolution < 0.001 ) {
      resolution = 0.001;
      setNumber( "resolution", resolution, Unit );
    }
    SizeResolution = resolution;
    // necessary precision:
    int pre = -1;
    do {
      pre++;
      double f = ::pow( 10.0, -pre );
      resolution -= floor( 1.001*resolution/f ) * f;
    } while ( pre < 3 && fabs( resolution ) > 1.0e-3 );
    setFormat( "threshold", 4+pre, pre, 'f' );
    setStep( "threshold", SizeResolution, Unit );
    setFormat( "maxsize", 4+pre, pre, 'f' );
    setStep( "maxsize", SizeResolution, Unit );
    setFormat( "size", 4+pre, pre, 'f' );
    SDW.updateSettings();
  }
  SDW.updateValues( OptWidget::changedFlag() );
}


void ThresholdSUSpikeDetector::autoConfigure( void )
{
  if ( Data == 0 )
    return;
  if ( AbsThresh ) {
    double th = 0.5 * ThreshFac * Data->stdev( Data->currentTime()-UpdateTime, Data->currentTime() );
    double a = Data->mean( Data->currentTime()-UpdateTime, Data->currentTime() );
    if ( DetectPeaks )
      Threshold = a + th;
    else
      Threshold = a - th;
  }
  else
    Threshold = ThreshFac * Data->stdev( Data->currentTime()-UpdateTime, Data->currentTime() );
  unsetNotify();
  setNumber( "threshold", Threshold, Unit );
  setNotify();
  SDW.updateValues( OptWidget::changedFlag() );
}


int ThresholdSUSpikeDetector::autoConfigure( const InData &data,
					   double tbegin, double tend )
{
  autoConfigure();
  return 0;
}


void ThresholdSUSpikeDetector::save( const string &param )
{
  save();
}


void ThresholdSUSpikeDetector::save( void )
{
  // create file:
  ofstream df( addPath( Str( ident() ).lower() + "-properties.dat" ).c_str(),
	       ofstream::out | ofstream::app );
  if ( ! df.good() )
    return;

  lock();

  // write header and key:
  Options header;
  header.addText( "ident", ident() );
  header.addText( "detector", name() );
  header.addText( "session time", sessionTimeStr() );
  header.newSection( *this, "Settings" );
  header.save( df, "# " );
  df << '\n';
  TableKey key;
  key.addNumber( "time", "ms", "%7.2f" );
  key.addNumber( "leftsize", Unit, "%6.1f" );
  key.addNumber( "rightsize", Unit, "%6.1f" );
  key.addNumber( "size", Unit, "%6.1f" );
  key.addNumber( "symmetry", "1", "%5.3f" );
  key.addNumber( "width", "ms", "%6.3f" );
  key.addNumber( "accepted", "1", "%1.0f" );
  key.saveKey( df, true, false );

  double t0 = SpikeTime[SpikeTime.size() - SpikeTime.accessibleSize()];
  for ( int k=0; k<SpikeTime.accessibleSize(); k++ ) {
    int i = SpikeTime.size() - SpikeTime.accessibleSize() + k;
    key.save( df, 1000.0*(SpikeTime[i] - t0), 0 );
    key.save( df, SpikeLeftSize[i] );
    key.save( df, SpikeRightSize[i] );
    key.save( df, SpikeSize[i] );
    key.save( df, SpikeSymmetry[i] );
    key.save( df, 1000.0*SpikeWidth[i] );
    double accepted = SpikeAccepted[i] ? 1.0 : 0.0;
    key.save( df, accepted );
    df << '\n';
  }
  df << '\n' << '\n';
  
  unlock();
}


int ThresholdSUSpikeDetector::detect( const InData &data, EventData &outevents,
				      const EventList &other, const EventData &stimuli )
{
  if ( DetectPeaks ) {
    if ( AbsThresh )
      D.rising( data.minBegin(), data.end(), outevents,
		Threshold, Threshold, Threshold, *this );
    else
      D.peak( data.minBegin(), data.end(), outevents,
	      Threshold, Threshold, Threshold, *this );
  }
  else {
    if ( AbsThresh )
      D.falling( data.minBegin(), data.end(), outevents,
		 Threshold, Threshold, Threshold, *this );
    else
      D.trough( data.minBegin(), data.end(), outevents,
		Threshold, Threshold, Threshold, *this );
  }

  unsetNotify();
  setNumber( "rate", outevents.meanRate() );
  setNumber( "size", outevents.meanSize(), Unit );
  setNotify();

  // update indicator widgets:
  if ( Update.elapsed()*0.001 < UpdateTime )
    return 0;
  Update.start();

  SDW.updateValues( OptWidget::changedFlag() );

  // snippets:
  SP->lock();
  SP->clear();
  SP->plotVLine( 0.0, Plot::White, 2 );
  if ( AbsThresh && TestMaxSize )
    SP->plotHLine( MaxSize, Plot::White, 2 );
  SampleDataF meansnippet( -SnippetsWidth, SnippetsWidth, data.stepsize(), 0.0f );
  int n = 0;
  for ( int k=SpikeTime.minIndex(); k<SpikeTime.size(); k++ ) {
    double st = SpikeTime[k];
    if ( st - SnippetsWidth <= data.minTime() )
      continue;
    if ( st + SnippetsWidth > data.currentTime() )
      break;
    SampleDataF snippet( -SnippetsWidth, SnippetsWidth, data.stepsize(), 0.0f );
    data.copy( st, snippet );
    if ( ! DetectPeaks )
      snippet *= -1.0;
    if ( ! AbsThresh ) {
      int k0 = snippet.index( 0.0 );
      snippet -= snippet[k0];
    }
    int color = Plot::Red;
    if ( SpikeAccepted[k] ) {
      meansnippet += (snippet - meansnippet)/(++n);
      color = Plot::Yellow;
    }
    SP->plot( snippet, 1000.0, color, 1, Plot::Solid );
  }
  SP->plot( meansnippet, 1000.0, Plot::Orange, 2, Plot::Solid );
  SP->draw();
  SP->unlock();

  // plot snippet properties:
  ArrayD spikesizeaccepted;
  ArrayD spikesizediscarded;
  ArrayD symmetryaccepted;
  ArrayD symmetrydiscarded;
  ArrayD widthaccepted;
  ArrayD widthdiscarded;
  for ( int k=0; k<SpikeSize.accessibleSize(); k++ ) {
    int i = SpikeSize.size() - SpikeSize.accessibleSize() + k;
    if ( SpikeAccepted[i] ) {
      spikesizeaccepted.push( SpikeSize[i] );
      symmetryaccepted.push( SpikeSymmetry[i] );
      widthaccepted.push( 1000.0*SpikeWidth[i] );
    }
    else {
      spikesizediscarded.push( SpikeSize[i] );
      symmetrydiscarded.push( SpikeSymmetry[i] );
      widthdiscarded.push( 1000.0*SpikeWidth[i] );
    }
  }

  PP1->lock();
  PP1->clear();
  if ( TestMaxSize )
    PP1->plotVLine( MaxSize, Plot::White, 2, Plot::Solid );
  if ( TestSymmetry ) {
    PP1->plotHLine( MaxSymmetry, Plot::White, 2, Plot::Solid );
    PP1->plotHLine( MinSymmetry, Plot::White, 2, Plot::Solid );
  }
  PP1->plot( spikesizediscarded, symmetrydiscarded, Plot::Transparent, 0, Plot::Solid,
	     Plot::Circle, 3, Plot::Red, Plot::Red );
  PP1->plot( spikesizeaccepted, symmetryaccepted, Plot::Transparent, 0, Plot::Solid,
	     Plot::Circle, 3, Plot::Yellow, Plot::Yellow );
  PP1->draw();
  PP1->unlock();

  PP2->lock();
  PP2->clear();
  if ( TestMaxSize )
    PP2->plotVLine( MaxSize, Plot::White, 2, Plot::Solid );
  if ( TestWidth ) {
    PP2->plotHLine( 1000.0*MaxWidth, Plot::White, 2, Plot::Solid );
    PP2->plotHLine( 1000.0*MinWidth, Plot::White, 2, Plot::Solid );
  }
  PP2->plot( spikesizediscarded, widthdiscarded, Plot::Transparent, 0, Plot::Solid,
	     Plot::Circle, 3, Plot::Red, Plot::Red );
  PP2->plot( spikesizeaccepted, widthaccepted, Plot::Transparent, 0, Plot::Solid,
	     Plot::Circle, 3, Plot::Yellow, Plot::Yellow );
  PP2->draw();
  PP2->unlock();

  return 0;
}


int ThresholdSUSpikeDetector::checkEvent( InData::const_iterator first, 
					  InData::const_iterator last,
					  InData::const_iterator event, 
					  InData::const_range_iterator eventtime, 
					  InData::const_iterator index,
					  InData::const_range_iterator indextime, 
					  InData::const_iterator prevevent, 
					  InData::const_range_iterator prevtime, 
					  EventData &outevents,
					  double &threshold,
					  double &minthresh, double &maxthresh,
					  double &time, double &size, double &width )
{ 
  InData::const_iterator left = event;
  InData::const_iterator right = event;
  if ( AbsThresh ) {
    if ( DetectPeaks ) {
      // go to the next local maximum:
      double max = *right;
      for ( ; *right >= threshold; ++right ) {
	if ( *right > max ) {
	  max = *right;
	  event = right;
	  eventtime = right;
	}
	if ( right + 1 >= last )
	  return -1;
      }
    }
    else {
      // go to the next local minimum:
      double min = *right;
      for ( ; *right <= threshold; ++right ) {
	if ( *right < min ) {
	  min = *right;
	  event = right;
	  eventtime = right;
	}
	if ( right + 1 >= last )
	  return -1;
      }
    }
  }

  // time of spike:
  time = *eventtime;
  size = fabs( *event );

  // same as before:
  if ( outevents.back() >= time - 1e-8 )
    return 0;

  // right size:
  double rightsize = 0.0;
  for ( ++right; ; ++right ) {
    if ( right+2 >= last )
      return -1;
    if ( DetectPeaks ) {
      if ( *(right+2) > *right && *(right+1) > *right &&
	   *(right-2) > *right && *(right-1) > *right ) {
	rightsize = *event - *right;
	break;
      }
    }
    else {
      if ( *(right+2) < *right && *(right+1) < *right &&
	   *(right-2) < *right && *(right-1) < *right ) {
	rightsize = *right - *event;
	break;
      }
    }
  }
  // left size:
  double leftsize = 0.0;
  for ( --left; ; --left ) {
    if ( left-2 < first )
      return 0;
    if ( DetectPeaks ) {
      if ( *(left+2) > *left && *(left+1) > *left &&
	   *(left-2) > *left && *(left-1) > *left ) {
	leftsize = *event - *left;
	break;
      }
    }
    else {
      if ( *(left+2) < *left && *(left+1) < *left &&
	   *(left-2) < *left && *(left-1) < *left ) {
	leftsize = *left - *event;
	break;
      }
    }
  }

  // size:
  if ( ! AbsThresh ) {
    size = leftsize;
    if ( size > rightsize )
      size = rightsize;
  }

  // symmetry:
  double symmetry = ( leftsize - rightsize )/( leftsize + rightsize );

  // width:
  double thresh = DetectPeaks ? *event - 0.5*size :  *event + 0.5*size;
  // right width:
  InData::const_iterator rightw = event;
  double pv = *rightw;
  for ( ++rightw; rightw < right; ++rightw ) {
    if ( DetectPeaks ) {
      if ( *rightw < thresh )
	break;
    }
    else {
      if ( *rightw > thresh )
	break;
    }
    pv = *rightw;
  }
  double rdt = Data->stepsize()*(thresh - *rightw)/(pv - *rightw);
  // left width:
  InData::const_iterator leftw = event;
  pv = *leftw;
  for ( --leftw; leftw > left; --leftw ) {
    if ( DetectPeaks ) {
      if ( *leftw < thresh )
	break;
    }
    else {
      if ( *leftw > thresh )
	break;
    }
    pv = *leftw;
  }
  double ldt = Data->stepsize()*(thresh - *leftw)/(pv - *leftw);
  width = Data->interval( rightw - leftw ) - ldt - rdt;

  // check:
  bool accept = true;
  if ( TestMaxSize && size > MaxSize )
    accept = false;
  if ( TestWidth && ( width > MaxWidth || width < MinWidth ) )
    accept = false;
  if ( TestSymmetry && ( symmetry > MaxSymmetry || symmetry < MinSymmetry ) )
    accept = false;
  if ( TestInterval && 
       outevents.size() > 0 && time - outevents.back() < MinInterval )
    accept = false;

  // store:
  SpikeTime.push( time );
  SpikeLeftSize.push( leftsize );
  SpikeRightSize.push( rightsize );
  SpikeSize.push( size );
  SpikeSymmetry.push( symmetry );
  SpikeWidth.push( width );
  SpikeAccepted.push( accept );

  return accept; 
}


void ThresholdSUSpikeDetector::customEvent( QEvent *qce )
{
  if ( qce->type() == QEvent::User+12 ) {
    SDW.assign( ((Options*)this), 2, 4, true, 0, mutex() );
  }
  else
    Filter::customEvent( qce );
}


addDetector( ThresholdSUSpikeDetector, ephys );

}; /* namespace ephys */

#include "moc_thresholdsuspikedetector.cc"
