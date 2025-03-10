#ifndef WCSimWCDigitizer_h
#define WCSimWCDigitizer_h 1

#include "WCSimRootOptions.hh"
#include "WCSimDarkRateMessenger.hh"
#include "WCSimWCDAQMessenger.hh"
#include "WCSimDetectorConstruction.hh"
#include "WCSimEnumerations.hh"
#include "G4VDigitizerModule.hh"
#include "WCSimWCDigi.hh"
#include "WCSimWCHit.hh"
#include "globals.hh"
#include "Randomize.hh"
#include <map>
#include <vector>


// *******************************************
// BASE CLASS
// *******************************************

class WCSimWCDigitizerBase : public G4VDigitizerModule
{
public:
  
  WCSimWCDigitizerBase(G4String name, WCSimDetectorConstruction*, WCSimWCDAQMessenger*, DigitizerType_t, G4String detectorElement);
  virtual ~WCSimWCDigitizerBase();
  
  bool AddNewDigit(int tube, int gate, float digihittime, float peSmeared, std::vector<int> digi_comp);
  virtual void DigitizeHits(WCSimWCDigitsCollection* WCHCPMT) = 0;
  void DigitizeGate(WCSimWCDigitsCollection* WCHC,G4int G);
  void Digitize();

  //.mac file option setting methods
  void SetDigitizerDeadTime         (int deadtime) { DigitizerDeadTime = deadtime;         }; ///< Override the default digitizer deadtime (ns)
  void SetDigitizerIntegrationWindow(int inttime ) { DigitizerIntegrationWindow = inttime; }; ///< Override the default digitizer integration window (ns)
  void SetExtendIntegrationWindow   (bool extendwindow) { ExtendDigitizerIntegrationWindow = extendwindow; };
  void SetDoPhotonIntegration(bool dointegration) { DoPhotonIntegration = dointegration; };

  ///Save current values of options
  void SaveOptionsToOutput(WCSimRootOptions * wcopt);
  
protected:
  void ReInitialize() { DigiStoreHitMap.clear(); }

  G4double peSmeared;

  WCSimDetectorConstruction* myDetector; ///< Get the geometry information
  WCSimWCDAQMessenger* DAQMessenger;     ///< Get the /DAQ/ .mac options

  WCSimWCDigitsCollection*  DigiStore;
  std::map<int,int> DigiStoreHitMap;   ///< Used to check if a digit has already been created on a PMT

  //generic digitizer properties. Defaults set with the GetDefault*() methods. Overidden by .mac options
  G4String DigitizerClassName;    ///< Name of the digitizer class being run
  int DigitizerDeadTime;          ///< Digitizer deadtime (ns)
  int DigitizerIntegrationWindow; ///< Digitizer integration window (ns)
  bool ExtendDigitizerIntegrationWindow;  // extend integration window when a new hit arrives in an existing window
  bool DoPhotonIntegration;       ///< Whether to perform integration of photons, or just convert each photon hit into a Digit

  DigitizerType_t DigitizerType; ///< Enumeration to say which digitizer we've constructed

  virtual int GetDefaultDeadTime() = 0;          ///< Set the default digitizer-specific deadtime (in ns) (overridden by .mac)
  virtual int GetDefaultIntegrationWindow() = 0; ///< Set the default digitizer-specific integration window (in ns) (overridden by .mac)
  virtual bool GetDefaultExtendIntegrationWindow() = 0;
  virtual bool GetDefaultDoPhotonIntegration() = 0; ///< Set the default digitizer-specific integration mode (overridden by .mac)

  void GetVariables(); ///< Get the default deadtime, etc. from the derived class, and override with read from the .mac file
  
  G4String detectorElement;
};



// *******************************************
// DERIVED CLASSES
// *******************************************


// SKI-based digitizer class
class WCSimWCDigitizerSKI : public WCSimWCDigitizerBase 
{
public:
  
  WCSimWCDigitizerSKI(G4String name, WCSimDetectorConstruction*, WCSimWCDAQMessenger*, G4String detectorElement);
  ~WCSimWCDigitizerSKI();

  void DigitizeHits(WCSimWCDigitsCollection* WCHCPMT);

private:
  int GetDefaultDeadTime()          { return 0; }   ///< SKI digitizer deadtime is 0 ns
  int GetDefaultIntegrationWindow() { return 200; } ///< 
  bool GetDefaultExtendIntegrationWindow(){ return false; }
  bool GetDefaultDoPhotonIntegration()    { return true; }

  static void Threshold(double& pe,int& iflag){
    //   CLHEP::HepRandom::setTheSeed(pe+2014);
    double x = pe+0.1; iflag=0;
    double thr; double RDUMMY,err;
    if ( x<1.1) {
      thr = std::min(1.0,
		     -0.06374+x*(3.748+x*(-63.23+x*(452.0+x*(-1449.0+x*(2513.0
									+x*(-2529.+x*(1472.0+x*(-452.2+x*(51.34+x*2.370))))))))));
    } else {
      thr = 1.0;
    }
    RDUMMY = G4UniformRand();
    if (thr < RDUMMY) {
      pe = 0.0;
      iflag = 1;
    }
    else {
      err = G4RandGauss::shoot(0.0,0.03);
      /////      call rngaus(0.0, 0.03, err);
      pe = pe+err;
    }
  }
};


#endif //WCSimWCDigitizer_h
