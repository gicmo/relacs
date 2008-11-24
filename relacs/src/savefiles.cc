/*
  savefiles.cc
  Write data to files

  RELACS - RealTime ELectrophysiological data Acquisition, Control, and Stimulation
  Copyright (C) 2002-2008 Jan Benda <j.benda@biologie.hu-berlin.de>

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

#include <cstdio>
#include <qdatetime.h>
#include <qpainter.h>
#include <qtooltip.h>
#include <relacs/relacswidget.h>
#include <relacs/savefiles.h>

namespace relacs {


SaveFiles::SaveFiles( RELACSWidget *rw, int height,
		      QWidget *parent, const char *name )
  : QHBox( parent, name ),
    Options(),
    RW( rw ),
    StimulusDataLock( true )
{
  Path = "";
  PathTemplate = "%04Y-%02m-%02d-%a2a";
  DefaultPath = "";

  PathNumber = 0;
  PathTime = ::time( 0 );

  FilesOpen = false;
  Writing = false;

  VF.clear();
  SF = 0;

  clearRemoveFiles();

  TraceToWrite = 0;
  SessionTime = 0.0;
  TraceIndex = 0;
  TraceOffs = 0;
  SignalOffs = 0;

  EventFiles.clear();

  StimulusToWrite.clear();
  StimulusData = false;
  StimulusKey.clear();

  ToggleOn = false;
  ToggleData = false;

  ReProName = "";
  ReProAuthor = "";
  ReProVersion = "";
  ReProDate = "";
  ReProSettings.clear();
  ReProData = false;

  setFixedHeight( height );

  FileLabel = new QLabel( "no files open", this );
  FileLabel->setTextFormat( PlainText );
  FileLabel->setIndent( 2 );
  FileLabel->setAlignment( AlignLeft | AlignVCenter );
  QToolTip::add( FileLabel, "The directory where files are currently stored" );

  SaveLabel = new SpikeTrace( 0.8, 8, 3, this );
  SaveLabel->setFixedWidth( SaveLabel->minimumSizeHint().width() );
  QToolTip::add( SaveLabel, "An animation indicating that raw data are stored on disk" );
}


SaveFiles::~SaveFiles()
{
  closeFiles();

  clearRemoveFiles();

  TraceToWrite = 0;

  EventFiles.clear();

  StimulusToWrite.clear();
}


bool SaveFiles::writing( void ) const
{
  return Writing;
}


bool SaveFiles::saving( void ) const
{
  return FilesOpen;
}


string SaveFiles::path( void ) const
{
  return Path;
}


void SaveFiles::setPath( const string &path )
{
  Path = path;
  setenv( "RELACSDATAPATH", Path.c_str(), 1 );
}


string SaveFiles::addPath( const string &file ) const
{
  return path() + file;
}


string SaveFiles::pathTemplate( void ) const
{
  return PathTemplate;
}


void SaveFiles::setPathTemplate( const string &path )
{
  if ( path.empty() )
    return;

  PathTemplate = path;

  Str fn = PathTemplate;
  fn.format( localtime( &PathTime ) );
  fn.format( 99, 'n', 'd' );
  fn.format( "aa", 'a' );
  fn.format( "AA", 'A' );
  FileLabel->setFixedWidth( QFontMetrics( HighlightFont ).boundingRect( fn.c_str() ).width() + 8 );
}


string SaveFiles::defaultPath( void ) const
{
  return DefaultPath;
}


void SaveFiles::setDefaultPath( const string &defaultpath )
{
  if ( defaultpath.empty() )
    return;

  if ( Path == DefaultPath )
    setPath( defaultpath );
  DefaultPath = defaultpath;
  setenv( "RELACSDEFAULTPATH", DefaultPath.c_str(), 1 );
}


string SaveFiles::addDefaultPath( const string &file ) const
{
  return defaultPath() + file;
}


void SaveFiles::notify( void )
{
  RW->notifyStimulusData();
}


void SaveFiles::lock( void ) const
{
  StimulusDataLock.lock();
}


void SaveFiles::unlock( void ) const
{
  StimulusDataLock.unlock();
}


QMutex *SaveFiles::mutex( void )
{
  return &StimulusDataLock;
}


void SaveFiles::polish( void )
{
  QWidget::polish();

  NormalFont = FileLabel->font();
  HighlightFont = QFont( fontInfo().family(), fontInfo().pointSize()*4/3, QFont::Bold );

  Str fn = PathTemplate;
  fn.format( localtime( &PathTime ) );
  fn.format( 99, 'n', 'd' );
  fn.format( "aa", 'a' );
  fn.format( "AA", 'A' );
  FileLabel->setFixedWidth( QFontMetrics( HighlightFont ).boundingRect( fn.c_str() ).width() + 8 );

  NormalPalette = FileLabel->palette();
  HighlightPalette = FileLabel->palette();
  HighlightPalette.setColor( QPalette::Normal, QColorGroup::Foreground, red );
  HighlightPalette.setColor( QPalette::Inactive, QColorGroup::Foreground, red );
}


void SaveFiles::write( bool on )
{
  //  cerr << "write toggle: " << on << '\n';
  ToggleData = true;
  ToggleOn = on;
}


void SaveFiles::writeToggle( void )
{
  //  cerr << "writeToggle(): " << ToggleData << ", on=" << ToggleOn << '\n';

  if ( ToggleData ) {
    Writing = ToggleOn;
    SaveLabel->setPause( !writing() );
    ToggleData = false;
  }

}


void SaveFiles::write( const InList &data, const EventList &events )
{
  //  cerr << "SaveFiles::write( InList &data, EventList &events )\n";

  // update write status:
  writeToggle();

  // indicate the new RePro:
  writeRePro();

  if ( !writing() )
    return;

  // write trace data:
  TraceToWrite = &data;
  /*
  //      cerr << "writeTrace\n";
  if ( ! VF.empty() && Writing ) {
    TraceOffs += TraceToWrite->saveBinary( VF, -1, SaveFilesMode, TraceIndex );
    TraceIndex = TraceToWrite->currentIndex( -1 );
    SignalOffs = TraceOffs - TraceIndex + TraceToWrite->signalIndex( -1 ); // XXX this works only if all traces are written!
  }
  */

  // write event data:
  for ( int k=0; k<(int)EventFiles.size() && k<events.size(); k++ )
    EventFiles[k].Events = &events[k];
  //      cerr << "writeEvents\n";
  if ( Writing ) {
    for ( unsigned int k=0; k<EventFiles.size(); k++ ) {
      if ( EventFiles[k].Stream != 0 ) {

	while ( EventFiles[k].Offset < EventFiles[k].Events->size() ) {
	  double et = (*EventFiles[k].Events)[EventFiles[k].Offset];
	  if ( EventFiles[0].Events->size() > 0 &&
	       et >= EventFiles[0].Events->back() &&
	       ( EventFiles[k].Offset == 0 || 
		 (*EventFiles[k].Events)[EventFiles[k].Offset-1] < EventFiles[0].Events->back() ) ) {
	    EventFiles[k].SignalEvent = EventFiles[k].Lines;
	    *EventFiles[k].Stream << '\n';
	  }
	  EventFiles[k].Key.save( *EventFiles[k].Stream, et - SessionTime, 0 );
	  if ( EventFiles[k].SaveSize )
	    EventFiles[k].Key.save( *EventFiles[k].Stream,
				    EventFiles[k].Events->sizeScale() *
				    EventFiles[k].Events->eventSize( EventFiles[k].Offset ) );
	  if ( EventFiles[k].SaveWidth )
	    EventFiles[k].Key.save( *EventFiles[k].Stream,
				    EventFiles[k].Events->widthScale() *
				    EventFiles[k].Events->eventWidth( EventFiles[k].Offset ) );
	  *EventFiles[k].Stream << '\n';
	  EventFiles[k].Lines++;
	  EventFiles[k].Offset++;
	}

      }
    }
  }

}


void SaveFiles::write( const OutData &signal )
{
  //  cerr << "SaveFiles::write( OutData &signal )\n";

  // write last trial data here!
  // we know that trace and events were written just before RePro::read
  // was called (from which the write( signal ) originates. 
  // Therefore all indices are known for writing the data of the last signal.
  writeStimulus();

  if ( signal.error() != 0 )
    return;

  if ( StimulusData )
    RW->printlog( "! warning: SaveFiles::write( OutData & ) -> already stimulus data there" );

  StimulusData = true;
  StimulusToWrite.add( &signal );
}


void SaveFiles::write( const OutList &signal )
{
  //  cerr << "SaveFiles::write( OutList &signal )\n";

  // write last trial data here!
  // we know that trace and events were written just before RePro::read
  // was called (from which the write( signal ) originates. 
  // Therefore all indices are known for writing the data of the last signal.
  writeStimulus();

  if ( signal.empty() || signal[0].failed() )
    return;

  if ( StimulusData )
    RW->printlog( "! warning: SaveFiles::write( OutList& ) -> already stimulus data there" );

  StimulusData = true;
  for ( int k=0; k<signal.size(); k++ )
    StimulusToWrite.add( &signal[k] );
}


void SaveFiles::writeStimulus( void )
{
  if ( StimulusData ) {
    
    //      cerr << "writeStimulus \n";
    
    // stimulus indices file:
    if ( SF != 0 && Writing ) {
      StimulusKey.save( *SF, SignalOffs, 0 );
      for ( int k=0; k<TraceToWrite->size(); k++ )
	if ( (*TraceToWrite)[k].mode() & SaveFilesMode ) {
	  StimulusKey.save( *SF, (*TraceToWrite)[k].gain() * (*TraceToWrite)[k].scale() );
	  StimulusKey.save( *SF, (*TraceToWrite)[k].offset() );
	}
      for ( unsigned int k=0; k<EventFiles.size(); k++ )
	if ( EventFiles[k].Stream != 0 ) {
	  StimulusKey.save( *SF, EventFiles[k].SignalEvent );
	  if ( EventFiles[k].SaveMeanRate )
	    StimulusKey.save( *SF, EventFiles[k].Events->meanRate() );  // XXX adaptive Zeit!
	  if ( EventFiles[k].SaveMeanSize )
	    StimulusKey.save( *SF, EventFiles[k].Events->sizeScale() *
			      EventFiles[k].Events->meanSize() );
	  if ( EventFiles[k].SaveMeanWidth )
	    StimulusKey.save( *SF, EventFiles[k].Events->widthScale() *
			      EventFiles[k].Events->meanWidth() );
	  if ( EventFiles[k].SaveMeanQuality )
	    StimulusKey.save( *SF, 100.0*EventFiles[k].Events->meanQuality() );
	}
      lock();
      if ( !Options::empty() ) {
	for( int k=0; k<Options::size(); k++ )
	  StimulusKey.save( *SF, (*this)[k].number() );
      }
      unlock();
      // XXX this is not really multi board!
      StimulusKey.save( *SF, (*TraceToWrite)[0].signalTime() - SessionTime );
      // StimulusToWrite:
      StimulusKey.save( *SF, 1000.0*StimulusToWrite[0].delay() );
      for ( int k=0; k<RW->AQ->outTracesSize(); k++ ) {
	for ( int j=0; j<StimulusToWrite.size(); j++ ) {
	  const Attenuate *att = RW->AQ->outTraceAttenuate( k );
	  if ( StimulusToWrite[j] == RW->AQ->outTrace( k ) ) {
	    StimulusKey.save( *SF, 0.001*StimulusToWrite[j].sampleRate() );
	    StimulusKey.save( *SF, 1000.0*StimulusToWrite[j].length() );
	    if ( att != 0 ) {
	      StimulusKey.save( *SF, StimulusToWrite[j].intensity() );
	      if ( ! att->frequencyName().empty() )
		StimulusKey.save( *SF, StimulusToWrite[j].carrierFreq() );
	    }
	    StimulusKey.save( *SF, StimulusToWrite[j].ident() );
	  }
	  else {
	    StimulusKey.save( *SF, "" );
	    StimulusKey.save( *SF, "" );
	    if ( att != 0 ) {
	      StimulusKey.save( *SF, "" );
	      if ( ! att->frequencyName().empty() )
		StimulusKey.save( *SF, "" );
	    }
	    StimulusKey.save( *SF, "" );
	  }
	}
      }
      *SF << endl;
    }
    
    StimulusData = false;
    StimulusToWrite.clear();
  }
}


void SaveFiles::write( const RePro &RP )
{
  //  cerr << "SaveFiles::write( const RePro &RP ) \n";

  if ( ReProData )
    RW->printlog( "! warning: SaveFiles::write( RePro & ) -> already RePro data there." );
  ReProData = true;
  ReProName =  RP.name();
  ReProAuthor =  RP.author();
  ReProVersion =  RP.version();
  ReProDate =  RP.date();
  ReProSettings = RP;

  // write last stimulus here!
  // it is the probably unfinished one of the previous RePro.
  writeStimulus();
}


void SaveFiles::writeRePro( void )
{
  //  cerr << "write RePro\n";

  if ( ReProData ) {
    
    // stimulus indices file:
    if ( SF != 0 && Writing ) {
      *SF << '\n';
      *SF << "# repro: " << ReProName  << '\n';
      *SF << "# author: " << ReProAuthor << '\n';
      *SF << "# version: " << ReProVersion << '\n';
      *SF << "# date: " << ReProDate << '\n';
      if ( ! ReProSettings.empty() ) {
	ReProSettings.setFlags( 0 );
	ReProSettings.setTypeFlags( 1, -Parameter::Blank );
	ReProSettings.save( *SF, "# ", -1, 1, false, true );
      }
      ReProSettings.clear();

      // write StimulusKey:
      *SF << '\n';
      StimulusKey.saveKey( *SF );
    }

    ReProData = false;
  }
}


void SaveFiles::addRemoveFile( const string &filename )
{
  RemoveFiles.push_back( filename );
}


void SaveFiles::clearRemoveFiles( void )
{
  RemoveFiles.clear();
}


void SaveFiles::removeFiles( void )
{
  for ( vector<string>::iterator h = RemoveFiles.begin();
	h != RemoveFiles.end();
	++h ) {
    remove( h->c_str() );
  }
  clearRemoveFiles();
}


ofstream *SaveFiles::openFile( const string &filename, int type )
{
  string fs = path() + filename;
  addRemoveFile( fs );
  ofstream *f = new ofstream( fs.c_str(), ofstream::openmode( type ) );
  if ( ! f->good() ) {
    f = 0;
    RW->printlog( "SaveFiles::openFile: can't open file '" + fs + "'" );
  }
  return f;
}


void SaveFiles::createTraceFile( const Acquire &intraces )
{
  // init trace variables:
  //  TraceToWrite = &data;
  SessionTime = (*TraceToWrite)[0].currentTime();
  TraceIndex = (*TraceToWrite)[0].currentIndex();
  TraceOffs = 0;
  SignalOffs = 0;

  // create file for trace-data:
  VF.clear();
  VF.reserve( intraces.inputsSize() );
  for ( int k=0; k<intraces.inputsSize(); k++ ) {
    string filename = "traces-" + Str( k+1 ) + "." + intraces.inputTraces( k )[0].deviceDataTypeId() + Str( intraces.inputTraces( k ).size() );
    VF[k] = openFile( filename, ios::out | ios::binary );
  }
}


void SaveFiles::createEventFiles( const EventList &events )
{
  EventFiles.resize( events.size() );

  for ( int k=0; k<events.size(); k++ ) {

    // init event variables:
    EventFiles[k].Events = &events[k];
    EventFiles[k].Offset = events[k].size();
    EventFiles[k].Lines = 0;
    EventFiles[k].SignalEvent = 0;

    // create file:
    if ( events[k].mode() & SaveFilesMode ) {
      Str fn = events[k].ident();
      EventFiles[k].FileName = fn.lower() + "-events.dat";
      EventFiles[k].Stream = openFile( EventFiles[k].FileName, ios::out );
      if ( EventFiles[k].Stream->good() ) {
	// write header:
	*EventFiles[k].Stream << "# events: " << events[k].ident() << '\n';
	*EventFiles[k].Stream << '\n';
	// init key:
	EventFiles[k].Key.clear();
	EventFiles[k].Key.addNumber( "t", "sec", "%0.5f" );
	EventFiles[k].SaveSize = ( events[k].sizeBuffer() && (events[k].mode() & SaveFilesSizeMode) );
	if ( EventFiles[k].SaveSize )
	  EventFiles[k].Key.addNumber( events[k].sizeName(), events[k].sizeUnit(),
				       events[k].sizeFormat() );
	EventFiles[k].SaveWidth = ( events[k].widthBuffer() && (events[k].mode() & SaveFilesWidthMode) );
	if ( EventFiles[k].SaveWidth )
	  EventFiles[k].Key.addNumber( events[k].widthName(), events[k].widthUnit(),
				       events[k].widthFormat() );
	// write key:
	EventFiles[k].Key.saveKey( *EventFiles[k].Stream );
      }
      else {
	EventFiles[k].FileName = "";
	EventFiles[k].Stream->close();
	EventFiles[k].Stream = 0;
      }
    }
    else {
      EventFiles[k].FileName = "";
      EventFiles[k].Stream = 0;
    }
  }
}


void SaveFiles::createStimulusFile( const Acquire &intraces,
				    const EventList &events )
{
  // init stimulus variables:
  StimulusData = false;

  // create file for stimuli:
  SF = openFile( "stimulus-indices.dat", ios::out );

  if ( (*SF) ) {
    // write header:
    *SF << "# analog input devices:\n";
    for ( int k=0; k<intraces.inputsSize(); k++ ) {
      *SF << "#      identifier" + Str( k+1 ) + ": " << intraces.inputDevice( k )->deviceIdent() << '\n';
      *SF << "#            name" + Str( k+1 ) + ": " << intraces.inputDevice( k )->deviceName() << '\n';
      *SF << "#          vendor" + Str( k+1 ) + ": " << intraces.inputDevice( k )->deviceVendor() << '\n';
      *SF << "#          device" + Str( k+1 ) + ": " << intraces.inputDevice( k )->deviceFile() << '\n';
      *SF << "#       data file" + Str( k+1 ) + ": " << "traces-" << Str( k+1 ) << "." << intraces.inputTraces( k )[0].deviceDataTypeId() << Str( intraces.inputTraces( k ).size() ) << '\n';
      *SF << "#          traces" + Str( k+1 ) + ": " << Str( intraces.inputTraces( k ).size() ) << '\n';
      *SF << "# sample interval" + Str( k+1 ) + ": " << Str( 1000.0*intraces.inputTraces( k )[0].sampleInterval(), 0, 2, 'f' ) << "ms\n";
    }
    *SF << "# event lists:\n";
    for ( unsigned int k=0; k<EventFiles.size(); k++ ) {
      if ( ! EventFiles[k].FileName.empty() )
	*SF << "#      event file" + Str( k+1 ) + ": " << EventFiles[k].FileName << '\n';
    }
    *SF << "# analog output traces:\n";
    for ( int k=0; k<RW->AQ->outTracesSize(); k++ ) {
      TraceSpec trace( RW->AQ->outTrace( k ) );
      *SF << "#   identifier" + Str( k+1 ) + ": " << trace.traceName() << '\n';
      *SF << "#       device" + Str( k+1 ) + ": " << trace.device() << '\n';
      *SF << "#      channel" + Str( k+1 ) + ": " << trace.channel() << '\n';
      *SF << "# signal delay" + Str( k+1 ) + ": " << 1000.0*trace.signalDelay() << "ms\n";
      *SF << "# maximum rate" + Str( k+1 ) + ": " << 0.001*trace.maxSampleRate() << "kHz\n";
    }
    *SF << '\n';

    // create key:
    StimulusKey.clear();
    for ( int k=0; k<intraces.inputsSize(); k++ ) {
      StimulusKey.addLabel( "traces" + Str( k+1 ) + " " + intraces.inputDevice( k )->deviceIdent() );
      StimulusKey.addLabel( "index" );
      StimulusKey.addNumber( "index", "word", "%10.0f" );
      for ( int j=0; j<intraces.inputTraces( k ).size(); j++ )
	if ( intraces.inputTraces( k )[j].mode() & SaveFilesMode ) {
	  StimulusKey.addLabel( intraces.inputTraces( k )[j].ident() );
	  StimulusKey.addNumber( "factor", "mV", "%9.4g" );
	  StimulusKey.addNumber( "offset", "mV", "%8.3g" );
	}
    }
    StimulusKey.addLabel( "events" );
    for ( unsigned int k=0; k<EventFiles.size(); k++ )
      if ( EventFiles[k].Stream != 0 ) {
	StimulusKey.addLabel( events[k].ident() );
	StimulusKey.addNumber( "index", "line", "%10.0f" );
	EventFiles[k].SaveMeanRate = ( events[k].mode() & SaveFilesMeanRateMode );
	if ( EventFiles[k].SaveMeanRate )
	  StimulusKey.addNumber( "freq", "Hz", "%6.1f" );
	EventFiles[k].SaveMeanSize = ( events[k].mode() & SaveFilesMeanSizeMode );
	if ( EventFiles[k].SaveMeanSize )
	  StimulusKey.addNumber( events[k].sizeName(), events[k].sizeUnit(), events[k].sizeFormat() );
	EventFiles[k].SaveMeanWidth = ( events[k].mode() & SaveFilesMeanWidthMode );
	if ( EventFiles[k].SaveMeanWidth )
	  StimulusKey.addNumber( events[k].widthName(), events[k].widthUnit(), events[k].widthFormat() );
	EventFiles[k].SaveMeanQuality = ( events[k].mode() & SaveFilesMeanQualityMode );
	if ( EventFiles[k].SaveMeanQuality )
	  StimulusKey.addNumber( "quality", "%", "%3.0f" );
      }
    lock();
    if ( !Options::empty() ) {
      StimulusKey.addLabel( "data" );
      StimulusKey.addLabel( "data" );
      const Options &data = *this;
      for( int k=0; k<data.size(); k++ )
	StimulusKey.addNumber( data[k].ident(), data[k].outUnit(), data[k].format() );
    }
    unlock();
    StimulusKey.addLabel( "stimulus" );
    StimulusKey.addLabel( "timing" );
    StimulusKey.addNumber( "time", "s", "%11.5f" );
    StimulusKey.addNumber( "delay", "ms", "%5.1f" );
    for ( int k=0; k<RW->AQ->outTracesSize(); k++ ) {
      StimulusKey.addLabel( RW->AQ->outTraceName( k ) );
      StimulusKey.addNumber( "rate", "kHz", "%8.3f" );
      StimulusKey.addNumber( "duration", "ms", "%8.0f" );
      const Attenuate *att = RW->AQ->outTraceAttenuate( k );
      if ( att != 0 ) {
	StimulusKey.addNumber( att->intensityName(), att->intensityUnit(),
			       att->intensityFormat() );
	if ( ! att->frequencyName().empty() )
	  StimulusKey.addNumber( att->frequencyName(), att->frequencyUnit(),
				 att->frequencyFormat() );
      }
      StimulusKey.addText( "signal", -30 );
    }
  }
}


void SaveFiles::openFiles( const Acquire &intraces, const EventList &events )
{
  // nothing to be done, if files are already open:
  if ( FilesOpen )
    return;

  // close all open files:
  closeFiles();
  clearRemoveFiles();

  // reset variables:
  ToggleData = false;
  ToggleOn = true;
  Writing = false;

  ReProData = false;
  ReProName = "";
  ReProAuthor = "";
  ReProVersion = "";
  ReProDate = "";
  ReProSettings.clear();

  setPath( defaultPath() );

  // get current time:
  time_t currenttime = RW->SN->startSessionTime();
  // time changed?
  if ( difftime( currenttime, PathTime ) != 0.0  )
    PathNumber = 0;
  PathTime = currenttime;

  // generate unused name for new files/directory:
  Str pathname = "";
  PathNumber++;
  int az = ('z'-'a'+1);
  for ( ; PathNumber <= az*az; PathNumber++ ) {

    // create file name:
    pathname = PathTemplate;
    pathname.format( localtime( &PathTime ) );
    pathname.format( PathNumber, 'n', 'd' );
    int n = PathNumber-1;
    Str s = char( 'a' + n%az );
    n /= az;
    while ( n > 0 ) {
      s.insert( 0u, 1u, char( 'a' + n%az ) );
      n /= az;
    }
    pathname.format( s, 'a' );
    s.upper();
    pathname.format( s, 'A' );

    if ( pathname[pathname.size()-1] == '/' ) {
      // try to create new directory:
      char s[200];
      sprintf( s, "%s %s", "mkdir", pathname.c_str() );
      // success?
      if ( system( s ) == 0 )
	break;
    }
    else {
      // try to open files:
      string fs = path() + "trigger.dat";
      ifstream f( fs.c_str() );
      // files do not exist?
      if ( ! f.good() )
	break;
    }
  }
  // running out of names?
  if ( PathNumber > az*az ) {
    RW->printlog( "! panic: SaveFiles::openFiles -> can't create data file!" );
    return;
  }
  // valid base name found:
  setPath( pathname );

  // open files:
  createTraceFile( intraces );
  createEventFiles( events );
  createStimulusFile( intraces, events );
  FilesOpen = true;

  // message:
  RW->printlog( "save in " + path() );

  // update widget:
  FileLabel->setFont( HighlightFont );
  FileLabel->setPalette( HighlightPalette );
  FileLabel->setText( path().c_str() );
  SaveLabel->setSpike( true );
}


void SaveFiles::closeFiles( void )
{
  ToggleData = true;
  ToggleOn = false;
  writeStimulus();

  for ( unsigned int k=0; k<VF.size(); k++ ) {
    if ( VF[k] != 0 )
      delete VF[k];
  }
  VF.clear();
  for ( unsigned int k=0; k<EventFiles.size(); k++ ) {
    if ( EventFiles[k].Stream != 0 ) {
      EventFiles[k].Stream->close();
      delete EventFiles[k].Stream;
    }
  }
  EventFiles.clear();
  if ( SF != 0 )
    delete SF;
  SF = 0;

  FilesOpen = false;

  SaveLabel->setSpike( false );
}


void SaveFiles::deleteFiles( void )
{
  closeFiles();

  // remove all files:
  removeFiles();

  if ( path() != defaultPath() && 
       path() != "" &&
       path()[path().size()-1] == '/' ) {
    // remove the whole directory:
    string s = "rm -f -r " + path();
    system( s.c_str() );
  }

  // message:
  RW->printlog( "discarded " + path() );
  FileLabel->setPalette( NormalPalette );
  FileLabel->setFont( NormalFont );
  FileLabel->setText( "deleted" );

  // back to default path:
  setPath( defaultPath() );
  PathNumber--;
}


void SaveFiles::completeFiles( void )
{
  closeFiles();

  // no files need to be deleted:
  clearRemoveFiles();

  // message:
  RW->printlog( "saved as " + path() );
  FileLabel->setPalette( NormalPalette );

  // back to default path:
  setPath( defaultPath() );
}



}; /* namespace relacs */

#include "moc_savefiles.cc"

