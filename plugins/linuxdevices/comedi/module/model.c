
  /*! Name, by which this module is known inside Linux: */
char *moduleName;

  /*! The period length of the realtime periodic task in seconds. */
float loopInterval;
  /*! One over the period length of the realtime periodic task in Hertz. */
float loopRate;

  /*! Analog input that is read from the DAQ board. */
#define INPUT_N 1
  /*! The \a inputNames are used to match the \a input variables with
      analog input traces in Relacs. */
char *inputNames[INPUT_N] = { "V-1" };
char *inputUnits[INPUT_N] = { "mV" };
  /*! The \a inputChannels and \a inputDevices are set automatically. */
int inputChannels[INPUT_N];
int inputDevices[INPUT_N];
  /*! \a input holds the current value that was read in from the DAQ board. */
float input[INPUT_N] = { 0.0 };

  /*! Analog output that is written to the DAQ board. */
#define OUTPUT_N 1
char *outputNames[OUTPUT_N] = { "Current-1" };
char *outputUnits[OUTPUT_N] = { "nA" };
int outputChannels[OUTPUT_N];
int outputDevices[OUTPUT_N];
float output[OUTPUT_N] = { 0.0 };

  /*! Parameter that are provided by the model and can be read out. */
#define PARAMINPUT_N 1
char *paramInputNames[PARAMINPUT_N] = { "LeakCurrent" };
char *paramInputUnits[PARAMINPUT_N] = { "nA" };
float paramInput[PARAMINPUT_N] = { 0.0 };

  /*! Parameter that are read by the model and are written to the model. */
#define PARAMOUTPUT_N 2
char *paramOutputNames[PARAMOUTPUT_N] = { "g", "E" };
char *paramOutputUnits[PARAMOUTPUT_N] = { "nS", "mV" };
float paramOutput[PARAMOUTPUT_N] = { 0.0, 0.0 };

  /*! Variables used by the model. */

void initModel( void )
{
   moduleName = "/dev/dynclamp";
}

void computeModel( void )
{
   output[0] = -0.001*paramOutput[0]*(input[0]-paramOutput[1]);
   paramInput[0] = output[0];
}
