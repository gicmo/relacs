/*
  attsim.h
  Implementation of the Attenuator class

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

#ifndef _RELACS_ATTSIM_H_
#define _RELACS_ATTSIM_H_ 1

#include <relacs/attenuator.h>

namespace relacs {


/*! 
\class AttSim
\author Jan Benda
\version 1.1
\brief Implementation of the Attenuator class
for simulating an attenuator.


The attenuation levels are stored in the global variable
\a AttSimDecibel.
*/


class AttSim: public Attenuator
{

public:

    /*! Constructs an AttSim. */
  AttSim( void );
    /*! Close the attenuator simulation. */
  ~AttSim( void );

    /*! Open the attenuator device driver specified by \a device. */
  int open( const string &device ) override;
    /*! Returns true if the attenuator device driver was succesfully opened. */
  bool isOpen( void ) const;
    /*! Close the attenuator device driver. */
  void close( void );

    /*! Returns the current settings of the attenuator. */
  virtual const Options &settings( void ) const;

    /*! Returns the number of attenuator devices the driver handles. */
  virtual int lines( void ) const;
    /*! Returns the minimum possible attenuation level in decibel.
        This number can be negative, indicating amplification. */
  virtual double minLevel( void ) const;
    /*! Returns the maximum possible attenuation level in decibel. */
  virtual double maxLevel( void ) const;
    /*! Returns in \a l all possible attenuation levels
        sorted by increasing attenuation levels (highest last). */
  virtual void levels( vector<double> &l ) const;

    /*! Set the attenuation level of the subdevice specified by its index \a di
        to \a decibel decibel. 
        Returns the actually set level in \a decibel. */
  int attenuate( int di, double &decibel );
    /*! Tests setting the attenuation level of the subdevice specified by its index \a di
        to \a decibel decibel. 
        Returns the actually set level in \a decibel. */
  int testAttenuate( int di, double &decibel );


  static const int MaxDevices = 2;
  static double Decibel[MaxDevices];


private:

  static const double AttStep;
  static const double AttMax;
  static const double AttMin;

};


}; /* namespace relacs */

#endif /* ! _RELACS_ATTSIM_H_ */

