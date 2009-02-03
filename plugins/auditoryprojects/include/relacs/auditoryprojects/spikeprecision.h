/*
  auditoryprojects/spikeprecision.h
  Assess spike precision in locust auditory receptors

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

#ifndef _RELACS_AUDITORYPROJECTS_SPIKEPRECISION_H_
#define _RELACS_AUDITORYPROJECTS_SPIKEPRECISION_H_ 1

#include <relacs/sampledata.h>
#include <relacs/multiplot.h>
#include <relacs/rangeloop.h>
#include <relacs/ephys/traces.h>
#include <relacs/acoustic/traces.h>
#include <relacs/repro.h>
using namespace relacs;

namespace auditoryprojects {


/*! 
\class SpikePrecision
\brief [RePro] Assess spike precision in locust auditory receptors
\author Samuel Glauser
\author Jan Benda
\version 1.5 (Jan 10, 2008)
-# removed stop() function
-# Introduced separate plot widget for intensity search
\version 1.4 (Feb 28, 2006)
-# Rescale cutoff in any case. Only noise gap has this as an option.
-# New ramp2 parameter for rectangular and saw tooth stimuli.
-# Changed "Clipped" warning to info.
\version 1.3 (Feb ??, 2006)
-# Rescale affects only the stimulus but not its amplitude.
*/


class SpikePrecision : public RePro, public ephys::Traces, public acoustic::Traces
{
  Q_OBJECT

public:

    /*! Constructs a SpikePrecision-RePro: intialize widgets and create options. */
  SpikePrecision( void );
    /*! Destructs a SpikePrecision-RePro. */
  ~SpikePrecision( void );

  virtual int main( void );


protected:

  struct EnvelopeFrequencyData
  {
    EnvelopeFrequencyData( double duration, double deltat )
      : Rate1( 0.0, duration, deltat, 0.0 ), Rate2( 0.0, duration, deltat, 0.0 ) {};
      EventList Spikes;
      double Frequency;
      double Intensity;
      double SSRate;
      double Correlation;
      string Envelope;
      SampleDataD Rate1;
      SampleDataD Rate2;
  };

  void analyze( vector < EnvelopeFrequencyData > &results );
  void plot( const SampleDataD &amdb,
	     const vector < EnvelopeFrequencyData > &results );
  void saveSpikes( const vector < EnvelopeFrequencyData > &results );
  void saveRates( const vector < EnvelopeFrequencyData > &results );
  void save( const vector < EnvelopeFrequencyData > &results );

  int createStimulus( OutData &signal, SampleDataD &amdb,
		      double frequency, const Str &file, bool store=true );

  void customEvent( QCustomEvent *qce );

  double CarrierFrequency;
  double Amplitude;
  double PeakAmplitude;
  double PeakAmplitudeFac;
  RangeLoop FreqRange;
  double Frequency;
  enum WaveForms { Sine, Rectangular, Triangular,
		   Sawup, Sawdown, Noisegap, Noisecutoff };
  WaveForms WaveForm;
  double DutyCycle;
  double RelFreqGap;
  double AbsFreqGap;
  bool Rescale;
  double Ramp;
  double Ramp2;
  double Intensity;
  double Duration;
  int StimRepetition;
  int Side;
  double SkipWindow;
  double Sigma1;
  double Sigma2;
  enum StoreModes { SessionPath, ReProPath, CustomPath };
  Str StorePath;
  Str StoreFile;

  MultiPlot SP;
  MultiPlot P;

  string StimulusLabel;

  Options Settings;

};


}; /* namespace auditoryprojects */

#endif /* ! _RELACS_AUDITORYPROJECTS_SPIKEPRECISION_H_ */
