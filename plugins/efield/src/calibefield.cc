/*
  efield/calibefield.cc
  Calibrates an attenuator for electric field stimuli.

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

#include <fstream>
#include <iomanip>
#include <relacs/str.h>
#include <relacs/options.h>
#include <relacs/tablekey.h>
#include <relacs/efield/calibefield.h>
using namespace relacs;

namespace efield {


CalibEField::CalibEField( void )
  : RePro( "CalibEField", "efield",
	   "Jan Benda", "2.4", "May 29, 2017" )
{
  // add some parameter as options:
  newSection( "General" );
  addBoolean( "reset", "Reset calibration?", false );
  addNumber( "resetval", "Reset gain factor to", 0.1, 0.0, 100.0, 0.01 ).setActivation( "reset", "true" );
  addBoolean( "am", "Calibrate amplitude modulation?", false );
  addNumber( "beatfreq", "Beat frequency to be used when fish EOD present", 20.0, 1.0, 100.0, 1.0, "Hz" );
  addNumber( "frequency", "Stimulus frequency to be used when no fish EOD is present", 600.0, 10.0, 1000.0, 5.0, "Hz" );
  addNumber( "duration", "Duration of stimulus", 0.4, 0.0, 10.0, 0.05, "seconds", "ms" );
  addNumber( "pause", "Pause", 0.0, 0.0, 10.0, 0.05, "seconds", "ms" );
  newSection( "Range" );
  addSelection( "amplsel", "Calibrate for", "contrast|amplitude" );
  addNumber( "targetcontrast", "Target contrast to be tested first", 0.2, 0.01, 1.0, 0.05, "", "%" ).setActivation( "amplsel", "contrast" );
  addNumber( "mincontrast", "Minimum contrast", 0.1, 0.01, 1.0, 0.05, "", "%" ).setActivation( "amplsel", "contrast" );
  addNumber( "maxcontrast", "Maximum contrast", 0.25, 0.01, 1.0, 0.05, "", "%" ).setActivation( "amplsel", "contrast" );
  addNumber( "targetamplitude", "Target contrast to be tested first", 1.0, 0.001, 1000.0, 0.1, "mV/cm" ).setActivation( "amplsel", "amplitude" );
  addNumber( "minamplitude", "Minimum amplitude", 0.5, 0.0, 1000.0, 0.1, "mV/cm" ).setActivation( "amplsel", "amplitude" );
  addNumber( "maxamplitude", "Maximum amplitude", 2.0, 0.0, 1000.0, 0.1, "mV/cm" ).setActivation( "amplsel", "amplitude" );
  addInteger( "numintensities", "Number of intensities (amplitudes) to be measured", 10, 4, 1000, 2 );

  // plot:
  setWidget( &P );    
}


CalibEField::~CalibEField( void )
{
}


int CalibEField::main( void )
{
  // get options:
  bool reset = boolean( "reset" );
  double resetval = number( "resetval" );
  bool am = boolean( "am" );
  double frequency = number( "frequency" );
  double beatfrequency = number( "beatfreq" );
  double duration = number( "duration" );
  double pause = number( "pause" );
  bool usecontrast = ( index( "amplsel" ) == 0 );
  double targetcontrast = number( "targetcontrast" );
  double maxcontrast = number( "maxcontrast" );
  double mincontrast = number( "mincontrast" );
  double targetamplitude = number( "targetamplitude" );
  double maxamplitude = number( "maxamplitude" );
  double minamplitude = number( "minamplitude" );
  int numintensities = integer( "numintensities" );

  int outtrace = am ? GlobalAMEField : GlobalEField;

  if ( duration*beatfrequency < 4 ) {
    warning( "stimulus too short or beat frequency too low." );
    return Failed;
  }

  // attenuator:
  base::LinearAttenuate *latt = 
    dynamic_cast<base::LinearAttenuate*>( attenuator( outTraceName( outtrace ) ) );
  if ( latt == 0 ) {
    warning( "No Attenuator plugin found!" );
    return Failed;
  }

  // check for analog input traces:
  int eodtraceindices[3] = { LocalEODTrace[0], EODTrace, FishEODTrace[0][0] };
  int eodeventsindices[3] = { LocalEODEvents[0], EODEvents, FishEODEvents[0][0] };
  int eodtraceinx = -1;
  int eodeventsinx = -1;
  for ( int k=0; k<3; k++ ) {
    eodtraceinx = eodtraceindices[k];
    eodeventsinx = eodeventsindices[k];
    if ( eodtraceinx >= 0 )
      break;
  }
  if ( eodtraceinx < 0 ) {
    warning( "No EOD recording available!" );
    return Failed;
  }

  // data:
  const InData &eodtrace = trace( eodtraceinx );
  bool fish = false;
  if ( eodeventsinx >= 0 ) {
    const EventData &eodevents = events( eodeventsinx );
    fish = ( eodevents.count( eodevents.rangeBack() - 1.0 ) > 100 );
  }
  message( fish ? "There IS a fish EOD" : "NO fish EOD present" );
  double fishrate = 0.0;
  double fishamplitude = 0.0;
  if ( fish ) {
    const EventData &eodevents = events( eodeventsinx );
    fishrate = eodevents.frequency( eodevents.rangeBack() - 0.5 );
    if ( duration * beatfrequency < 6 ) {
      double newbeatfrequency = ceil( 10.0 / duration );
      double newduration = 6.0/beatfrequency;
      Str s = "Not enough beat periods! <br>You should set the beat frequency to "
	+ Str( newbeatfrequency ) + "Hz.<br>Alternatively, you should increase the stimulus duration to at least "
	+ Str( 1000.0*newduration ) + "ms";
      warning( s, 4.0 );
      return Failed;
    }
    if ( am )
      frequency = beatfrequency;
    else
      frequency = fishrate + beatfrequency;

    // mean EOD amplitude:
    fishamplitude = eodAmplitude( eodtrace,
				  currentTime() - 0.5, currentTime() );
    Str s = "EOD amplitude = " + Str( fishamplitude );
    message( s );
    if ( ! usecontrast ) {
      mincontrast = minamplitude / fishamplitude;
      maxcontrast = maxamplitude / fishamplitude;
    }
  }
  else {
    if ( usecontrast ) {
      warning( "Need fish EOD for calibrating contrasts!" );
      return Failed;
    }
    if ( am ) {
      warning( "Need fish EOD for calibrating amplitude modulation!" );
      return Failed;
    }
  }
  EODUnit = eodtrace.unit();

  double origgain = latt->gain();
  double origoffset = latt->offset();
  FitGain = 1.0;
  FitOffset = 0.0;

  if ( reset ) {
    message( "reset gain to " + Str( resetval ) );
    latt->setGain( resetval, 0.0 );
  }

  // plot trace:
  tracePlotSignal( duration );

  // adjust local EOD:
  if ( usecontrast ) {
    double val2 = eodtrace.maxAbs( currentTime()-0.1, currentTime() );
    if ( val2 > 0.0 )
      adjustGain( eodtrace, ( 1.0 + maxcontrast ) * val2 );
  }
  else
    adjustGain( eodtrace, maxamplitude + fishamplitude );

  // plot:
  P.lock();
  P.clear();
  P.resetRanges();
  P.setXLabel( "Gain" );
  string ylabel = am ? "Measured AM Amplitude" : "Measured EOD Amplitude";
  ylabel += " [" + EODUnit + "]";
  P.setYLabel( ylabel );
  P.unlock();

  // phase 1: find right gain to get target contrast/amplitude: ////////////////////
  double maxsignal = 0.0;
  double targetintensity = 0.0;
  if ( usecontrast )
    targetintensity = targetcontrast * fishamplitude;
  else
    targetintensity = targetamplitude;
  // make sure the intensity is valid:
  double gain = latt->gain();
  double maxgain = ( ::pow( 10.0, -minLevel( outtrace )/20.0 ) )/targetintensity;
  double mingain = ( ::pow( 10.0, -maxLevel( outtrace )/20.0 ) )/targetintensity;
  if ( targetintensity > maxIntensity( outtrace ) ) {
    gain = 0.5*maxgain;
    latt->setGain( gain, 0.0 );
    printlog( "Set gain to " + Str( gain ) + " to make targetintensity " +
	      Str( targetintensity ) + " smaller than maximum possible intensity of " +
	      Str( maxIntensity( outtrace ) ) );
  }
  else if ( targetintensity < minIntensity( outtrace ) ) {
    gain = 2.0*mingain;
    latt->setGain( gain, 0.0 );
    printlog( "Set gain to " + Str( gain ) + " to make targetintensity " +
	      Str( targetintensity ) + " greater than minimum possible intensity of " +
	      Str( minIntensity( outtrace ) ) );
  }
  double gainfac = sqrt(10.0);
  int gaindir = 0;
  bool bisect = false;
  MapD gainamplitudes;
  gainamplitudes.reserve( 100 );

  // stimulus:
  OutData signal;
  signal.setTrace( outtrace );
  signal.sineWave( duration, -1.0, frequency, 0.0, 1.0 );
  signal.setIntensity( targetintensity );
  signal.back() = 0;

  while ( softStop() == 0 ) {

    // write stimulus:
    write( signal );
    if ( signal.failed() ) {
      warning( "Failed to write out signal!" );
      latt->setGain( origgain, origoffset );
      writeZero( outtrace );
      return Failed;
    }

    sleep( pause );
    if ( interrupt() ) {
      latt->setGain( origgain, origoffset );
      writeZero( outtrace );
      return Aborted;
    }

    // adjust signal analog input gains:
    if ( !am && GlobalEFieldTrace >= 0 ) {
      double max = trace( GlobalEFieldTrace ).maxAbs( signalTime(), signalTime()+duration );
      if ( maxsignal < max ) {
	maxsignal = max;
	adjustGain( trace( GlobalEFieldTrace ), 1.1 * maxsignal );
      }
    }

    // analyze amplitude:
    double amplitude = 0.0;
    int r = analyze( eodtrace, duration, beatfrequency,
		     usecontrast, mincontrast, maxcontrast,
		     minamplitude, maxamplitude,
		     targetintensity, fish, amplitude );
    gainamplitudes.push( latt->gain(), amplitude );

    Str s = "Phase <b>1</b>: Tried <b>" + Str( signal.intensity(), 0, 3, 'g' ) + EODUnit + "</b>";
    s += ",  Measured <b>" + Str( amplitude, 0, 3, 'g' ) + EODUnit + "</b>";
    s += " at gain <b>" + Str( latt->gain(), 0, 5, 'g' ) + "</b>";
    message( s );

    if ( amplitude < 0.01 * eodtrace.maxValue() ) {
      latt->setGain( origgain, origoffset );
      warning( "Did not record signal on measurement electrodes! Amplitude=" + Str( amplitude )
	       + " is smaller than 1% of the maximum range of the " + eodtrace.ident()
	       + " trace (=" + Str( eodtrace.maxValue() ) + ").<br>Increase the input gain." );
      return Failed;
    }

    plotGain( gainamplitudes, targetintensity );

    // next stimulus:
    if ( r==0 && fabs( amplitude - targetintensity )/targetintensity < 0.05 ) {
      printlog( "Finished phase 1" );
      break;
    }
    else {
      if ( amplitude > targetintensity || r == 2 ) {
	// reduces gain:
	if ( gain <= mingain ) {
	  latt->setGain( origgain, origoffset );
	  warning( "Cannot increase attenuator level any further!<br>Requested stimulus intensity is too low." );
	  return Failed;
	}
	if ( gaindir == 1 )
	  bisect = true;
	if ( bisect )
	  gainfac = sqrt( gainfac );
	gain /= gainfac;
	if ( gain < mingain )
	  gain = mingain;
	latt->setGain( gain, 0.0 );
	printlog( "Set new gain to " + Str( gain ) );
	gaindir = -1;
      }
      else {
	if ( gain >= maxgain ) {
	  latt->setGain( origgain, origoffset );
	  warning( "Cannot decrease attenuator level any further!<br>Requested stimulus intensity is too high." );
	  return Failed;
	}
	// increase gain:
	if ( gaindir == -1 )
	  bisect = true;
	if ( bisect )
	  gainfac = sqrt( gainfac );
	gain *= gainfac;
	if ( gain > maxgain )
	  gain = maxgain;
	latt->setGain( gain, 0.0 );
	printlog( "Set new gain to " + Str( gain ) );
	gaindir = 1;
      }
    }

  }

  // phase 2&3: Measure whole intensity range: ////////////////////
  int phase = 2;
  MapD intensities;
  intensities.reserve( 100 );

  double maxintensity = 0.0;
  double minintensity = 0.0;
  if ( usecontrast ) {
    maxintensity = maxcontrast * fishamplitude;
    minintensity = mincontrast * fishamplitude;
  }
  else {
    maxintensity = maxamplitude;
    minintensity = minamplitude;
  }
  if ( maxintensity > maxIntensity( outtrace ) )
    maxintensity = maxIntensity( outtrace );
  if ( minintensity < minIntensity( outtrace ) )
    minintensity = minIntensity( outtrace );
  if ( maxintensity <= minintensity ) {
    latt->setGain( origgain, origoffset );
    warning( "Requested intensity range not possible!" );
    writeZero( outtrace );
    return Failed;
  }
  double intensitystep = (maxintensity - minintensity)/(numintensities-1);
  double intensity = minintensity;
  int intensitycount = 0;

  // plot:
  P.lock();
  P.resetRanges();
  P.unlock();

  while ( softStop() == 0 ) {

    // write stimulus:
    signal.setIntensity( intensity );
    write( signal );
    if ( signal.failed() ) {
      warning( "Failed to write out signal!" );
      latt->setGain( origgain, origoffset );
      writeZero( outtrace );
      return Failed;
    }

    sleep( pause );
    if ( interrupt() ) {
      saveData( intensities, latt );
      writeZero( outtrace );
      return Aborted;
    }

    // adjust signal analog input gains:
    if ( !am && GlobalEFieldTrace >= 0 ) {
      double max = trace( GlobalEFieldTrace ).maxAbs( signalTime(), signalTime()+duration );
      if ( maxsignal < max ) {
	maxsignal = max;
	adjustGain( trace( GlobalEFieldTrace ), 1.1 * maxsignal );
      }
    }

    // setup signal eod detector:
    if ( GlobalEFieldTrace >= 0 && GlobalEFieldEvents >= 0 ) {
      double max = trace( GlobalEFieldTrace ).maxValue();
      detectorEventsOpts( GlobalEFieldEvents ).setMinMax( "threshold", 0.01 * max, max, 0.01*max );
      detectorEventsOpts( GlobalEFieldEvents ).setNumber( "threshold", 0.1 * max );
    }

    double amplitude = 0.0;
    int r = analyze( eodtrace, duration, beatfrequency, usecontrast, mincontrast, maxcontrast,
		     minamplitude, maxamplitude, intensity, fish, amplitude );

    Str s = "Phase <b>" + Str( phase ) + "</b>";
    s += ":  Tried <b>" + Str( signal.intensity(), 0, 3, 'g' ) + EODUnit + "</b>";
    s += ",  Measured <b>" + Str( amplitude, 0, 3, 'g' ) + EODUnit + "</b>";
    message( s );

    if ( r == 0 ) {
      // store data:
      intensities.push( intensity, amplitude );
      if ( intensities.size() >= 2 ) {
	// fit straight line:
	/*
	  double b[2];
	  intensities->lineFit( b );
	  FitOffset = b[0];
	  FitGain = b[1];
	*/
	double sxx=0.0, sxy=0.0;
	for ( int i=0; i<intensities.size(); i++ ) {
	  sxx += intensities.x(i) * intensities.x(i);
	  sxy += intensities.x(i) * intensities.y(i);
	}
	FitOffset = 0.0;
	FitGain = sxy/sxx;
      }
    }

    plotIntensities( intensities, targetintensity, maxintensity );

    // next stimulus:
    intensitycount++;
    if ( intensitycount >= numintensities || r > 0 ) {
      // response too strong?
      if ( r == 2 ) { 
	if ( intensitycount < 4 || intensitycount < numintensities/4 ) {
	  printlog( "stop increasing intensity." );
	  r = 0;
	}
	else {
	  intensitystep *= 0.5;
	  minintensity *= 0.5;
	  maxintensity *= 0.5;
	  printlog( "divided intensity range by two. minintensity=" + Str( minintensity ) + ", maxintensity=" + Str( maxintensity ) );
	}
      }
      // response too weak?
      else if ( r == 1 || 
		( intensitycount >= 4 && fabs( FitGain ) < 1.0e-6 ) ) {
	intensitystep *= 2.0;
	minintensity *= 2.0;
	maxintensity *= 2.0;
	printlog( "multiplied intensity range by two. minintensity=" + Str( minintensity ) + ", maxintensity=" + Str( maxintensity ) );
	if ( maxintensity > maxIntensity( outtrace ) ) {
	  minintensity = intensity + intensitystep;
	  maxintensity = maxIntensity( outtrace );
	  printlog( "prevented overflow: new intensity range minintensity=" + Str( minintensity ) + ", maxintensity=" + Str( maxintensity ) );
	}
      }
      // fit totally failed:
      else if ( FitGain <= 0.0 ) {
	warning( "Negative slope. <br>Exit." );
	latt->setGain( origgain, origoffset );
	writeZero( outtrace );
	return Failed;
      }
      // set new parameter:
      if ( r == 0 ) {
	double offset = latt->offset() - FitOffset * latt->gain() / FitGain;
	double gain = latt->gain() / FitGain;
	latt->setGain( gain, offset );
	Str s = "new gain = " + Str( gain );
	s += ",  new offset = " + Str( offset );
	message( s );
	// reset intensity ranges:
	if ( usecontrast ) {
	  maxintensity = maxcontrast * fishamplitude;
	  minintensity = mincontrast * fishamplitude;
	}
	else {
	  maxintensity = maxamplitude;
	  minintensity = minamplitude;
	}
	if ( maxintensity > maxIntensity( outtrace ) )
	  maxintensity = maxIntensity( outtrace );
	if ( minintensity < minIntensity( outtrace ) )
	  minintensity = minIntensity( outtrace );
	if ( maxintensity <= minintensity ) {
	  latt->setGain( origgain, origoffset );
	  warning( "Requested intensity range not possible!", 4.0 );
	  writeZero( outtrace );
	  return Failed;
	}
	printlog( "New intensity range: minintensity=" + Str( minintensity ) + ", maxintensity=" + Str( maxintensity ) );
	// next phase:
	phase++;
	if ( phase == 3 ) {
	  // increase resolution:
	  duration *= 2.0;
	  signal.setTrace( outtrace );
	  signal.sineWave( duration, -1.0, frequency, 0.0, 1.0 );
	  signal.back() = 0;
	  numintensities *= 2;
	  intensitystep *= 0.5;
	  tracePlotSignal( duration );
	  FitGain = 1.0;
	  FitOffset = 0.0;
	}
	else {
	  // ready!
	  saveData( intensities, latt );
	  writeZero( outtrace );
	  return Completed;
	}
      }
      intensitystep = (maxintensity - minintensity)/(numintensities-1);
      intensity = minintensity;
      intensitycount = 0;
      intensities.clear();
      printlog( "new intensity range. minintensity=" + Str( minintensity ) + ", maxintensity=" + Str( maxintensity ) + ", intensitystep=" + Str( intensitystep ) );
    }
    else
      intensity += intensitystep;
  }

  saveData( intensities, latt );
  writeZero( outtrace );
  return Completed;
}


void CalibEField::saveData( const MapD &intensities,
			    const base::LinearAttenuate *latt )
{
  if ( intensities.empty() )
    return;

  // create file:
  ofstream df( addPath( "calibrate.dat" ).c_str(),
	       ofstream::out | ofstream::app );
  if ( ! df.good() )
    return;

  // write header and key:
  Options header;
  header.addInteger( "device", latt->aoDevice() );
  header.addInteger( "channel", latt->aoChannel() );
  header.addText( "session time", sessionTimeStr() );
  header.newSection( settings() );
  header.save( df, "# ", 0, Options::FirstOnly );
  df << '\n';
  TableKey key;
  key.addNumber( "intens", EODUnit, "%8.5g" );
  key.addNumber( "measured", EODUnit, "%8.5g" );
  key.saveKey( df, true, false );

  // write data:
  for ( int k=0; k<intensities.size(); k++ ) {
    key.save( df, intensities.x(k), 0 );
    key.save( df, intensities.y(k), 1 );
    df << '\n';
  }
  df << '\n' << '\n';
}


void CalibEField::plotGain( const MapD &gainamplitudes, double targetintensity )
{
  P.lock();
  P.clear();
  if ( ! P.zoomedXRange() )
    P.setXRange( Plot::AutoScale, Plot::AutoScale );
  double ymax = max( gainamplitudes.y() );
  if ( ymax < targetintensity )
    ymax = targetintensity;
  if ( ! P.zoomedYRange() )
    P.setYRange( 0.0, ymax );
  P.plotHLine( targetintensity, Plot::White, 4 );
  P.plot( gainamplitudes, 1.0, Plot::Transparent, 1, Plot::Solid, Plot::Circle, 10, Plot::Red, Plot::Red );
  P.draw();
  P.unlock();
}


void CalibEField::plotIntensities( const MapD &intensities, double targetintensity, double maxx )
{
  P.lock();
  P.clear();
  P.setXLabel( "Requested Intensity [" + EODUnit + "]" );
  if ( ! P.zoomedXRange() )
    P.setXRange( 0.0, maxx );
  if ( ! P.zoomedYRange() )
    P.setYRange( 0.0, maxx*FitGain+FitOffset );
  //  P.setYRange( 0.0, Plot::AutoScale );
  P.plotHLine( targetintensity, Plot::White, 4 );
  P.plotLine( 0.0, 0.0, maxx, maxx, Plot::Blue, 4 );
  P.plotLine( 0.0, FitOffset, maxx, maxx*FitGain+FitOffset, Plot::Yellow, 2 );
  P.plot( intensities, 1.0, Plot::Transparent, 1, Plot::Solid, Plot::Circle, 10, Plot::Red, Plot::Red );
  P.draw();
  P.unlock();
}


int CalibEField::analyze( const InData &eodtrace,
			  double duration, double beatfrequency,
			  bool usecontrast, double mincontrast, double maxcontrast,
			  double minamplitude, double maxamplitude,
			  double intensity, bool fish, double &amplitude )
{
  if ( fish ) {
    // fish EOD present:
    double uppermean = 0.0;
    double upperampl = 0.0;
    double lowermean = 0.0;
    double lowerampl = 0.0;
    beatAmplitudes( eodtrace, signalTime(), signalTime() + duration,
		    1.0/beatfrequency, maxcontrast,
		    uppermean, upperampl, lowermean, lowerampl );
    /*
    printlog( "UM=" + Str( uppermean, "%.3f" ) + " LM=" + Str( lowermean, "%.3f" ) +
	      " UA=" + Str( upperampl, "%.3f" ) + " LA=" + Str( lowerampl, "%.3f" ) );
    */
    amplitude = 0.5 * (upperampl + lowerampl);

    // range overflow?
    if ( uppermean+upperampl > 0.95 * eodtrace.maxValue() ||
	 fabs(lowermean-lowerampl) > 0.95 * eodtrace.maxValue() ) {
      Str s = "Beat range overflow: upperpeak = " + Str( uppermean+upperampl );
      s += ", lowerpeak = " + Str( fabs(lowermean-lowerampl) );
      s += ", maxValue = " + Str( eodtrace.maxValue() );
      message( s );
      return 2;
    }

    double contrast = (upperampl + lowerampl)/fabs(uppermean - lowermean);
    printlog( "Contrast: " + Str( 100.0*contrast ) + "%" );

    // contrast overflow?
    if ( usecontrast && contrast > 1.1*maxcontrast ) {
      Str s = "Contrast overflow: " + Str( 100.0*contrast ) + "% > "
	+ Str( 100.0*maxcontrast ) + "%";
      message( s );
      return 2;
    }
    if ( ! usecontrast && amplitude > 1.1*maxamplitude ) {
      Str s = "Amplitude overflow: " + Str( amplitude ) + "mV/cm > "
	+ Str( maxamplitude ) + "mV/cm";
      message( s );
      return 2;
    }

    // contrast underflow?
    if ( usecontrast && contrast < 0.9*mincontrast ) {
      Str s = "Contrast underflow: " + Str( 100.0*contrast ) + "% < "
	+ Str( 100.0*mincontrast ) + "%";
      message( s );
      return 1;
    }
    if ( ! usecontrast && amplitude < 0.9*minamplitude ) {
      Str s = "Amplitude underflow: " + Str( amplitude ) + "% < "
	+ Str( minamplitude ) + "%";
      message( s );
      return 1;
    }
  }
  else {
    // no fish EOD present:

    // mean EOD amplitude:
    double offset = 0.1 * duration;
    amplitude = eodAmplitude( eodtrace, signalTime() + offset,
			      signalTime() + duration - offset );
    printlog( "Amplitude: " + Str( amplitude ) + "mV/cm" );
    
    // overflow?
    if ( amplitude > 1.1 * maxamplitude ) {
      Str s = "Signal overflow: Amplitude = " + Str( amplitude )
	+ "mV/cm > " + Str( maxamplitude ) + "mV/cm";
      message( s );
      return 2;
    }
  }

  return 0;
}


addRePro( CalibEField, efield );

}; /* namespace efield */

#include "moc_calibefield.cc"
