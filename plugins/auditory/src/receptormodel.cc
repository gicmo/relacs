/*
  auditory/receptormodel.cc
  Spiking neuron model stimulated through an auditory tranduction chain.

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
#include <relacs/optwidget.h>
#include <relacs/random.h>
#include <relacs/odealgorithm.h>
#include <relacs/auditory/receptormodel.h>
using namespace relacs;

namespace auditory {


ReceptorModel::ReceptorModel( void )
  : NeuronModels( "ReceptorModel", "auditory", "Alexander Wolf, Jan Benda",
		  "1.1", "Jan 15, 2006" )
{
  // tympanum:
  TDec = 0.154;
  // nonlinearities:
  Imax = 60.0;
  Imin = 0.0;
  CutPoint = 0.02;
  Slope = 3000;
  X0 = 0.01;
  Slope2 = 600000;

  // options:
  newSection( "Recording" );
  addBoolean( "extracellular", "Extracellular", false );
  addNumber( "extranoise", "Extracellular noise", 1.0, 0.0, 10000.0, 1.0, "mV" );
  newSection( "Transduction chain" );
  addSelection( "tymp", "Tympanum model", "Scaling|None|Scaling|Oscillator" );
  addNumber( "freq", "Eigenfrequency", 5.0, 0.5, 40.0, 0.1, "kHz" );
  addNumber( "tdec", "Decay constant", TDec, 0.01, 10.0, 0.01, "ms" );
  addSelection( "nl", "Static nonlinearity", "Square saturated|Square|Square saturated|linear Boltzman|square Boltzman|Linear|Linear saturated|Box|None" );
  addOptions();
  addFlags( 1 );

  newSubSection( "Square = ax^2+imin, a=(imax-imin)/cut^2" ).setStyle( OptWidget::MathLabel );
  newSubSection( "Square saturated = imax, for |x|>=cut" ).setStyle( OptWidget::MathLabel );
  newSubSection( "Linear = b|x|+imin, b=(imax-imin)/cut" ).setStyle( OptWidget::MathLabel );
  newSubSection( "Linear saturated = imax, for |x|>=cut" ).setStyle( OptWidget::MathLabel );
  newSubSection( "Box = imin, for |x|<cut, = imax else" ).setStyle( OptWidget::MathLabel );
  newSubSection( "None = ax, a=(imax-imin)/cut" ).setStyle( OptWidget::MathLabel );
  addNumber( "imax", "Maximum current", Imax, -500.0, 500.0, 1.0, "muA/cm^2" );
  addNumber( "imin", "Minimum current (zero point current)", Imin, -500.0, 500.0, 1.0, "muA/cm^2" );
  addNumber( "cut", "Amplitude of tympanum where imax is reached", CutPoint, 0.0, 1000.0, 0.0001, "mPa" );
  newSubSection( "Boltzmann, (imax/(1-f_0))*(1/(1+exp[-slope*(x-x0)])+1/(1+exp[slope*(x+x0)])-f_0)+imin" ).setStyle( OptWidget::MathLabel );
  addNumber( "slope", "Slope of Boltzmann", Slope, 0.0, 100000.0, 1.0, "mPa^-1" ).setActivation( "matchslope", "false" );
  addBoolean( "matchslope", "Set slope of Boltzmann to match square", true );
  addNumber( "x0", "1/2 of Imax-Imin is reached", X0, 0.0, 10.0, 0.0001, "mPa" );
  newSubSection( "Boltzmann, 2(imax-imin)(1/(1+exp[-slope2*x^2])-1/2)+imin" ).setStyle( OptWidget::MathLabel );
  addNumber( "slope2", "Slope of square Boltzmann", Slope2, 0.0, 1.0e10, 5.0e4, "mPa^-2" ).setActivation( "matchslope2", "false" );
  addBoolean( "matchslope2", "Set slope of square Boltzmann to match square", true );
  addFlags( 2 );
  delFlags( 2, 1 );

  addModels();
}


ReceptorModel::~ReceptorModel( void )
{
}


void ReceptorModel::main( void )
{
  // recording:
  bool extracellular = boolean( "extracellular" );
  double extranoise = number( "extranoise" );
  // tympanum:
  TympanumModel = index( "tymp" );
  Omega = 2*M_PI*number( "freq" );
  TDec = number( "tdec" );
  Alpha = 2.0/TDec;
  Beta = Omega*Omega;
  int sigdimension = 0;
  if ( TympanumModel == 2 )
    sigdimension = 2;
  // nonlinearities
  int nonlin = index( "nl" );
  if ( nonlin == 0 )
    Nonlinearity = &ReceptorModel::square;
  else if ( nonlin == 1 )
    Nonlinearity = &ReceptorModel::squareSaturated;
  else if ( nonlin == 2 )
    Nonlinearity = &ReceptorModel::linearBoltzman;
  else if ( nonlin == 3 )
    Nonlinearity = &ReceptorModel::squareBoltzman;
  else if ( nonlin == 4 )
    Nonlinearity = &ReceptorModel::linear;
  else if ( nonlin == 5 )
    Nonlinearity = &ReceptorModel::linearSaturated;
  else if ( nonlin == 6 )
    Nonlinearity = &ReceptorModel::box;
  else
    Nonlinearity = &ReceptorModel::identity;
  Imax = number( "imax" );
  Imin = number( "imin" );
  CutPoint = number( "cut" );
  DI = Imax-Imin;
  if ( DI <= 0.0 ) {
    warning( "Imax is smaller than Imin: Change model options" );
  }
  DIC = DI/CutPoint;
  DICC = DI/(CutPoint*CutPoint);
  Slope = number( "slope" );
  X0 = number( "x0" );
  if ( boolean( "matchslope" ) && nonlin == 2 ) {
    // make second derivative at 0 the same as DICC*x^2:
    // second derivative: y = DI*Slope*Slope/(1.0+cosh(Slope*X0))
    // Taylor of denominator: y = DI*Slope*Slope/(2.0+0.5*X0*X0*Slope*Slope)
    // => Slope = 2/CutPoint/sqrt(1.0-X0*X0/CutPoint/CutPoint)
    // is smaller than true solution.
    // refine with bisection:
    double s1 = 2.0/CutPoint/sqrt(1.0-X0*X0/CutPoint/CutPoint);
    double y1 = DI*s1*s1/(1.0+cosh(s1*X0)) - 2.0*DICC;
    while ( y1 > 0.0 ) {
      s1 *= 0.8;
      y1 = DI*s1*s1/(1.0+cosh(s1*X0)) - 2.0*DICC;
    }
    double s2 = 1.4*s1;  // approximately the position of the maximum of y
    double y2 = DI*s2*s2/(1.0+cosh(s2*X0)) - 2.0*DICC;
    while ( y2 < 0.0 ) {
      s2 *= 1.1;
      y2 = DI*s2*s2/(1.0+cosh(s2*X0)) - 2.0*DICC;
    }
    double ds = s2 - s1;
    Slope = s1;
    for ( int k=0; k<40; k++ ) {
      ds *= 0.5;
      double smid = Slope + ds;
      double ymid = DI*smid*smid/(1.0+cosh(smid*X0)) - 2.0*DICC;
      if ( ymid <= 0.0 )
	Slope = smid;
      if ( ds < 1.0e-3 || ymid == 0.0 )
	break;
    }
    info( "Set <tt>slope</tt> to <b>" + Str( Slope ) + "</b>" );
  }
  F0 = 2.0/(1.0+exp(Slope*X0));
  Slope2 = number( "slope2" );
  if ( boolean( "matchslope2" ) && nonlin == 3 ) {
    // make second derivative at 0 the same as DICC*x^2:
    Slope2 = 2.0/(CutPoint*CutPoint);
    info( "Set <tt>slope2</tt> to <b>" + Str( Slope2 ) + "</b>" );
  }

  // spiking neuron and general integration options:
  readOptions();

  // deltat( 0 ) must be integer multiple of delta t for integration:
  int maxs = int( ::floor( 1000.0*deltat( 0 )/timeStep() ) );
  if ( maxs <= 0 )
    maxs = 1;
  setTimeStep( 1000.0 * deltat( 0 ) / maxs );
  int cs = 0;
  setNoiseFac();

  // state variables:
  int simn = sigdimension + neuron()->dimension();
  if ( GMC > 1e-8  ) {
    MMCInx = simn;
    simn++;
  }
  else
    MMCInx = -1;
  if ( GMHC > 1e-8  ) {
    MMHCInx = simn;
    simn++;
    HMHCInx = simn;
    simn++;
  }
  else {
    MMHCInx = -1;
    HMHCInx = -1;
  }
  double simx[simn];
  double dxdt[simn];
  for ( int k=0; k<simn; k++ )
    simx[k] = dxdt[k] = 0.0;
  neuron()->init( simx+sigdimension );

  // equilibrium:
  for ( int c=0; c<100; c++ ) {
    double t = c * timeStep();
    (*neuron())( t, 0.0, simx+sigdimension, dxdt+sigdimension, neuron()->dimension() );
    for ( int k=sigdimension; k<simn; k++ )
      simx[k] += timeStep()*dxdt[k];
  }

  // integrate:
  Random rand;
  double pv = simx[sigdimension];
  double t = 1000.0*time( 0 );  // time must be syncrhonous to recorded trace!
  while ( ! interrupt() ) {

    Integrate( t, simx, dxdt, simn, timeStep(), *this );

    cs++;
    if ( cs == maxs ) {
      if ( extracellular )
	push( 0, simx[sigdimension] - pv + extranoise*rand.gaussian() );
      else
	push( 0, simx[sigdimension] );
      pv = simx[sigdimension];
      next();
      cs = 0;
    }

    t += timeStep();
  }

}


void ReceptorModel::process( const OutData &source, OutData &dest )
{
  // rescale stimulus to mPa:
  const double p0 = 2.0e-5;
  double i = source.intensity();
  if ( source.trace() == RightSpeaker[0] )
    i -= 10.0;
  double g = p0 * ::pow( 10.0, i / 20.0 );

  int inx = source.indices( 5.0*TDec );
  dest.reserve( source.size() + inx );
  dest = source;

  // tympanum:
  if ( TympanumModel == 1 ) {
    double omega = 2.0*M_PI*fabs( 0.001*source.carrierFreq() );
    double omega2 = omega*omega;
    double x = Beta - omega2;
    double h = Beta / ::sqrt( x*x + Alpha*Alpha*omega2 );
    dest *= g * h;
  }
  else if ( TympanumModel == 2 ) {
    dest *= g * Beta;
    return;
    /*
    // too slow!
    dest.append( 0.0, inx );
    double dt = 1000.0*dest.stepsize();  // milliseconds!
    double x[2] = { 0.0, 0.0 };
    double dxdt[2];
    for ( int k=0; k<dest.size(); k++ ) {
      dxdt[0] = x[1];
      dxdt[1] = -Beta*x[0] - Alpha*x[1] + dest[k];
      x[0] += dxdt[0] * dt;
      x[1] += dxdt[1] * dt;
      dest[k] = x[0];
    }
    */
  }
  else
    dest *= g;

  // nonlinearity:
  for ( int k=0; k<dest.size(); k++ )
    dest[k] = (this->*Nonlinearity)( dest[k] );

  dest += neuron()->offset();
  dest *= neuron()->gain();
}


void ReceptorModel::operator()( double t, double *x, double *dxdt, int n )
{
  static Random rand;

  double s = signal( 0.001 * t, LeftSpeaker[0] );
  s += signal( 0.001 * t, RightSpeaker[0] );
  if ( TympanumModel == 2 ) {
    dxdt[0] = x[1];
    dxdt[1] = -Beta*x[0] - Alpha*x[1] + s;
    double s = ( (this->*Nonlinearity)( x[0] ) + neuron()->offset() ) * neuron()->gain();
    s += noiseFac() * rand.gaussian();
    if ( MMCInx >= 0 )
      s -= GMC*x[MMCInx]*(x[2]-EMC);
    if ( MMHCInx >= 0 )
      s -= GMHC * ::pow( x[MMHCInx], PMMHC ) * ::pow( x[HMHCInx], PHMHC ) * (x[2]-EMHC);
    (*neuron())( t, s, x+2, dxdt+2, n-2 );
    if ( MMCInx >= 0 ) {
      double m0mc = 1.0/(exp(-(x[2]-MVMC)/MWMC)+1.0);
      dxdt[MMCInx] = ( m0mc - x[MMCInx] )/TAUMC;
    }
    if ( MMHCInx >= 0 ) {
      double m0mhc = 1.0/(exp(-(x[2]-MVMHC)/MWMHC)+1.0);
      dxdt[MMHCInx] = ( m0mhc - x[MMHCInx] )/TAUMMHC;
      double h0mhc = 1.0/(exp(-(x[2]-HVMHC)/HWMHC)+1.0);
      dxdt[HMHCInx] = ( h0mhc - x[HMHCInx] )/TAUHMHC;
    }
  }
  else {
    s += noiseFac() * rand.gaussian();
    if ( MMCInx >= 0 )
      s -= GMC*x[MMCInx]*(x[0]-EMC);
    if ( MMHCInx >= 0 )
      s -= GMHC * ::pow( x[MMHCInx], PMMHC ) * ::pow( x[HMHCInx], PHMHC ) * (x[0]-EMHC);
    (*neuron())( t, s, x, dxdt, n );
    if ( MMCInx >= 0 ) {
      double m0mc = 1.0/(exp(-(x[0]-MVMC)/MWMC)+1.0);
      dxdt[MMCInx] = ( m0mc - x[MMCInx] )/TAUMC;
    }
    if ( MMHCInx >= 0 ) {
      double m0mhc = 1.0/(exp(-(x[0]-MVMHC)/MWMHC)+1.0);
      dxdt[MMHCInx] = ( m0mhc - x[MMHCInx] )/TAUMMHC;
      double h0mhc = 1.0/(exp(-(x[0]-HVMHC)/HWMHC)+1.0);
      dxdt[HMHCInx] = ( h0mhc - x[HMHCInx] )/TAUHMHC;
    }
  }
}


double ReceptorModel::identity( double x ) const
{
  return DIC*x;
}


double ReceptorModel::box( double x ) const
{
  return fabs(x) < CutPoint ? Imin : Imax;
}


double ReceptorModel::linear( double x ) const
{
  return DIC*fabs(x) + Imin;
}


double ReceptorModel::linearSaturated( double x ) const
{
  double s = DIC*fabs(x) + Imin;
  return s<Imax ? s : Imax;
}


double ReceptorModel::square( double x ) const
{
  return DICC*x*x + Imin;
}


double ReceptorModel::squareSaturated( double x ) const
{
  double s = DICC*x*x + Imin;
  return s<Imax ? s : Imax;
}


double ReceptorModel::linearBoltzman( double x ) const
{
  /*
    Second derivative at x=0:
    DI*Slope*Slope/(1.0+cosh(Slope*X0))
   */
  return (DI/(1.0-F0))*(1.0/(1.0+exp(-Slope*(x-X0)))+1.0/(1.0+exp(Slope*(x+X0)))-F0)+Imin;
}


double ReceptorModel::squareBoltzman( double x ) const
{
  return 2.0*DI*(1.0/(1.0+exp(-Slope2*x*x))-0.5)+Imin;
}


OptWidget *ReceptorModel::dialogOptions( OptDialog *od, string *tabhotkeys )
{
  OptWidget *ow = od->addTabOptions( "General", *this, dialogSelectMask() | 1,
				     dialogReadOnlyMask(), dialogStyle(), 
				     mutex(), tabhotkeys );
  od->addTabOptions( "Nonlinearity", *this, dialogSelectMask() | 2,
		     dialogReadOnlyMask(), dialogStyle(), mutex(), tabhotkeys );
  dialogModelOptions( od, tabhotkeys );
  od->setVerticalSpacing( 1 );
  od->setMargins( 10 );
  return ow;
}


addModel( ReceptorModel, auditory );

}; /* namespace auditory */
