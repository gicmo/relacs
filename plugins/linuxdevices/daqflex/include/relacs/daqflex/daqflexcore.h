/*
  daqflex/daqflexcore.cc
  The DAQFlex interface over libusb

  RELACS - Relaxed ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2012 Jan Benda <benda@bio.lmu.de>

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

#ifndef _RELACS_DAQFLEX_DAQFLEXCORE_H_
#define _RELACS_DAQFLEX_DAQFLEXCORE_H_ 1

#include <string>
#include <libusb-1.0/libusb.h>
#include <relacs/device.h>
using namespace std;
using namespace relacs;

namespace daqflex {


/*!
\class DAQFlexCore
\author Jan Benda
\version 1.2
\brief [Device] The DAQFlex interface over libusb
\par Options
- \c firmwarepath=/usr/lib/daqflex/: Path to the *.rbf firmware files
*/

class DAQFlexCore : public Device
{

public:

  enum DAQFlexError {
    Success,
    ErrorNoDevice,
    ErrorInvalidID,
    ErrorUSBInit,
    ErrorPipe,
    ErrorTransferFailed,
    ErrorInvalidBufferSize,
    ErrorCantOpenFPGAFile,
    ErrorFPGAUploadFailed,
    ErrorLibUSBIO,
    ErrorLibUSBInvalidParam,
    ErrorLibUSBAccess,
    ErrorLibUSBNoDevice,
    ErrorLibUSBNotFound,
    ErrorLibUSBBusy,
    ErrorLibUSBTimeout,
    ErrorLibUSBOverflow,
    ErrorLibUSBPipe,
    ErrorLibUSBInterrupted,
    ErrorLibUSBNoMem,
    ErrorLibUSBNotSupported,
    ErrorLibUSBOther,
    ErrorLibUSBUnknown
  };
  static const int DAQFlexErrorMax = 23;
  static const string DAQFlexErrorText[DAQFlexErrorMax];

    /*! vendor ID of MCC USB DAQ boards. */
  static const int MCCVendorID = 0x09db;

  // device product IDs:
  static const int USB_2001_TC = 0x00F9;
  static const int USB_7202 = 0x00F2;
  static const int USB_7204 = 0x00F0;
  static const int USB_1608_GX = 0x0111;
  static const int USB_1608_GX_2AO = 0x0112;

  DAQFlexCore( void );
  DAQFlexCore( const string &device, const Options &opts );
  ~DAQFlexCore( void );

  virtual int open( const string &device, const Options &opts );
  virtual bool isOpen( void ) const;
  virtual void close( void );
  virtual int reset( void );

    /*! \return response if transfer successful, empty string if not. */
  string sendMessage( const string &message );
    /*! Send command to the device. 
        \return error code */
  int sendCommand( const string &command );
    /*! Send commands to the device.
        \return error code  */
  int sendCommands( const string &command1, const string &command2 );

    /*! \return the resolution of the A/D converter. */
  unsigned short maxAIData( void ) const;
    /*! \return the maximum scan rate of the A/D converter. */
  double maxAIRate( void ) const;
    /*! \return the number of analog input channels. */
  int maxAIChannels( void ) const;
    /*! \return the number of samples the AI FIFO can hold. */
  int aiFIFOSize( void ) const;

    /*! \return the resolution of the D/A converter. */
  unsigned short maxAOData( void ) const;
    /*! \return the maximum scan rate of the D/A converter. */
  double maxAORate( void ) const;
    /*! \return the number of analog output channels. */
  int maxAOChannels( void ) const;
    /*! \return the number of samples the AI FIFO can hold. */
  int aoFIFOSize( void ) const;

    /*! The size of a single incoming packet in bytes. */
  int inPacketSize( void ) const;
    /*! The size of a single outgoing packet in bytes. */
  int outPacketSize( void ) const;

    /*! Transfer data from the device to a buffer. */
  int readBulkTransfer( unsigned char *data, int length, int *transferred,
			unsigned int timeout );
    /*! Transfer data from a buffer to the device. */
  int writeBulkTransfer( unsigned char *data, int length, int *transferred,
			 unsigned int timeout );

    /*! Clear the reading endpoint. */
  void clearRead( void );
    /*! Clear the writing endpoint. */
  void clearWrite( void );


 private:

    /*! A handle to the USB device. */
  libusb_device_handle *deviceHandle( void );
    /*! The endpoint for reading data. */
  unsigned char endpointIn( void );
    /*! The endpoint for writing data. */
  unsigned char endpointOut( void );

  string productName( int productid );
  void setLibUSBError( int libusberror );
    /*! Send a message to the device. */
  int sendControlTransfer( const string &message );
    /*! Receive a message from the device. This should follow a call to
        sendControlTransfer. */
  string getControlTransfer( void );

  int getEndpoints( void );
  unsigned char getEndpointInAddress( unsigned char* data, int n );
  unsigned char getEndpointOutAddress( unsigned char* data, int n );

  int initDevice( const string &path );
  int transferFPGAfile( const string &path );

  libusb_device_handle *DeviceHandle;
  unsigned char EndpointIn;
  unsigned char EndpointOut;
  int InPacketSize;
  int OutPacketSize;
  int ProductID;
  unsigned short MaxAIData;
  double MaxAIRate;
  int MaxAIChannels;
  int AIFIFOSize;
  unsigned short MaxAOData;
  double MaxAORate;
  int MaxAOChannels;
  int AOFIFOSize;
  DAQFlexError ErrorState;
  static const uint16_t MaxMessageSize = 64;
  static const uint8_t StringMessage = 0x80;
  static const string DefaultFirmwarePath;
  static const int FPGADATAREQUEST = 0x51;

};


}; /* namespace daqflex */

#endif /* ! _RELACS_DAQFLEX_DAQFLEXCORE_H_ */
