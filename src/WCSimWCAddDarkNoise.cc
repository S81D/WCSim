#include "WCSimWCAddDarkNoise.hh"
#include "WCSimWCPMT.hh"
#include "WCSimWCLAPPD.hh"
#include "WCSimWCDigi.hh"
#include "WCSimWCHit.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4DigiManager.hh"
#include "G4ios.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4LogicalVolumeStore.hh"

#include "WCSimDetectorConstruction.hh"
#include "WCSimPmtInfo.hh"
#include "WCSimDarkRateMessenger.hh"

#include "WCSimWCTrigger.hh"

#include <vector>
#include <utility>
#include <numeric> // for std::iota
// for memset
#include <cstring>

#ifndef WCSIMWCADDDARKNOISE_VERBOSE
//#define WCSIMWCADDDARKNOISE_VERBOSE
//#define NPMTS_VERBOSE 1000
#endif

#ifndef HYPER_VERBOSITY
//#define HYPER_VERBOSITY
#endif

WCSimWCAddDarkNoise::WCSimWCAddDarkNoise(G4String name,
					 WCSimDetectorConstruction* inDetector, G4String detectorElement)
  :G4VDigitizerModule(name), fCalledAddDarkNoise(false), myDetector(inDetector), detectorElement(detectorElement)
{
  //Set defaults to be unphysical, so that we know if they have been overwritten by the user
  PMTDarkRate = -99;
  ConvRate    = -99;
  EffectivePMTDarkRate = -99;

  //Get the user options
  //DarkRateMessenger = new WCSimDarkRateMessenger(this);
  DarkRateMessenger = WCSimDarkRateMessenger::GetInstance();
  DarkRateMessenger->AddDarkRateInstance(this, detectorElement);
  DarkRateMessenger->Initialize();
  ReInitialize();
}

WCSimWCAddDarkNoise::~WCSimWCAddDarkNoise(){
  //delete DarkRateMessenger;
  DarkRateMessenger->RemoveDarkRateInstance(detectorElement);	// DarkRateMessenger singleton calls it's destructor when map is empty
  DarkRateMessenger = 0;
}

void WCSimWCAddDarkNoise::SetPMTDarkDefaults()
{
  //Grab Dark Rate and Conversion from PMT itself
  G4String WCCollectionName;
  if(detectorElement=="tank"){
    WCCollectionName = myDetector->GetIDCollectionName();
  } else if(detectorElement=="mrd"){
    WCCollectionName = myDetector->GetMRDCollectionName();
  } else if(detectorElement=="facc"){
    WCCollectionName = myDetector->GetFACCCollectionName();
  }
  
  WCSimPMTObject* PMT=0;
  if(WCCollectionName!="WCIDCollectionNameIsUnused") PMT = myDetector->GetPMTPointer(WCCollectionName);

  double const conversion_to_kHz = 1000000; //ToDo: remove this & treat DarkRate in CLHEP units throughout
  double defaultPMTDarkRate = (PMT) ? PMT->GetDarkRate() * conversion_to_kHz : -99;
  double defaultConvRate = (PMT) ? PMT->GetDarkRateConversionFactor() : -99;

  //Only set the defaults if the user hasn't overwritten the unphysical defaults
  if(PMTDarkRate < -98) PMTDarkRate = defaultPMTDarkRate;
  if(ConvRate < -98) ConvRate = defaultConvRate;
    
  // if there are multiple tank PMT types, retrieve the dark rate for each tank collection
  PMTDarkRateMap.clear();
  ConvRateMap.clear();
  if(detectorElement!="tank"||WCCollectionName!="WCIDCollectionNameIsUnused"){
    // if not multiple types, just put the exisiting ones into the maps
    ConvRateMap.emplace(WCCollectionName, ConvRate);
    PMTDarkRateMap.emplace(WCCollectionName, PMTDarkRate);
    EffectivePMTDarkRate = PMTDarkRate;
  } else {
    // if we have multiple types, scan the collections, and add each noise rate to the map in turn
    int numtankpmts = myDetector->GetTotalNumPmts();
    std::vector<G4String> WCTankCollectionNames = myDetector->GetIDCollectionNames();
    for(auto acollection : WCTankCollectionNames){
      WCSimPMTObject* PMT = myDetector->GetPMTPointer(acollection);
      defaultPMTDarkRate = PMT->GetDarkRate() * conversion_to_kHz;
      defaultConvRate = PMT->GetDarkRateConversionFactor();
      if(PMTDarkRate < -98) PMTDarkRate = defaultPMTDarkRate;
      if(ConvRate < -98) ConvRate = defaultConvRate;
      PMTDarkRateMap.emplace(acollection, PMTDarkRate);
      ConvRateMap.emplace(acollection, ConvRate);
     
      std::vector<int> pmtsinthiscollection = myDetector->GetTubesInCollection(acollection);
      int npmtscol = pmtsinthiscollection.size();
      EffectivePMTDarkRate+= PMTDarkRate * (npmtscol/numtankpmts);
    }
  }
}

void WCSimWCAddDarkNoise::AddDarkNoise(){
  //Grab the PMT-specific defaults
  if(!fCalledAddDarkNoise) {
    SetPMTDarkDefaults();
    fCalledAddDarkNoise = true;
  }

  //clear the result and range vectors
  ReInitialize();

  G4DigiManager* DigiMan = G4DigiManager::GetDMpointer();
  // Get the PMT collection ID
  G4String thecollectionName;
  if(detectorElement=="tank"){
    thecollectionName="WCRawPMTSignalCollection";
  } else if(detectorElement=="mrd"){
    thecollectionName="WCRawPMTSignalCollection_MRD";
  } else if(detectorElement=="facc"){
    thecollectionName="WCRawPMTSignalCollection_FACC";
  }
  
  G4int WCHCID = DigiMan->GetDigiCollectionID(thecollectionName);
  // Get the PMT Digits collection
  WCSimWCDigitsCollection* WCHCPMT =
    (WCSimWCDigitsCollection*)(DigiMan->GetDigiCollection(WCHCID));
#ifdef HYPER_VERBOSITY
  if(detectorElement=="tank"){G4cout<<"WCSimWCAddDarkNoise::AddDarkNoise ☆ retrieved hit collection (WCSimWCDigitsCollection*)"<<thecollectionName<<" which has "<<WCHCPMT->entries()<<" entries"<<G4endl;}
#endif
  G4String thetriggertype="";
  if(detectorElement=="mrd"){	// check to see if this detector element uses the tank for triggering
    WCSimWCTriggerBase* WCTM = (WCSimWCTriggerBase*)DigiMan->FindDigitizerModule("WCReadout_MRD");
    thetriggertype = WCTM->GetTriggerClassName();
  } else if(detectorElement=="facc"){
    WCSimWCTriggerBase* WCTM = (WCSimWCTriggerBase*)DigiMan->FindDigitizerModule("WCReadout_FACC");
    thetriggertype = WCTM->GetTriggerClassName();
  }
  WCSimWCDigitsCollection* WCHCPMT_tank=NULL;
  if(thetriggertype=="TankDigits"&&detectorElement!="tank"){
    // if triggertype is TankDigits, add noise in windows around the *tank* digits, not this
    // collection's digits, since tank digits determine read out windows
    int WCHCID_tank = DigiMan->GetDigiCollectionID("WCRawPMTSignalCollection");
    WCHCPMT_tank = (WCSimWCDigitsCollection*)(DigiMan->GetDigiCollection(WCHCID_tank));
#ifdef HYPER_VERBOSITY
    G4cout<<"WCSimWCAddDarkNoise::AddDarkNoise ☆ retrieved hit collection (WCSimWCDigitsCollection*)WCRawPMTSignalCollection for finding dark noise windows, which has "<<WCHCPMT_tank->entries()<<" entries"<<G4endl;
#endif
  }
  
  if (( (WCHCPMT != NULL) ||(thetriggertype=="TankDigits"&&WCHCPMT_tank!=NULL)) && (this->PMTDarkRate > 1E-307)) {
    //Determine ranges for adding noise
    if(DarkMode == 1){
      if(thetriggertype=="TankDigits"){
        WCSimWCAddDarkNoise* WCDNM_tank = (WCSimWCAddDarkNoise*)DigiMan->FindDigitizerModule("WCDarkNoise");
        double DarkWindow_tank = WCDNM_tank->GetDarkWindow();
#ifdef HYPER_VERBOSITY
        if(detectorElement=="tank"){
        G4cout<<"WCSimWCAddDarkNoise::AddDarkNoise ☆ calling FindDarkNoiseRanges() with tank raw collection and darkwindow size"<<G4endl;}
#endif
        FindDarkNoiseRanges(WCHCPMT_tank,DarkWindow_tank);
      } else {
        FindDarkNoiseRanges(WCHCPMT,this->DarkWindow);
      }
    }
    else if(DarkMode == 0) {
      result.push_back(std::pair<float,float>(DarkLow,DarkHigh));
    }
    //Call routine to add dark noise here.
    //loop over pairs which represent ranges.
    //Add noise to those ranges
#ifdef HYPER_VERBOSITY
    if(detectorElement=="tank"){
    G4cout<<"WCSimWCAddDarkNoise::AddDarkNoise ☆ adding dark noise hits in "<<result.size()<<" window(s) around found digits."<<G4endl;}
#endif
    int windowfordarknoise=0;
    for(std::vector<std::pair<float, float> >::iterator it2 = result.begin(); it2 != result.end(); it2++) {
#ifdef HYPER_VERBOSITY
      if(detectorElement=="tank"){
      G4cout<<"WCSimWCAddDarkNoise::AddDarkNoise ☆ adding dark noise in window "<<windowfordarknoise<<G4endl; windowfordarknoise++;}
#endif
      AddDarkNoiseBeforeDigi(WCHCPMT,it2->first,it2->second);
    }
  }
}

void WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi(WCSimWCDigitsCollection* WCHCPMT, float num1 ,float num2) {
    // Introduces dark noise into each PMT during an event window
    // This won't introduce noise only events, and isn't written
    // to handle different rates for each PMT (although this shouldn't
    // be too difficult to add at a later time)
    // 
    // Added by: Morgan Askins (maskins@ucdavis.edu)

    G4int number_entries = WCHCPMT->entries();
#ifdef HYPER_VERBOSITY
    if(detectorElement=="tank"){
    G4cout<<"WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi ☆ adding dark noise to collection of "<<number_entries<<" entries"<<G4endl;}
#endif
    G4int number_pmts;
    if(detectorElement=="tank"){
      number_pmts = myDetector->GetTotalNumPmts();
    } else if(detectorElement=="mrd"){
      number_pmts = myDetector->GetTotalNumMrdPmts();
    } else if(detectorElement=="facc"){
      number_pmts = myDetector->GetTotalNumFaccPmts();
    }
    int *PMTindex = new int [number_pmts+1];

    //initialize PMTindex
    for (int l=0; l<number_pmts+1; l++){
      PMTindex[l] =0;
    }

    //    G4cout<<"entries before "<<WCHCPMT->entries()<<"\n";
    //Set up proper indices for tubes which have already been hit
    int num_hit_b4=0;
    for (int g=0; g<number_entries; g++){
      G4int tube = (*WCHCPMT)[g]->GetTubeID();
      //G4cout<<"totalpe "<<tube<<" "<<(*WCHCPMT)[g]->GetTotalPe()<<"\n";
      //should this be tube-1?
      PMTindex[tube] += (*WCHCPMT)[g]->GetTotalPe();
      num_hit_b4     += (*WCHCPMT)[g]->GetTotalPe();
      //G4cout<<"TotalPe "<<(*WCHCPMT)[g]->GetTotalPe()<<" "<<PMTindex[tube]<<"\n";
    }

    // Get the info for pmt positions
    std::vector<WCSimPmtInfo*> *pmts;
    if(detectorElement=="tank"){
      pmts = myDetector->Get_Pmts();
    } else if(detectorElement=="mrd"){
      pmts = myDetector->Get_MrdPmts();
    } else if(detectorElement=="facc"){
      pmts = myDetector->Get_FaccPmts();
    }
    // It works out that the pmts here are ordered !
    // pmts->at(i) has tubeid i+1
    
    std::vector<int> list;
    list.assign( number_pmts+1, 0 );

    for( int h = 0; h < number_entries; h++ ) {
      list[(*WCHCPMT)[h]->GetTubeID()] = h+1;
    }
   
    // Add noise to PMT's here, do so in the range num1 to num2
    double current_time = 0;
    double pe = 0.0;
    //Calculate the time window size
    double windowsize = num2 - num1;

    G4DigiManager* DMman = G4DigiManager::GetDMpointer();
    G4String thewcpmtname;
    if(detectorElement=="tank"){
      thewcpmtname="WCReadoutPMT";
    } else if(detectorElement=="mrd"){
      thewcpmtname="WCReadoutPMT_MRD";
    } else if(detectorElement=="facc"){
      thewcpmtname="WCReadoutPMT_FACC";
    }
    WCSimWCPMT* WCPMT = (WCSimWCPMT*)DMman->FindDigitizerModule(thewcpmtname);
#ifdef HYPER_VERBOSITY
    if(detectorElement=="tank"){G4cout<<"WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi ☆ getting (WCSimWCPMT*)"<<thewcpmtname<<G4endl;}
#endif
  
  // loop over the different PMT types if applicable
  for(auto anoisegroup : PMTDarkRateMap){
    // each element of PMTDarkRateMap represents a subset of PMTs in WCRawPMTSignalCollection
    // of a particular PMT type, with a unique dark rate.
    G4String WCCollectionName = anoisegroup.first;
#ifdef WCSIMWCADDDARKNOISE_VERBOSE
    G4cout<<"adding dark noise on PMT collection "<<WCCollectionName<<G4endl;
#endif
    G4double currentPMTDarkRate = anoisegroup.second;
    std::vector<Int_t> pmtsinthiscollection;
    if(detectorElement=="tank"&&myDetector->GetIDCollectionNames().size()>1){
      pmtsinthiscollection = myDetector->GetTubesInCollection(WCCollectionName);
      number_pmts = pmtsinthiscollection.size();
    } else {
      pmtsinthiscollection.resize(number_pmts);
      std::iota(pmtsinthiscollection.begin(), pmtsinthiscollection.end(), 1); // tube IDs start from 1 NOT 0
    }
  
    //average number of PMTs with noise
    double ave=number_pmts * currentPMTDarkRate * ConvRateMap.at(WCCollectionName) * windowsize * 1E-6;

    //poisson distributed noise, number of noise hits to add
    int nnoispmt = CLHEP::RandPoisson::shoot(ave);
    
    // get the pmt object corresponding to the hits collection we're faking for rn1pe and log vol
    WCSimPMTObject* PMT = myDetector->GetPMTPointer(WCCollectionName);
    
    G4String thecollectionName;
    if(detectorElement=="tank"){
      thecollectionName="WCRawPMTSignalCollection";
    } else if(detectorElement=="mrd"){
      thecollectionName="WCRawPMTSignalCollection_MRD";
    } else if(detectorElement=="facc"){
      thecollectionName="WCRawPMTSignalCollection_FACC";
    }
#ifdef HYPER_VERBOSITY
    if(detectorElement=="tank"){G4cout<<"WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi ☆ adding "<<nnoispmt<<" dark hits to "<<thecollectionName<<G4endl;}
#endif
    
#ifdef WCSIMWCADDDARKNOISE_VERBOSE
    G4cout << "WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi Going to add " << nnoispmt << " dark noise hits in time window [" << num1 << "," << num2 << "]" << G4endl;
#endif
    for( int i = 0; i < nnoispmt; i++ )
      {
	//time of noise hit to be generated
	//A time from t=num1 to num2
	current_time = num1 + G4UniformRand()*windowsize;
	//now a random PMT.  Assuming noise levels are the same for
	//each PMT.
	int noise_pmt_index = static_cast<int>( G4UniformRand() * number_pmts ); //so that pmt numbers runs from 1 to Npmt
	if(noise_pmt_index>pmtsinthiscollection.size()) 
	    G4cerr<<" NOISE PMT INDEX "<<noise_pmt_index<<" OUTSIDE RANGE "<<pmtsinthiscollection.size()<<G4endl;
	int noise_pmt = pmtsinthiscollection.at(noise_pmt_index);
      
	if( list[ noise_pmt ] == 0 )
	{
	    //PMT has no hits yet. Create a new WCSimWCDigi
	    WCSimWCDigi* ahit = new WCSimWCDigi();
	    ahit->SetTubeID( noise_pmt);
	    //G4cout<<"setting new noise pmt "<<noise_pmt<<" "<<ahit->GetTubeID()<<"\n";
	    // This Logical volume is GlassFaceWCPMT
	    ahit->SetLogicalVolume(G4LogicalVolumeStore::GetInstance()->GetVolume(WCCollectionName));
	    //G4cout<<"1 "<<(G4LogicalVolumeStore::GetInstance()->GetVolume("glassFaceWCPMT"))->GetName()<<"\n";
	    //G4cout<<"2 "<<(*WCHCPMT)[0]->GetLogicalVolume()->GetName()<<"\n";
	    ahit->SetTrackID(-1);
	    ahit->SetParentID(PMTindex[noise_pmt], -1);
	    // Set the position and rotation of the pmt
	    Float_t hit_pos[3];
	    Float_t hit_rot[3];
	    // TODO: need to change the format of hit_pos to G4ThreeVector
	    // and change hit_rot to G4RotationMatrix
	    
	    WCSimPmtInfo* pmtinfo = (WCSimPmtInfo*)pmts->at( noise_pmt -1 );
	    hit_pos[0] = 10*pmtinfo->Get_transx();
	    hit_pos[1] = 10*pmtinfo->Get_transy();
	    hit_pos[2] = 10*pmtinfo->Get_transz();
	    hit_rot[0] = pmtinfo->Get_orienx();
	    hit_rot[1] = pmtinfo->Get_orieny();
	    hit_rot[2] = pmtinfo->Get_orienz();
	    G4RotationMatrix pmt_rotation(hit_rot[0], hit_rot[1], hit_rot[2]);
	    G4ThreeVector pmt_position(hit_pos[0], hit_pos[1], hit_pos[2]);
	    ahit->SetRot(pmt_rotation);
	    ahit->SetPos(pmt_position);
	    ahit->SetTime(PMTindex[noise_pmt],current_time);
	    ahit->SetPreSmearTime(PMTindex[noise_pmt],current_time); //presmear==postsmear for dark noise
	    pe = WCPMT->rn1pe(PMT);
	    ahit->SetPe(PMTindex[noise_pmt],pe);
	    //Added this line to increase the totalPe by 1
	    ahit->AddPe(current_time);
	    WCHCPMT->insert(ahit);
	    PMTindex[noise_pmt]++;
	    number_entries ++;
	    list[ noise_pmt ] = number_entries; // Add this PMT to the end of the list
#ifdef WCSIMWCADDDARKNOISE_VERBOSE
	    if(noise_pmt < NPMTS_VERBOSE)
	      G4cout << "WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi Added NEW DIGI with dark noise hit at time " << current_time << " to PMT " << noise_pmt << G4endl;
#endif
	  }
	else {
	  (*WCHCPMT)[ list[noise_pmt]-1 ]->AddPe(current_time);
	  pe = WCPMT->rn1pe(PMT);
	  (*WCHCPMT)[ list[noise_pmt]-1 ]->SetPe(PMTindex[noise_pmt],pe);
	  (*WCHCPMT)[ list[noise_pmt]-1 ]->SetTime(PMTindex[noise_pmt],current_time);
	  (*WCHCPMT)[ list[noise_pmt]-1 ]->SetPreSmearTime(PMTindex[noise_pmt],current_time); //presmear==postsmear for dark noise
	  (*WCHCPMT)[ list[noise_pmt]-1 ]->SetParentID(PMTindex[noise_pmt],-1);
	  PMTindex[noise_pmt]++;
#ifdef WCSIMWCADDDARKNOISE_VERBOSE
	  if(noise_pmt < NPMTS_VERBOSE)
	    G4cout << "WCSimWCAddDarkNoise::AddDarkNoiseBeforeDigi Added to exisiting digi a dark noise hit at time " << current_time << " to PMT " << noise_pmt << G4endl;
#endif
	}
		
      }//i (number of noise hits to add)
    }
    delete [] PMTindex;
    return;
}



void WCSimWCAddDarkNoise::FindDarkNoiseRanges(WCSimWCDigitsCollection* WCHCPMT, float width) {
  //Loop over all Hits and assign a time window around each hit
  //store these in the ranges vector as pairs
  for (int g=0; g<WCHCPMT->entries(); g++){
    for (int gp=0; gp<(*WCHCPMT)[g]->GetTotalPe(); gp++){
      float time = (*WCHCPMT)[g]->GetTime(gp);
      //lets assume a 5us window.  So we centre this on the hit time.
      //t1 is the lower limit of the window.
      float t1=time - width/2.;
      float t2=time + width/2.;
      ranges.push_back(std::pair<float, float>(t1, t2));
      //if(detectorElement=="mrd"){
      //G4cout<<"WCSimWCAddDarkNoise::FindDarkNoiseRanges ☆ adding dark noise window ["<<t1<<","<<t2<<"] to "<<detectorElement<<G4endl;}
    }
  }
#ifdef HYPER_VERBOSITY
  G4cout<<"WCSimWCAddDarkNoise::FindDarkNoiseRanges ☆ "<<ranges.size()<<" windows around digits found before pruning."<<G4endl;
#endif
  //we need to ensure that the ranges found above are sorted first
  //for the algorithm below to work
  sort(ranges.begin(),ranges.end());

  //check if the vector range has any entries
  //If no entries this indicates that no digits were
  //found in the WCSimWCDigitsCollection, which can cause 
  //segmentation faults in the next part of the code in this method
  //which searches for overlapping ranges.
  //Set range vector to have one element from 0 to 0 (so no noise digits will be added)
  if(ranges.size() == 0)
    {
      //push back a range of 0 and 0 and return                                                                                                                              
      result.push_back(std::make_pair(0.,0.));
      return;
    }

  //the ranges vector contains overlapping ranges
  //this loop removes overlaps
  //output are pairs stored in the result vector
  std::vector<std::pair<float, float> >::iterator it = ranges.begin();
  std::pair<float, float> current = *(it)++;
  for( ; it != ranges.end(); it++) {
    if (current.second >= it->first){
      current.second = std::max(current.second, it->second); 
    }
    else {
      result.push_back(current);
      current = *(it);
    }
  }
  result.push_back(current);
  //G4cout<<result.size()<<"non-overlapping windows found in which to add dark noise."<<G4endl;

  //now we should have a vector of non-overlapping range pairs to pass to the
  //dark noise routine
}

void WCSimWCAddDarkNoise::SaveOptionsToOutput(WCSimRootOptions * wcopt)
{
  wcopt->SetPMTDarkRate(PMTDarkRate);
  wcopt->SetConvRate(ConvRate);
  wcopt->SetDarkHigh(DarkHigh);
  wcopt->SetDarkLow(DarkLow);
  wcopt->SetDarkWindow(DarkWindow);
  wcopt->SetDarkMode(DarkMode);
}
