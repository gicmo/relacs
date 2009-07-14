/*
  comedi/comedidigitalio.h
  Interface for accessing digital I/O lines of a daq-board via comedi.

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

#ifndef _COMEDI_COMEDIDIGITALIO_H_
#define _COMEDI_COMEDIDIGITALIO_H_

#include <comedilib.h>
#include <vector>
#include <relacs/digitalio.h>
using namespace std;
using namespace relacs;

namespace comedi {


/*! 
\class ComediDigitalIO
\author Jan Benda
\brief [DigitalIO] Interface for accessing digital I/O lines of a daq-board via comedi.
*/


class ComediDigitalIO : public DigitalIO
{

public:

    /*! Create a new ComediDigitalIO without opening a device. */
  ComediDigitalIO( void );
    /*! Open the digital I/O driver specified by its device file \a device. */
  ComediDigitalIO( const string &device, long mode=0 );
    /*! Close the daq driver. */
  virtual ~ComediDigitalIO( void );

    /*! Open the digital I/O device specified by \a device.
 	Returns zero on success, or InvalidDevice (or any other negative number
	indicating the error).
        \sa isOpen(), close() */
  virtual int open( const string &device, long mode=0 );
    /*! Returns true if the device is open.
        \sa open(), close() */
  virtual bool isOpen( void ) const;
    /*! Close the device.
        \sa open(), isOpen() */
  virtual void close( void );

    /*! \return the number of digital I/O lines the device supports */
  virtual int lines( void ) const;

    /*! Configure digital I/O lines specified by \a mask for input (0) or output (1).
        \return 0 on success, otherwise a negative number indicating the error */
  virtual int configure( unsigned long dios, unsigned long mask ) const;

    /*! Write \a val to the digital I/O line \a line.
        \return 0 on success, otherwise a negative number indicating the error
        \sa read() */
  virtual int write( unsigned long line, bool val );
    /*! Read from digital I/O line \a line and return value in \a val.
        \return 0 on success, otherwise a negative number indicating the error
        \sa write() */
  virtual int read( unsigned long line, bool &val ) const;

    /*! Write \a dios to the digital I/O lines defined in \a mask.
        \return 0 on success, otherwise a negative number indicating the error
        \sa read() */
  virtual int write( unsigned long dios, unsigned long mask );
    /*! Read digital I/O lines and return them in \a dios.
        \return 0 on success, otherwise a negative number indicating the error
        \sa write() */
  virtual int read( unsigned long &dios ) const;


private:

    /*! Pointer to the comedi device. */
  comedi_t *DeviceP;
    /*! The comedi subdevice number. */
  unsigned int SubDevice;
    /*! The number of supported digital I/O lines */
  int MaxLines;

};


}; /* namespace comedi */

#endif /* ! _COMEDI_COMEDIDIGITALIO_H_ */
