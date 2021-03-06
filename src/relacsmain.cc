/*
  relacsmain.cc
  

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

#include <string>
#include <getopt.h>
#include <QApplication>
#include <QSplashScreen>
#include <QThread>
#include <relacs/relacswidget.h>

using namespace std;


class Sleep : public QThread
{
public:
  static void sleep( unsigned long secs ) {
    QThread::sleep( secs );
  }
};


int main( int argc, char **argv )
{
  relacs::RELACSWidget::ModeTypes mode = relacs::RELACSWidget::AcquisitionMode;
  bool fullscreen = false;
  bool splashscreen = false;
  string pluginrelative = "./";
  string pluginhomes = "";
  string pluginhelp = "";
  string coreconfigfiles = "relacs.cfg";
  string pluginconfigfiles = "relacsplugins.cfg";
  string docpath = "";
  string cfgexamplespath = "";
  string iconpath = "";
  bool doxydoc = false;

  static struct option longoptions[] = {
    { "version", 0, 0, 0 },
    { "help", 0, 0, 0 },
    { "plugin-relative-dir", 1, 0, 0 },
    { "plugin-home-dirs", 1, 0, 0 },
    { "plugin-help-dirs", 1, 0, 0 },
    { "core-config-files", 1, 0, 0 },
    { "plugins-config-files", 1, 0, 0 },
    { "doc-path", 1, 0, 0 },
    { "cfgexamples-path", 1, 0, 0 },
    { "icon-path", 1, 0, 0 },
    { "with-doxygen", 0, 0, 0 },
    { 0, 0, 0, 0 }
  };
  optind = 0;
  opterr = 0;
  int longindex = 0;
  char c;
  while ( (c = getopt_long( argc, argv, "f3p", longoptions, &longindex )) >= 0 ) {
    switch ( c ) {
    case 0: switch ( longindex ) {
      case 0:
	cout << "RELACS " << RELACSVERSION << endl;
	cout << "Copyright (C) 2002-2012 Jan Benda\n";
	exit( 0 );
	break;
      case 1:
	cout << "relacsgui should not be called directly\n"
	     << "Use 'relacs' instead!\n";
	exit( 0 );
	break;
      case 2:
	if ( optarg && *optarg != '\0' )
	  pluginrelative = optarg;
	break;
      case 3:
	if ( optarg && *optarg != '\0' )
	  pluginhomes = optarg;
	break;
      case 4:
	if ( optarg && *optarg != '\0' )
	  pluginhelp = optarg;
	break;
      case 5:
	if ( optarg && *optarg != '\0' )
	  coreconfigfiles = optarg;
	break;
      case 6:
	if ( optarg && *optarg != '\0' )
	  pluginconfigfiles = optarg;
	break;
      case 7:
	if ( optarg && *optarg != '\0' )
	  docpath = optarg;
	break;
      case 8:
	if ( optarg && *optarg != '\0' )
	  cfgexamplespath = optarg;
	break;
      case 9:
	if ( optarg && *optarg != '\0' )
	  iconpath = optarg;
	break;
      case 10:
	doxydoc = true;
	break;
      }
      break;

    case 'f':
      fullscreen = true;
      break;

    case 'p':
      splashscreen = true;
      break;

    case '3':
      mode = relacs::RELACSWidget::SimulationMode;
      break;

    default:
      break;
    }
  }

  QApplication::setColorSpec( QApplication::CustomColor );
  QApplication app( argc, argv );
  
  QPixmap pixmap( string( iconpath + "/relacssplash.png" ).c_str() );
  QSplashScreen *splash = 0;
  if ( splashscreen ) {
    splash = new QSplashScreen( pixmap );
    splash->setFont( QFont( "Helvetica", 18, QFont::Bold ) );
    splash->show();
    splash->showMessage( "Loading ...", Qt::AlignLeft | Qt::AlignBottom );
  }

  relacs::RELACSWidget relacs( pluginrelative, pluginhomes, pluginhelp,
			       coreconfigfiles, pluginconfigfiles, 
			       docpath, cfgexamplespath,
			       iconpath, doxydoc, mode );

  if ( splashscreen ) {
    Sleep::sleep( 2 );
    splash->showMessage( "Finished ...", Qt::AlignLeft | Qt::AlignBottom );
  }

  if ( fullscreen )
    relacs.fullScreen();

  relacs.show();
  relacs.init();

  if ( splashscreen ) {
    splash->finish( &relacs );
    delete splash;
  }

  return app.exec();
}
