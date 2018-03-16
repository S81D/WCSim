//  -*- mode:c++; tab-width:4;  -*-
#include "WCSimDetectorConstruction.hh"

#include "G4Material.hh"
#include "G4Element.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4Torus.hh"
#include "G4Sphere.hh"
#include "G4Trd.hh"
#include "G4IntersectionSolid.hh"
#include "G4Polyhedra.hh"
#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4PVReplica.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"
#include "G4AssemblyVolume.hh"
#include "G4SubtractionSolid.hh"
#include "globals.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalSurface.hh"
#include "G4UserLimits.hh"
#include "G4ReflectionFactory.hh"
#include "G4GeometryTolerance.hh"
#include "G4GeometryManager.hh"

#include "WCSimTuningParameters.hh" //jl145

#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"


/***********************************************************
 *
 * This file containts the functions which construct a 
 * cylindrical WC detector.  It used by both the SK and 
 * LBNE WC detector modes.  It is called in the Construct()
 * method in WCSimDetectorConstruction.cc.
 *
 * Sourcefile for the WCSimDetectorConstruction class
 *
 ***********************************************************/


G4LogicalVolume* WCSimDetectorConstruction::ConstructCylinder()
{
    G4cout << "**** Building Cylindrical Detector ****" << G4endl;

  //-----------------------------------------------------
  // Positions
  //-----------------------------------------------------

  debugMode = false;
  
  WCPosition=0.;//Set the WC tube offset to zero

  WCIDRadius = WCIDDiameter/2.;
  // the number of regular cell in phi direction:
  WCBarrelRingNPhi     = (G4int)(WCBarrelNumPMTHorizontal/WCPMTperCellHorizontal); 
  // the part of one ring, that is covered by the regular cells: 
  totalAngle  = 2.0*pi*rad*(WCBarrelRingNPhi*WCPMTperCellHorizontal/WCBarrelNumPMTHorizontal) ;
  // angle per regular cell:
  dPhi        =  totalAngle/ WCBarrelRingNPhi;
  // it's hight:
  if(WCDetectorName!="ANNIEp2v2"){
  barrelCellHeight  = (WCIDHeight-2.*WCBarrelPMTOffset)/WCBarrelNRings;
  mainAnnulusHeight = WCIDHeight -2.*WCBarrelPMTOffset -2.*barrelCellHeight;
  } else {
  barrelCellHeight  = (WCIDHeight-2.*WCBarrelPMTOffset)*((WCBarrelNRings-2)/WCBarrelNRings);
  G4double borderbarrelCellHeight = (WCIDHeight-2.*WCBarrelPMTOffset)/WCBarrelNRings;
  mainAnnulusHeight = WCIDHeight -2.*WCBarrelPMTOffset -2.*borderbarrelCellHeight;
  }
  // the hight of all regular cells together:
  G4cout<<"WCBarrelRingNPhi= "<<WCBarrelRingNPhi<<" totalAngle= "<<totalAngle<<" dPhi= "<<dPhi<<" barrelCellHeight= "<<barrelCellHeight<<" mainAnnulusHeight= "<<mainAnnulusHeight<<G4endl;
  
  
  innerAnnulusRadius = WCIDRadius - WCPMTExposeHeight-1.*mm;
  outerAnnulusRadius = WCIDRadius + WCBlackSheetThickness + 1.*mm;//+ Stealstructure etc.
  // the radii are measured to the center of the surfaces
  // (tangent distance). Thus distances between the corner and the center are bigger.
  
  if(!isANNIE){
    WCLength    = WCIDHeight + 2*2.3*m;	//jl145 - reflects top veto blueprint, cf. Farshid Feyzi
    WCRadius    = (WCIDDiameter/2. + WCBlackSheetThickness + 1.5*m)/cos(dPhi/2.) ; // TODO: OD 
    
  // now we know the extend of the detector and are able to tune the tolerance
  G4GeometryManager::GetInstance()->SetWorldMaximumExtent(WCLength > WCRadius ? WCLength : WCRadius);
  G4cout << "Computed tolerance = "
         << G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/mm
         << " mm" << G4endl;
  } else {
    // modify for ANNIE - much more compact, smaller excess <<< this accounts for extra volume around PMT faces!
    WCLength    = WCIDHeight + 5.*cm;	// arbitrary additional space?
    //WCRadius    = (WCIDDiameter/2. + WCBlackSheetThickness + 20*cm)/cos(dPhi/2.); 
    // WCRadius is known - set in DetectorConfigs
    //G4cout<<"innerAnnulusRadius= "<<innerAnnulusRadius<<" outerAnnulusRadius= "<<outerAnnulusRadius<<G4endl;
  }
  
  //Decide if adding Gd
  water = "Water";
  if (WCAddGd)
  {water = "Doped Water";}


  //-----------------------------------------------------
  // Volumes
  //-----------------------------------------------------

  // The water barrel is placed in an tubs of air
  
  G4double WChalfheightexcess, WCradialexcess;
  //jl145 - per blueprint for SK, less for ANNIE
  (!isANNIE) ? WChalfheightexcess=4.2*m : WChalfheightexcess=4.7625*mm; 
  (!isANNIE) ? WCradialexcess=2.0*m : WCradialexcess=4.7625*mm;    // for ANNIE this is used as the tank steel: 4.7625mm A36 steel (needs to be small as MRD/FACC are very close also)
  
  G4Tubs* solidWC = new G4Tubs("WC",
			       0.0*m,
			       WCRadius+WCradialexcess, 
			       .5*WCLength+WChalfheightexcess,
			       0.*deg,
			       360.*deg);
  
  // No steel container for the water? Add one for ANNIE
  G4LogicalVolume* logicWC;
  if(!isANNIE){
	logicWC = new G4LogicalVolume(solidWC,
			G4Material::GetMaterial("Air"),
			"WC",
			0,0,0);
  } else {
	logicWC = new G4LogicalVolume(solidWC,
			G4Material::GetMaterial("StainlessSteel"),
			"WC",
			0,0,0);
  }
 
   G4VisAttributes* showColor = new G4VisAttributes(G4Colour(0.0,1.0,0.0));
   logicWC->SetVisAttributes(showColor);

   //logicWC->SetVisAttributes(G4VisAttributes::Invisible); //amb79
  
  //-----------------------------------------------------
  // everything else is contained in this water tubs
  //-----------------------------------------------------
  G4double WCradiusexcess;
  (!isANNIE) ? WCradiusexcess=1.*m : WCradiusexcess=0.*m;
  
  G4Tubs* solidWCBarrel = new G4Tubs("WCBarrel",
				     0.0*m,
				     WCRadius+WCradiusexcess, // add a bit of extra space
				     .5*WCLength,  //jl145 - per blueprint
				     0.*deg,
				     360.*deg);
  
  G4LogicalVolume* logicWCBarrel = 
    new G4LogicalVolume(solidWCBarrel,
			G4Material::GetMaterial(water),
			"WCBarrel",
			0,0,0);

    G4VPhysicalVolume* physiWCBarrel = 
    new G4PVPlacement(0,
		      G4ThreeVector(0.,0.,0.),
		      logicWCBarrel,
		      "WCBarrel",
		      logicWC,
		      false,
	 	      0); 

// This volume needs to made invisible to view the blacksheet and PMTs with RayTracer
  if (Vis_Choice == "RayTracer")
   {logicWCBarrel->SetVisAttributes(G4VisAttributes::Invisible);} 

  else
   {//{if(!debugMode)
    //logicWCBarrel->SetVisAttributes(G4VisAttributes::Invisible);} 
   }
  //-----------------------------------------------------
  // Form annular section of barrel to hold PMTs 
  //----------------------------------------------------

 
  G4double mainAnnulusZ[2] = {-mainAnnulusHeight/2., mainAnnulusHeight/2};
  G4double mainAnnulusRmin[2] = {innerAnnulusRadius, innerAnnulusRadius};
  G4double mainAnnulusRmax[2] = {outerAnnulusRadius, outerAnnulusRadius};

  G4Polyhedra* solidWCBarrelAnnulus = new G4Polyhedra("WCBarrelAnnulus",
                                                   0.*deg, // phi start
                                                   totalAngle, 
                                                   (G4int)WCBarrelRingNPhi, //NPhi-gon
                                                   2,
                                                   mainAnnulusZ,
                                                   mainAnnulusRmin,
                                                   mainAnnulusRmax);
  
  G4LogicalVolume* logicWCBarrelAnnulus = 
    new G4LogicalVolume(solidWCBarrelAnnulus,
			G4Material::GetMaterial(water),
			"WCBarrelAnnulus",
			0,0,0);
  // G4cout << *solidWCBarrelAnnulus << G4endl; 
  G4VPhysicalVolume* physiWCBarrelAnnulus = 
    new G4PVPlacement(0,
		      G4ThreeVector(0.,0.,0.),
		      logicWCBarrelAnnulus,
		      "WCBarrelAnnulus",
		      logicWCBarrel,
		      false,
		      0,true);
if(!debugMode)
   logicWCBarrelAnnulus->SetVisAttributes(G4VisAttributes::Invisible); //amb79
  //-----------------------------------------------------
  // Subdivide the BarrelAnnulus into rings
  //-----------------------------------------------------
  G4double RingZ[2] = {-barrelCellHeight/2.,
                        barrelCellHeight/2.};

  G4Polyhedra* solidWCBarrelRing = new G4Polyhedra("WCBarrelRing",
                                                   0.*deg,//+dPhi/2., // phi start
                                                   totalAngle, //phi end
                                                   (G4int)WCBarrelRingNPhi, //NPhi-gon
                                                   2,
                                                   RingZ,
                                                   mainAnnulusRmin,
                                                   mainAnnulusRmax);

  G4LogicalVolume* logicWCBarrelRing = 
    new G4LogicalVolume(solidWCBarrelRing,
			G4Material::GetMaterial(water),
			"WCBarrelRing",
			0,0,0);

  if(WCDetectorName!="ANNIEp2v2"){
  G4VPhysicalVolume* physiWCBarrelRing = 
    new G4PVReplica("WCBarrelRing",
		    logicWCBarrelRing,
		    logicWCBarrelAnnulus,
		    kZAxis,
		    (G4int)WCBarrelNRings-2,
		    barrelCellHeight);
  } else {
  G4VPhysicalVolume* physiWCBarrelRing = 
    new G4PVReplica("WCBarrelRing",
		    logicWCBarrelRing,
		    logicWCBarrelAnnulus,
		    kZAxis,
		    (G4int)1,    // just one giant central cell for ANNIEp2v2
		    barrelCellHeight);
  }

if(!debugMode)
  {G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(0,0.5,1.));
  tmpVisAtt->SetForceWireframe(true);// This line is used to give definition to the rings in OGLSX Visualizer
  logicWCBarrelRing->SetVisAttributes(tmpVisAtt);
  //If you want the rings on the Annulus to show in OGLSX, then comment out the line below.
  logicWCBarrelRing->SetVisAttributes(G4VisAttributes::Invisible);
  }
else {
        G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(0,0.5,1.));
        tmpVisAtt->SetForceWireframe(true);
        logicWCBarrelRing->SetVisAttributes(tmpVisAtt);
  }

  //-----------------------------------------------------
  // Subdivisions of the BarrelRings are cells
  //------------------------------------------------------


  G4Polyhedra* solidWCBarrelCell = new G4Polyhedra("WCBarrelCell",
                                                   -dPhi/2.+0.*deg, // phi start
                                                   dPhi, //total Phi
                                                   1, //NPhi-gon
                                                   2,
                                                   RingZ,
                                                   mainAnnulusRmin,
                                                   mainAnnulusRmax); 
  //G4cout << *solidWCBarrelCell << G4endl; 
  G4LogicalVolume* logicWCBarrelCell = 
    new G4LogicalVolume(solidWCBarrelCell,
			G4Material::GetMaterial(water),
			"WCBarrelCell",
			0,0,0);

  G4VPhysicalVolume* physiWCBarrelCell = 
    new G4PVReplica("WCBarrelCell",
		    logicWCBarrelCell,
		    logicWCBarrelRing,
		    kPhi,
		    (G4int)WCBarrelRingNPhi,
		    dPhi,
                    0.); 

  if(!debugMode)
  	{G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(1.,0.5,0.5));
  	tmpVisAtt->SetForceWireframe(true);// This line is used to give definition to the cells in OGLSX Visualizer
  	logicWCBarrelCell->SetVisAttributes(tmpVisAtt); 
	//If you want the columns on the Annulus to show in OGLSX, then comment out the line below.
  	logicWCBarrelCell->SetVisAttributes(G4VisAttributes::Invisible);
	}
  else {
  	G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(1.,0.5,0.5));
  	tmpVisAtt->SetForceWireframe(true);
  	logicWCBarrelCell->SetVisAttributes(tmpVisAtt);
  }

  //-----------------------------------------------------------
  // The Blacksheet, a daughter of the cells containing PMTs,
  // and also some other volumes to make the edges light tight
  //-----------------------------------------------------------

  //-------------------------------------------------------------
  // add barrel blacksheet to the normal barrel cells 
  // ------------------------------------------------------------
  G4double annulusBlackSheetRmax[2] = {(WCIDRadius+WCBlackSheetThickness),
                                        WCIDRadius+WCBlackSheetThickness};
  G4double annulusBlackSheetRmin[2] = {(WCIDRadius),
                                        WCIDRadius};

  G4Polyhedra* solidWCBarrelCellBlackSheet = new G4Polyhedra("WCBarrelCellBlackSheet",
                                                   -dPhi/2., // phi start
                                                   dPhi, //total phi
                                                   1, //NPhi-gon
                                                   2,
                                                   RingZ,
                                                   annulusBlackSheetRmin,
                                                   annulusBlackSheetRmax);

  logicWCBarrelCellBlackSheet =
    new G4LogicalVolume(solidWCBarrelCellBlackSheet,
                        G4Material::GetMaterial("Blacksheet"),
                        "WCBarrelCellBlackSheet",
                          0,0,0);

   G4VPhysicalVolume* physiWCBarrelCellBlackSheet =
    new G4PVPlacement(0,
                      G4ThreeVector(0.,0.,0.),
                      logicWCBarrelCellBlackSheet,
                      "WCBarrelCellBlackSheet",
                      logicWCBarrelCell,
                      false,
                      0,true);

  G4LogicalBorderSurface * WaterBSBarrelCellSurface 
    = new G4LogicalBorderSurface("WaterBSBarrelCellSurface",
                                 physiWCBarrelCell,
                                 physiWCBarrelCellBlackSheet, 
                                 OpWaterBSSurface);

 // Change made here to have the if statement contain the !debugmode to be consistent
 // This code gives the Blacksheet its color. 

if (Vis_Choice == "RayTracer"){

   G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
      = new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color
     WCBarrelBlackSheetCellVisAtt->SetForceSolid(true); // force the object to be visualized with a surface
	 WCBarrelBlackSheetCellVisAtt->SetForceAuxEdgeVisible(true); // force auxiliary edges to be shown
      if(!debugMode)
        logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
      else
		logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);}

else {

   G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
      = new G4VisAttributes(G4Colour(0.2,0.9,0.2));
      if(!debugMode)
        logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
      else
        logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);}

 //-----------------------------------------------------------
 // add extra tower if nessecary
 // ---------------------------------------------------------
 
  // we have to declare the logical Volumes 
  // outside of the if block to access it later on 
  G4LogicalVolume* logicWCExtraTowerCell;
  G4LogicalVolume* logicWCExtraBorderCell;
  if(!(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal)){

    // as the angles between the corners of the main annulus 
    // and the corners extra tower are different, we need to adjust the 
    // tangent distance the surfaces of the extra tower. Otherwise
    // the corners of the main annulus and the extra tower would 
    // not match. 
    G4double extraTowerRmin[2];
    G4double extraTowerRmax[2];
    for(int i = 0; i < 2 ; i++){
      extraTowerRmin[i] = mainAnnulusRmin[i] != 0 ? mainAnnulusRmin[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
      extraTowerRmax[i] = mainAnnulusRmax[i] != 0 ? mainAnnulusRmax[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
    }
    G4Polyhedra* solidWCExtraTower = new G4Polyhedra("WCextraTower",
  			 totalAngle-2.*pi,//+dPhi/2., // phi start
			 2.*pi -  totalAngle // total angle.
			 -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m),
			   // we need this little Gap between the extra tower and the main annulus
			   // to avoid a shared surface. Without the gap the photons stuck
			   // at this place for mare than 25 steps and the howl simulation
			   // crashes.
	         	 1, //NPhi-gon
		         2,
			 mainAnnulusZ,
			 extraTowerRmin,
		         extraTowerRmax);

    G4LogicalVolume* logicWCExtraTower = 
      new G4LogicalVolume(solidWCExtraTower,
			  G4Material::GetMaterial(water),
			  "WCExtraTower",
			  0,0,0);
    G4VPhysicalVolume* physiWCExtraTower = 
      new G4PVPlacement(0,
			G4ThreeVector(0.,0.,0.),
			logicWCExtraTower,
			"WCExtraTower",
			logicWCBarrel,
			false,
			0,true);
 

    logicWCExtraTower->SetVisAttributes(G4VisAttributes::Invisible);
  //-------------------------------------------
  // subdivide the extra tower into cells  
  //------------------------------------------

    G4Polyhedra* solidWCExtraTowerCell = new G4Polyhedra("WCExtraTowerCell",
			   totalAngle-2.*pi,//+dPhi/2., // phi start
			   2.*pi -  totalAngle -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m), //phi end
			   1, //NPhi-gon
			   2,
			   RingZ,
			   extraTowerRmin,
			   extraTowerRmax); 
    //G4cout << * solidWCExtraTowerCell << G4endl;
    logicWCExtraTowerCell = 
      new G4LogicalVolume(solidWCExtraTowerCell,
			  G4Material::GetMaterial(water),
			  "WCExtraTowerCell",
			  0,0,0);
    G4VPhysicalVolume* physiWCTowerCell = 
      new G4PVReplica("extraTowerCell",
		      logicWCExtraTowerCell,
		      logicWCExtraTower,
		      kZAxis,
		      (G4int)WCBarrelNRings-2,
		      barrelCellHeight);
    logicWCExtraTowerCell->SetVisAttributes(G4VisAttributes::Invisible);
    
    //---------------------------------------------
    // add blacksheet to this cells
    //--------------------------------------------

    G4double towerBSRmin[2];
    G4double towerBSRmax[2];
    for(int i = 0; i < 2; i++){
      towerBSRmin[i] = annulusBlackSheetRmin[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.);
      towerBSRmax[i] = annulusBlackSheetRmax[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.);
    }
    G4Polyhedra* solidWCTowerBlackSheet = new G4Polyhedra("WCExtraTowerBlackSheet",
			   totalAngle-2.*pi,//+dPhi/2., // phi start
			   2.*pi -  totalAngle -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m), //phi end
		           1, //NPhi-gon
			   2,
			   RingZ,
			   towerBSRmin,
			   towerBSRmax);
    //G4cout << * solidWCTowerBlackSheet << G4endl;
    logicWCTowerBlackSheet =
      new G4LogicalVolume(solidWCTowerBlackSheet,
			  G4Material::GetMaterial("Blacksheet"),
			  "WCExtraTowerBlackSheet",
			    0,0,0);

     G4VPhysicalVolume* physiWCTowerBlackSheet =
      new G4PVPlacement(0,
			G4ThreeVector(0.,0.,0.),
			logicWCTowerBlackSheet,
			"WCExtraTowerBlackSheet",
			logicWCExtraTowerCell,
			false,
			0,true);

    G4LogicalBorderSurface * WaterBSTowerCellSurface 
      = new G4LogicalBorderSurface("WaterBSBarrelCellSurface",
				   physiWCTowerCell,
				   physiWCTowerBlackSheet, 
				   OpWaterBSSurface);

// These lines add color to the blacksheet in the extratower. If using RayTracer, comment the first chunk and use the second. The Blacksheet should be green.

  if (Vis_Choice == "OGLSX"){

   G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
      = new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color

	if(!debugMode)
	  {logicWCTowerBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);}
	else
	  {logicWCTowerBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);}}
  
  if (Vis_Choice == "RayTracer"){
   
    G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
      = new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color
     WCBarrelBlackSheetCellVisAtt->SetForceSolid(true); // force the object to be visualized with a surface
	 WCBarrelBlackSheetCellVisAtt->SetForceAuxEdgeVisible(true); // force auxiliary edges to be shown

	if(!debugMode)
	  logicWCTowerBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
	else
	  logicWCTowerBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);}

  else {

   G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
      = new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color

	if(!debugMode)
	  {logicWCTowerBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);}
	else
	  {logicWCTowerBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);}}
  

}

  //jl145------------------------------------------------
  // Add top veto volume
  //-----------------------------------------------------

  G4bool WCTopVeto = (WCSimTuningParams->GetTopVeto());

  G4LogicalVolume* logicWCTopVeto;

  if(WCTopVeto){

	  G4double WCTyvekThickness = 1.0*mm; //completely made up

	  G4VSolid* solidWCTopVeto;
	  solidWCTopVeto =
			new G4Tubs(			"WCTopVeto",
								0.0*m,
								WCIDRadius + WCTyvekThickness,
								0.5*m + WCTyvekThickness,
								0.*deg,
								360.*deg);

	  logicWCTopVeto = 
			new G4LogicalVolume(solidWCTopVeto,
								G4Material::GetMaterial(water),
								"WCTopVeto",
								0,0,0);

	  G4VPhysicalVolume* physiWCTopVeto =
			new G4PVPlacement(	0,
								G4ThreeVector(0.,0.,WCIDHeight/2
													+1.0*m),
								logicWCTopVeto,
								"WCTopVeto",
								logicWCBarrel,
								false,0,true);

	  //Add the top veto Tyvek
	  //-----------------------------------------------------

	  G4VSolid* solidWCTVTyvek;
	  solidWCTVTyvek =
			new G4Tubs(			"WCTVTyvek",
								0.0*m,
								WCIDRadius,
								WCTyvekThickness/2,
								0.*deg,
								360.*deg);


	  G4LogicalVolume* logicWCTVTyvek =
			new G4LogicalVolume(solidWCTVTyvek,
								G4Material::GetMaterial("Tyvek"),
								"WCTVTyvek",
								0,0,0);

	  //Bottom
	  G4VPhysicalVolume* physiWCTVTyvekBot =
			new G4PVPlacement(	0,
		                  		G4ThreeVector(0.,0.,-0.5*m
													-WCTyvekThickness/2),
								logicWCTVTyvek,
		               			"WCTVTyvekBot",
		          				logicWCTopVeto,
				 				false,0,true);

	  G4LogicalBorderSurface * WaterTyTVSurfaceBot =
			new G4LogicalBorderSurface(	"WaterTyTVSurfaceBot",
										physiWCTopVeto,
										physiWCTVTyvekBot,
										OpWaterTySurface);

	  //Top
	  G4VPhysicalVolume* physiWCTVTyvekTop =
			new G4PVPlacement(	0,
		                  		G4ThreeVector(0.,0.,0.5*m
													+WCTyvekThickness/2),
								logicWCTVTyvek,
		               			"WCTVTyvekTop",
		          				logicWCTopVeto,
				 				false,0,true);

	  G4LogicalBorderSurface * WaterTyTVSurfaceTop =
			new G4LogicalBorderSurface(	"WaterTyTVSurfaceTop",
										physiWCTopVeto,
										physiWCTVTyvekTop,
										OpWaterTySurface);

	  //Side
	  G4VSolid* solidWCTVTyvekSide;
	  solidWCTVTyvekSide =
			new G4Tubs(			"WCTVTyvekSide",
								WCIDRadius,
								WCIDRadius + WCTyvekThickness,
								0.5*m + WCTyvekThickness,
								0.*deg,
								360.*deg);


	  G4LogicalVolume* logicWCTVTyvekSide =
			new G4LogicalVolume(solidWCTVTyvekSide,
								G4Material::GetMaterial("Tyvek"),
								"WCTVTyvekSide",
								0,0,0);

	  G4VPhysicalVolume* physiWCTVTyvekSide =
			new G4PVPlacement(	0,
		                  		G4ThreeVector(0.,0.,0.),
								logicWCTVTyvekSide,
		               			"WCTVTyvekSide",
		          				logicWCTopVeto,
				 				false,0,true);

	  G4LogicalBorderSurface * WaterTyTVSurfaceSide =
			new G4LogicalBorderSurface(	"WaterTyTVSurfaceSide",
										physiWCTopVeto,
										physiWCTVTyvekSide,
										OpWaterTySurface);

  }

  //
  //
  //jl145------------------------------------------------


  //////////// M Fechner : I need to  declare the PMT  here in order to
  // place the PMTs in the truncated cells
  //-----------------------------------------------------
  // The PMT
  //-----------------------------------------------------
  
  ////////////J Felde: The PMT logical volume is now constructed separately 
  // from any specific detector geometry so that any geometry can use the same definition. 
  // K.Zbiri: The PMT volume and the PMT glass are now put in parallel. 
  // The PMT glass is the sensitive volume in this new configuration.

  G4LogicalVolume* logicWCPMT; // = ConstructPMT(WCPMTName, WCIDCollectionName, "tank"); // ⚠
  std::vector<G4LogicalVolume*> logicWCPMTs;
  if(WCIDCollectionName!="WCIDCollectionNameIsUnused"){
    WCTankCollectionNames.push_back(WCIDCollectionName);
    WCPMTNameMap.emplace(std::make_pair(WCIDCollectionName, WCPMTName));
  }
  for(auto atankcollection : WCTankCollectionNames){
    G4String thepmtname = WCPMTNameMap.at(atankcollection);
    logicWCPMT= ConstructPMT(thepmtname, atankcollection, "tank");
    logicWCPMTs.push_back(logicWCPMT);
  }
  G4LogicalVolume* logicWCLAPPD=0;
  if(isANNIE) logicWCLAPPD = ConstructLAPPD(WCLAPPDName, WCIDCollectionName2);//WCIDCollectionName2);

  /*These lines of code will give color and volume to the PMTs if it hasn't been set in WCSimConstructPMT.cc.
I recommend setting them in WCSimConstructPMT.cc. 
If used here, uncomment the SetVisAttributes(WClogic) line, and comment out the SetVisAttributes(G4VisAttributes::Invisible) line.*/
  
  G4VisAttributes* WClogic 
      = new G4VisAttributes(G4Colour(0.4,0.0,0.8));
     WClogic->SetForceSolid(true);
	 WClogic->SetForceAuxEdgeVisible(true);

    //logicWCPMT->SetVisAttributes(WClogic);
	for(auto alogicWCPMT : logicWCPMTs) alogicWCPMT->SetVisAttributes(G4VisAttributes::Invisible);
	if(logicWCLAPPD) logicWCLAPPD->SetVisAttributes(G4VisAttributes::Invisible);

  //jl145------------------------------------------------
  // Add top veto PMTs
  //-----------------------------------------------------

  if(WCTopVeto){

	  G4double WCTVPMTSpacing = (WCSimTuningParams->GetTVSpacing())*cm;
	  G4double WCTVEdgeLimit = WCCapEdgeLimit;
	  G4int TVNCell = WCTVEdgeLimit/WCTVPMTSpacing + 2;

	  int icopy = 0;

	  for ( int i = -TVNCell ; i <  TVNCell; i++) {
		for (int j = -TVNCell ; j <  TVNCell; j++)   {

		  G4double xoffset = i*WCTVPMTSpacing + WCTVPMTSpacing*0.5;
		  G4double yoffset = j*WCTVPMTSpacing + WCTVPMTSpacing*0.5;

		  G4ThreeVector cellpos =
		  		G4ThreeVector(	xoffset, yoffset, -0.5*m);

		  if ((sqrt(xoffset*xoffset + yoffset*yoffset) + WCPMTRadius) < WCTVEdgeLimit) {

		    G4VPhysicalVolume* physiCapPMT =
		    		new G4PVPlacement(	0,						// no rotation
		    							cellpos,				// its position
		    							logicWCPMT,				// its logical volume
		    							"WCPMT",				// its name 
		    							logicWCTopVeto,			// its mother volume
		    							false,					// no boolean os
		    							icopy);					// every PMT need a unique id.

		    icopy++;
		  }
		}
	  }

	  G4double WCTVEfficiency = icopy*WCPMTRadius*WCPMTRadius/((WCIDRadius)*(WCIDRadius));
	  G4cout << "Total on top veto: " << icopy << "\n";
	  G4cout << "PMT Coverage was calculated to be: " << WCTVEfficiency << "\n";

  }

  //
  //
  //jl145------------------------------------------------


    ///////////////   Barrel PMT placement
  G4RotationMatrix* WCPMTRotation = new G4RotationMatrix;
  WCPMTRotation->rotateY(90.*deg);

  G4double compressionfactor;
  if(WCDetectorName=="ANNIEp2") compressionfactor=0.9;
  else compressionfactor=1.0;
  // squeeze PMTs closer to prevent geometry overlap in ANNIE
  G4double barrelCellWidth = 2.*WCIDRadius*tan(dPhi/2.)*compressionfactor;
  G4double horizontalSpacing   = barrelCellWidth/WCPMTperCellHorizontal;
  G4double verticalSpacing     = barrelCellHeight/WCPMTperCellVertical;

  for(G4double i = 0; i < WCPMTperCellHorizontal; i++){
    for(G4double j = 0; j < WCPMTperCellVertical; j++){
    G4ThreeVector PMTPosition;
      if(WCDetectorName=="ANNIEp2"){
        PMTPosition =  G4ThreeVector(WCIDRadius,
                                                  -barrelCellWidth/2.+(i+0.5)*horizontalSpacing,
                                                  -barrelCellHeight/2.+(j+0.5)*verticalSpacing);
      } else if(WCDetectorName=="ANNIEp2v2"){
        // for ANNIEp2v2 but j is actually ring number. need to modify horizontal position
        // and number of placements based on which ring we're in
        
        if((((int)j%2)!=0)&&(i==1)) continue;   // odd rings have 1 PMT / cell, skip second placement
        
        // which PMT logical vol, based on ring
        if(j==0||j==4){ // << 2 outermost 16-pmt barrel rings are 8"
          logicWCPMT = logicWCPMTs.at(2); // 8" HQE new
        } else {        // all others are 10"
          logicWCPMT = logicWCPMTs.at(0); // 10" LUX/Watchboy
        }
        
//4 rings of 8 pmts = row 0, 2, 4, 6
//  rows 0-> PMT logical volume = 8"              (8x   8" pmts)
//  rows 2,4,6-> PMT logical volume = 10"         (24x 10" pmts)
//3 rings of 16 pmts = row 1, 3, 5.
//  row 3-> PMT logical volume = 10"              (16x 10" pmts)
//  row 1, 5 -> PMT logical volume = 8"           (32x  8" pmts)
//                                         TOTAL: (40x  8" pmts) + 40x  10" pmts) 
// PMTs are placed in the vector in the order their collections are defined - i.e. in the order they are 
// declared in DetectorConfigs: 
// {R7081 (10" WB/LUX : barrel+bottom), D784KFLB (11" LBNE : top), R5912HQE (8" HQE : barrel)}
        
        // position: vertically it's as normal for a cell
        double verticalposition = -barrelCellHeight/2.+(j+0.5)*verticalSpacing;
        // horizontally it depends on which ring
        double horizontalposition;
        if((int)j%2!=0) horizontalposition = 0;  // 1 pmt per cell horizontally; place centrally within cell
        else horizontalposition = pow(-1,i)*(barrelCellWidth/4.); // 2 evenly within cell
        
        PMTPosition = G4ThreeVector(WCIDRadius, horizontalposition, verticalposition);  // centre of cell
      
      } else {
        PMTPosition =  G4ThreeVector(WCIDRadius,   // configuration 2 rows of 2 at cell edges
                                         -barrelCellWidth/2.+ (((i>1)*2)+0.5)*horizontalSpacing,
                                         -barrelCellHeight/2.+(((((int)i)%2)*2)+1)*(verticalSpacing/4.));
      }

  //G4cout<<"PMT position: "<<WCIDRadius<<","<<-barrelCellWidth/2.+(i+0.5)*horizontalSpacing<<","<<-barrelCellHeight/2.+(j+0.5)*verticalSpacing<<G4endl;

      G4VPhysicalVolume* physiWCBarrelPMT =
	new G4PVPlacement(WCPMTRotation,              // its rotation
			  PMTPosition, 
			  logicWCPMT,                // its logical volume
			  "WCPMT",             // its name
			  logicWCBarrelCell,         // its mother volume
			  false,                     // no boolean operations
			  (int)(i*WCPMTperCellVertical+j),
			  true);                       
      
   // logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
     // daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
		// this is still the case.
    }
  }

  //////----------- Barrel LAPPD placement --------------
  G4RotationMatrix* WCLAPPDRotation = new G4RotationMatrix;
  WCLAPPDRotation->rotateY(90.*deg);
  //when I define it with the pmt parameters it works fine...Why????
  G4double barrelCellWidth2 = 2.*WCIDRadius*tan(dPhi/2.)*0.9;
  //G4double horizontalSpacingLAPPD   = barrelCellWidth/WCPMTperCellHorizontal;
  //G4double verticalSpacingLAPPD     = barrelCellHeight/WCPMTperCellVertical;
  double horizontalSpacingLAPPD = ((WCDetectorName=="ANNIEp2") ? barrelCellWidth/WCLAPPDperCellHorizontal : 0);
  double verticalSpacingLAPPD   = ((WCDetectorName=="ANNIEp2") ? barrelCellHeight/WCLAPPDperCellVertical : 0);

  // G4cout<<"barrelCellWidth2= "<<barrelCellWidth2<<" barrelCellHeight2= "<<barrelCellHeight2<< " horizontalSpacingLAPPD= "<<horizontalSpacingLAPPD<<" verticalSpacingLAPPD= "<<verticalSpacingLAPPD<<G4endl;

  for(G4double i = 0; i < WCLAPPDperCellHorizontal; i++){
    for(G4double j = 0; j < WCLAPPDperCellVertical; j++){
  //for(G4double i = 0; i < WCPMTperCellHorizontal; i++){
  //	for(G4double j = 0; j < WCPMTperCellVertical; j++){
	  
// configuration 4 across at cell bottom
//      G4ThreeVector LAPPDPosition =  G4ThreeVector((WCIDRadius-((outerAnnulusRadius-innerAnnulusRadius)/2.)),
//												   -barrelCellWidth2/2.+(i+0.5)*horizontalSpacingLAPPD,
//												   -barrelCellHeight2/2.+(j+0.5)*verticalSpacingLAPPD
//												   -(verticalSpacingLAPPD/4.));
// configuration 2 rows of 2 at cell centres
//      G4ThreeVector LAPPDPosition =  G4ThreeVector((WCIDRadius-((outerAnnulusRadius-innerAnnulusRadius)/2.)),
//						 -barrelCellWidth2/2.+ ((((int)i)%2)+1.5)*horizontalSpacingLAPPD,
//						 -barrelCellHeight2/2.+(((((int)i)%2)*2)-1)*(verticalSpacingLAPPD/4.));
      G4ThreeVector LAPPDPosition =  G4ThreeVector((WCIDRadius-((outerAnnulusRadius-innerAnnulusRadius)/2.)),
						 -barrelCellWidth2/2.+ (((i>1)*2)+1.5)*horizontalSpacingLAPPD,
						 -barrelCellHeight2/2.+(((((int)i)%2)*2)-1)*(verticalSpacingLAPPD/4.));
	 
      G4VPhysicalVolume* physiWCBarrelLAPPD =
		new G4PVPlacement(WCLAPPDRotation,                      // its rotation
						  LAPPDPosition,                        // its position
						  logicWCLAPPD,                         // its logical volume
						  "WCLAPPD",                            // its name
						  logicWCBarrelCell,                    // its mother volume
						  false,                                // no boolean operations
						  (int)(i*WCLAPPDperCellVertical+j),    // a unique copy number
						  false);                               // don't check overlaps
	  
	  // logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
	  // daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
	  // this is still the case.
    }
  }
  //-------------------------------------------------------------
  // Add PMTs in extra Tower if necessary
  //------------------------------------------------------------


  if(!(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal)){

    G4RotationMatrix* WCPMTRotation = new G4RotationMatrix;
    WCPMTRotation->rotateY(90.*deg);
    WCPMTRotation->rotateX((2*pi-totalAngle)/2.);//align the PMT with the Cell
                                                 
    G4double towerWidth = WCIDRadius*tan(2*pi-totalAngle);

    G4double horizontalSpacing   = towerWidth/(WCBarrelNumPMTHorizontal-WCBarrelRingNPhi*WCPMTperCellHorizontal);
    G4double verticalSpacing     = barrelCellHeight/WCPMTperCellVertical;

    for(G4double i = 0; i < (WCBarrelNumPMTHorizontal-WCBarrelRingNPhi*WCPMTperCellHorizontal); i++){
      for(G4double j = 0; j < WCPMTperCellVertical; j++){
	G4ThreeVector PMTPosition =  G4ThreeVector(WCIDRadius/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.),
				towerWidth/2.-(i+0.5)*horizontalSpacing,
			       -barrelCellHeight/2.+(j+0.5)*verticalSpacing);
	PMTPosition.rotateZ(-(2*pi-totalAngle)/2.); // align with the symmetry 
	                                            //axes of the cell 

	G4VPhysicalVolume* physiWCBarrelPMT =
	  new G4PVPlacement(WCPMTRotation,             // its rotation
			    PMTPosition, 
			    logicWCPMT,                // its logical volume
			    "WCPMT",             // its name
			    logicWCExtraTowerCell,         // its mother volume
			    false,                     // no boolean operations
			    (int)(i*WCPMTperCellVertical+j),
			    true);                       
	
		// logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
		// daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
		// this is still the case.
      }
    }

  }

  //---------------------------- CAPS ----------------------------

  G4LogicalVolume* logicTopCapAssembly = ConstructCaps(-1);
  G4LogicalVolume* logicBottomCapAssembly = ConstructCaps(1);

 // These lines make the large cap volume invisible to view the caps blacksheets. Need to make invisible for 
 // RayTracer
  if (Vis_Choice == "RayTracer"){
	logicBottomCapAssembly->SetVisAttributes(G4VisAttributes::Invisible);
	logicTopCapAssembly->SetVisAttributes(G4VisAttributes::Invisible);}

  G4VPhysicalVolume* physiTopCapAssembly =
  new G4PVPlacement(0,
                  G4ThreeVector(0.,0.,(mainAnnulusHeight/2.+ capAssemblyHeight/2.)),
                  logicTopCapAssembly,
                  "TopCapAssembly",
                  logicWCBarrel,
                  false, 0,true);

  G4VPhysicalVolume* physiBottomCapAssembly =
    new G4PVPlacement(0,
                  G4ThreeVector(0.,0.,(-mainAnnulusHeight/2.- capAssemblyHeight/2.)),
                  logicBottomCapAssembly,
                  "BottomCapAssembly",
                  logicWCBarrel,
                  false, 0,true);

  return logicWC;
  
  
  
}


//------------------------- Construct Caps ---------------------------
G4LogicalVolume* WCSimDetectorConstruction::ConstructCaps(G4int zflip)
{

  capAssemblyHeight = (WCIDHeight-mainAnnulusHeight)/2+1*mm+WCBlackSheetThickness;

  G4Tubs* solidCapAssembly = new G4Tubs("CapAssembly",
							0.0*m,
							outerAnnulusRadius/cos(dPhi/2.), 
							capAssemblyHeight/2,
							0.*deg,
							360.*deg);

  G4LogicalVolume* logicCapAssembly =
    new G4LogicalVolume(solidCapAssembly,
                        G4Material::GetMaterial(water),
                        "CapAssembly",
                        0,0,0);




  //----------------------------------------------------
  // extra rings for the top and bottom of the annulus
  //---------------------------------------------------
  // border ring height should correspond to a single ring
  if(WCDetectorName=="ANNIEp2v2") barrelCellHeight = (WCIDHeight-2.*WCBarrelPMTOffset)/WCBarrelNRings;
  G4double borderAnnulusZ[3] = {-barrelCellHeight/2.*zflip,
                                (-barrelCellHeight/2.+(WCIDRadius-innerAnnulusRadius))*zflip,
				barrelCellHeight/2.*zflip};
  G4double borderAnnulusRmin[3] = { WCIDRadius, innerAnnulusRadius, innerAnnulusRadius};
  G4double borderAnnulusRmax[3] = {outerAnnulusRadius, outerAnnulusRadius,outerAnnulusRadius};
  G4Polyhedra* solidWCBarrelBorderRing = new G4Polyhedra("WCBarrelBorderRing",
                                                   0.*deg, // phi start
                                                   totalAngle,
                                                   (G4int)WCBarrelRingNPhi, //NPhi-gon
                                                   3,
                                                   borderAnnulusZ,
                                                   borderAnnulusRmin,
                                                   borderAnnulusRmax);
  G4LogicalVolume* logicWCBarrelBorderRing =
    new G4LogicalVolume(solidWCBarrelBorderRing,
                        G4Material::GetMaterial(water),
                        "WCBarrelRing",
                        0,0,0);
  //G4cout << *solidWCBarrelBorderRing << G4endl;

  G4VPhysicalVolume* physiWCBarrelBorderRing =
    new G4PVPlacement(0,
                  G4ThreeVector(0.,0.,(capAssemblyHeight/2.- barrelCellHeight/2.)*zflip),
                  logicWCBarrelBorderRing,
                  "WCBarrelBorderRing",
                  logicCapAssembly,
                  false, 0,true);


                  
  if(!debugMode) 
    logicWCBarrelBorderRing->SetVisAttributes(G4VisAttributes::Invisible); 
  //----------------------------------------------------
  // Subdevide border rings into cells
  // --------------------------------------------------
  G4Polyhedra* solidWCBarrelBorderCell = new G4Polyhedra("WCBarrelBorderCell",
                                                   -dPhi/2., // phi start
                                                   dPhi, //total angle 
                                                   1, //NPhi-gon
                                                   3,
                                                   borderAnnulusZ,
                                                   borderAnnulusRmin,
                                                   borderAnnulusRmax);

  G4LogicalVolume* logicWCBarrelBorderCell =
    new G4LogicalVolume(solidWCBarrelBorderCell, 
                        G4Material::GetMaterial(water),
                        "WCBarrelBorderCell", 
                        0,0,0);
  //G4cout << *solidWCBarrelBorderCell << G4endl;
  G4VPhysicalVolume* physiWCBarrelBorderCell =
    new G4PVReplica("WCBarrelBorderCell",
                    logicWCBarrelBorderCell,
                    logicWCBarrelBorderRing,
                    kPhi,
                    (G4int)WCBarrelRingNPhi,
                    dPhi,
                    0.);

// These lines of code below will turn the border rings invisible. 

// used for RayTracer
 if (Vis_Choice == "RayTracer"){

  if(!debugMode){
        G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(1.,0.5,0.5));
        tmpVisAtt->SetForceSolid(true);
        logicWCBarrelBorderCell->SetVisAttributes(tmpVisAtt);
		logicWCBarrelBorderCell->SetVisAttributes(G4VisAttributes::Invisible);}
  else {
        G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(1.,0.5,0.5));
        tmpVisAtt->SetForceWireframe(true);
        logicWCBarrelBorderCell->SetVisAttributes(tmpVisAtt);
		logicWCBarrelBorderCell->SetVisAttributes(G4VisAttributes::Invisible);}}

// used for OGLSX
 else {

  if(!debugMode)
        {logicWCBarrelBorderCell->SetVisAttributes(G4VisAttributes::Invisible);}
  else {
        G4VisAttributes* tmpVisAtt = new G4VisAttributes(G4Colour(1.,0.5,0.5));
        tmpVisAtt->SetForceWireframe(true);
        logicWCBarrelBorderCell->SetVisAttributes(tmpVisAtt);}}


  //------------------------------------------------------------
  // add blacksheet to the border cells.
  // We can use the same logical volume as for the normal
  // barrel cells.
  // ---------------------------------------------------------

  if(WCDetectorName=="ANNIEp2v2"){  // ANNIEp2v2 can't re-use the logical volume, wrong size.
    // The code to generate it is the same, though. Just barrelCellHeight is now smaller.
    G4double borderbarrelCellHeight = (WCIDHeight-2.*WCBarrelPMTOffset)/WCBarrelNRings;
    G4double RingZ[2] = {-borderbarrelCellHeight/2.,
                          borderbarrelCellHeight/2.};
    G4double annulusBlackSheetRmax[2] = {WCIDRadius+WCBlackSheetThickness,
                                         WCIDRadius+WCBlackSheetThickness};
    G4double annulusBlackSheetRmin[2] = {WCIDRadius,
                                         WCIDRadius};
    
    G4Polyhedra* solidWCBarrelCellBlackSheet = new G4Polyhedra("WCBarrelCellBlackSheet",
                                                   -dPhi/2., // phi start
                                                   dPhi, //total phi
                                                   1, //NPhi-gon
                                                   2,
                                                   RingZ,
                                                   annulusBlackSheetRmin,
                                                   annulusBlackSheetRmax);

    logicWCBarrelCellBlackSheet =
      new G4LogicalVolume(solidWCBarrelCellBlackSheet,
                          G4Material::GetMaterial("Blacksheet"),
                          "WCBarrelCellBlackSheet",
                            0,0,0);

   if (Vis_Choice == "RayTracer"){
     G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
        = new G4VisAttributes(G4Colour(0.2,0.9,0.2)); // green color
       WCBarrelBlackSheetCellVisAtt->SetForceSolid(true); // force the object to be visualized with a surface
	   WCBarrelBlackSheetCellVisAtt->SetForceAuxEdgeVisible(true); // force auxiliary edges to be shown
        if(!debugMode)
          logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
        else
		  logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
    } else {
     G4VisAttributes* WCBarrelBlackSheetCellVisAtt 
        = new G4VisAttributes(G4Colour(0.2,0.9,0.2));
        if(!debugMode)
          logicWCBarrelCellBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
        else
          logicWCBarrelCellBlackSheet->SetVisAttributes(WCBarrelBlackSheetCellVisAtt);
    }
  
  }

   G4VPhysicalVolume* physiWCBarrelBorderCellBlackSheet =
    new G4PVPlacement(0,
                      G4ThreeVector(0.,0.,0.),
                      logicWCBarrelCellBlackSheet,
                      "WCBarrelCellBlackSheet",
                      logicWCBarrelBorderCell,
                      false,
                      0,true);

  G4LogicalBorderSurface * WaterBSBarrelBorderCellSurface
    = new G4LogicalBorderSurface("WaterBSBarrelCellSurface",
                                 physiWCBarrelBorderCell,
                                 physiWCBarrelBorderCellBlackSheet,
                                 OpWaterBSSurface);

  // we have to declare the logical Volumes 
  // outside of the if block to access it later on 
  G4LogicalVolume* logicWCExtraTowerCell;
  G4LogicalVolume* logicWCExtraBorderCell;
  if(!(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal)){
    //----------------------------------------------
    // also the extra tower need special cells at the 
    // top and th bottom.
    // (the top cell is created later on by reflecting the
    // bottom cell) 
    //---------------------------------------------
    G4double extraBorderRmin[3];
    G4double extraBorderRmax[3];
    for(int i = 0; i < 3; i++){
      extraBorderRmin[i] = borderAnnulusRmin[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.);
      extraBorderRmax[i] = borderAnnulusRmax[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.);
    } 
    G4Polyhedra* solidWCExtraBorderCell = new G4Polyhedra("WCspecialBarrelBorderCell",
			   totalAngle-2.*pi,//+dPhi/2., // phi start
			   2.*pi -  totalAngle -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m), //total phi
			   1, //NPhi-gon
			   3,
			   borderAnnulusZ,
			   extraBorderRmin,
			   extraBorderRmax);

    logicWCExtraBorderCell =
      new G4LogicalVolume(solidWCExtraBorderCell, 
			  G4Material::GetMaterial(water),
			  "WCspecialBarrelBorderCell", 
			  0,0,0);
    //G4cout << *solidWCExtraBorderCell << G4endl;

    G4VPhysicalVolume* physiWCExtraBorderCell =
      new G4PVPlacement(0,
                  G4ThreeVector(0.,0.,(capAssemblyHeight/2.- barrelCellHeight/2.)*zflip),
                  logicWCExtraBorderCell,
                  "WCExtraTowerBorderCell",
                  logicCapAssembly,
                  false, 0,true);

    logicWCExtraBorderCell->SetVisAttributes(G4VisAttributes::Invisible);

    G4VPhysicalVolume* physiWCExtraBorderBlackSheet =
      new G4PVPlacement(0,
			G4ThreeVector(0.,0.,0.),
			logicWCTowerBlackSheet,
			"WCExtraTowerBlackSheet",
			logicWCExtraBorderCell,
			false,
			0,true);

    G4LogicalBorderSurface * WaterBSExtraBorderCellSurface 
      = new G4LogicalBorderSurface("WaterBSBarrelCellSurface",
				   physiWCExtraBorderCell,
				   physiWCExtraBorderBlackSheet, 
				   OpWaterBSSurface);

  }
 //------------------------------------------------------------
 // add caps
 // -----------------------------------------------------------
 
  G4double capZ[4] = { (-WCBlackSheetThickness-1.*mm)*zflip,
                      WCBarrelPMTOffset*zflip,
		      WCBarrelPMTOffset*zflip,
		      (WCBarrelPMTOffset+(WCIDRadius-innerAnnulusRadius))*zflip} ;
  G4double capRmin[4] = {  0. , 0., 0., 0.} ;
  G4double capRmax[4] = {outerAnnulusRadius, outerAnnulusRadius,  WCIDRadius, innerAnnulusRadius};
  G4VSolid* solidWCCap;
  if(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal){
    solidWCCap
      = new G4Polyhedra("WCCap",
			0.*deg, // phi start
			totalAngle, //phi end
			(int)WCBarrelRingNPhi, //NPhi-gon
			4, // 2 z-planes
			capZ, //position of the Z planes
			capRmin, // min radius at the z planes
			capRmax// max radius at the Z planes
			);
  } else {
  // if there is an extra tower, the cap volume is a union of
  // to polyhedra. We have to unite both parts, because there are 
  // PMTs that are on the border between both parts.
   G4Polyhedra* mainPart 
      = new G4Polyhedra("WCCapMainPart",
			0.*deg, // phi start
			totalAngle, //phi end
			(int)WCBarrelRingNPhi, //NPhi-gon
			4, // 2 z-planes
			capZ, //position of the Z planes
			capRmin, // min radius at the z planes
			capRmax// max radius at the Z planes
			);
    G4double extraCapRmin[4]; 
    G4double extraCapRmax[4]; 
    for(int i = 0; i < 4 ; i++){
      extraCapRmin[i] = capRmin[i] != 0. ?  capRmin[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
      extraCapRmax[i] = capRmax[i] != 0. ? capRmax[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
    }
    G4Polyhedra* extraSlice 
      = new G4Polyhedra("WCCapExtraSlice",
			totalAngle-2.*pi, // phi start
			2.*pi -  totalAngle -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m), //total phi 
			// fortunately there are no PMTs an the gap!
			1, //NPhi-gon
			4, //  z-planes
			capZ, //position of the Z planes
			extraCapRmin, // min radius at the z planes
			extraCapRmax// max radius at the Z planes
			);
     solidWCCap =
        new G4UnionSolid("WCCap", mainPart, extraSlice);

     //G4cout << *solidWCCap << G4endl;
   
  }
  // G4cout << *solidWCCap << G4endl;
  G4LogicalVolume* logicWCCap = 
    new G4LogicalVolume(solidWCCap,
			G4Material::GetMaterial(water),
			"WCCapPolygon",
			0,0,0);

  G4VPhysicalVolume* physiWCCap =
    new G4PVPlacement(0,                           // no rotation
		      G4ThreeVector(0.,0.,(-capAssemblyHeight/2.+1*mm+WCBlackSheetThickness)*zflip),     // its position
		      logicWCCap,          // its logical volume
		      "WCCap",             // its name
		      logicCapAssembly,                  // its mother volume
		      false,                       // no boolean operations
		      0,true);                          // Copy #


// used for RayTracer
 if (Vis_Choice == "RayTracer"){
  if(!debugMode){  
    G4VisAttributes* tmpVisAtt2 = new G4VisAttributes(G4Colour(1,0.5,0.5));
	tmpVisAtt2->SetForceSolid(true);
	logicWCCap->SetVisAttributes(tmpVisAtt2);
    logicWCCap->SetVisAttributes(G4VisAttributes::Invisible);

  } else{
	
	G4VisAttributes* tmpVisAtt2 = new G4VisAttributes(G4Colour(0.6,0.5,0.5));
	tmpVisAtt2->SetForceSolid(true);
    logicWCCap->SetVisAttributes(tmpVisAtt2);}}

// used for OGLSX
 else{
  if(!debugMode){  
    logicWCCap->SetVisAttributes(G4VisAttributes::Invisible);
  } else
	{G4VisAttributes* tmpVisAtt2 = new G4VisAttributes(G4Colour(.6,0.5,0.5));
    tmpVisAtt2->SetForceWireframe(true);
    logicWCCap->SetVisAttributes(tmpVisAtt2);}}

  //---------------------------------------------------------------------
  // add cap blacksheet
  // -------------------------------------------------------------------
  
  G4double capBlackSheetZ[4] = {-WCBlackSheetThickness*zflip, 0., 0., WCBarrelPMTOffset*zflip};
  G4double capBlackSheetRmin[4] = {0., 0., WCIDRadius, WCIDRadius};
  G4double capBlackSheetRmax[4] = {WCIDRadius+WCBlackSheetThickness, 
                                   WCIDRadius+WCBlackSheetThickness,
				   WCIDRadius+WCBlackSheetThickness,
				   WCIDRadius+WCBlackSheetThickness};
  G4VSolid* solidWCCapBlackSheet;
  if(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal){
    solidWCCapBlackSheet
      = new G4Polyhedra("WCCapBlackSheet",
			0.*deg, // phi start
			totalAngle, //total phi
			WCBarrelRingNPhi, //NPhi-gon
			4, //  z-planes
			capBlackSheetZ, //position of the Z planes
			capBlackSheetRmin, // min radius at the z planes
			capBlackSheetRmax// max radius at the Z planes
			);
    // G4cout << *solidWCCapBlackSheet << G4endl;
  } else { 
    // same as for the cap volume
     G4Polyhedra* mainPart
      = new G4Polyhedra("WCCapBlackSheetMainPart",
			0.*deg, // phi start
			totalAngle, //phi end
			WCBarrelRingNPhi, //NPhi-gon
			4, //  z-planes
			capBlackSheetZ, //position of the Z planes
			capBlackSheetRmin, // min radius at the z planes
			capBlackSheetRmax// max radius at the Z planes
			);
     G4double extraBSRmin[4];
     G4double extraBSRmax[4];
     for(int i = 0; i < 4 ; i++){
       extraBSRmin[i] = capBlackSheetRmin[i] != 0. ? capBlackSheetRmin[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
       extraBSRmax[i] = capBlackSheetRmax[i] != 0. ? capBlackSheetRmax[i]/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.) : 0.;
     }
     G4Polyhedra* extraSlice
      = new G4Polyhedra("WCCapBlackSheetextraSlice",
			totalAngle-2.*pi, // phi start
			2.*pi -  totalAngle -G4GeometryTolerance::GetInstance()->GetSurfaceTolerance()/(10.*m), //
			WCBarrelRingNPhi, //NPhi-gon
			4, //  z-planes
			capBlackSheetZ, //position of the Z planes
			extraBSRmin, // min radius at the z planes
			extraBSRmax// max radius at the Z planes
			);
    
     solidWCCapBlackSheet =
        new G4UnionSolid("WCCapBlackSheet", mainPart, extraSlice);
  }
  G4LogicalVolume* logicWCCapBlackSheet =
    new G4LogicalVolume(solidWCCapBlackSheet,
			G4Material::GetMaterial("Blacksheet"),
			"WCCapBlackSheet",
			0,0,0);
  G4VPhysicalVolume* physiWCCapBlackSheet =
    new G4PVPlacement(0,
                      G4ThreeVector(0.,0.,0.),
                      logicWCCapBlackSheet,
                      "WCCapBlackSheet",
                      logicWCCap,
                      false,
                      0,true);
   G4LogicalBorderSurface * WaterBSBottomCapSurface 
      = new G4LogicalBorderSurface("WaterBSCapPolySurface",
                                   physiWCCap,physiWCCapBlackSheet,
                                   OpWaterBSSurface);

   G4VisAttributes* WCCapBlackSheetVisAtt 
      = new G4VisAttributes(G4Colour(0.9,0.2,0.2));
    
// used for OGLSX
 if (Vis_Choice == "OGLSX"){

   G4VisAttributes* WCCapBlackSheetVisAtt 
   = new G4VisAttributes(G4Colour(0.9,0.2,0.2));
 
	if(!debugMode)
        logicWCCapBlackSheet->SetVisAttributes(G4VisAttributes::Invisible);
    else
        logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);}

// used for RayTracer (makes the caps blacksheet yellow)
 if (Vis_Choice == "RayTracer"){

   G4VisAttributes* WCCapBlackSheetVisAtt 
   = new G4VisAttributes(G4Colour(1.0,1.0,0.0));

	if(!debugMode)
       //logicWCCapBlackSheet->SetVisAttributes(G4VisAttributes::Invisible); //Use this line if you want to make the blacksheet on the caps invisible to view through
	   logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);
    else
        logicWCCapBlackSheet->SetVisAttributes(WCCapBlackSheetVisAtt);}

  //---------------------------------------------------------
  // Add top and bottom PMTs
  // -----------------------------------------------------
  G4String thecollectionName;
	if(WCDetectorName=="ANNIEp2v2"){  // hard-coding, needs to tie up with WCDetectorConfigs.
		if(zflip>0){
			thecollectionName = WCTankCollectionNames.at(1);
			WCPMTName = WCPMTNameMap.at(thecollectionName);   // top is D784KFLB
		} else {
			thecollectionName = WCTankCollectionNames.at(0);
			WCPMTName = WCPMTNameMap.at(thecollectionName);  // bottom is R7081
		}
	} else {
		thecollectionName = WCIDCollectionName;
	}
	G4cout<<"ConstructPMT "<<WCPMTName<<" in caps"<<G4endl;
	G4LogicalVolume* logicWCPMT = ConstructPMT(WCPMTName, thecollectionName, "tank");
	G4LogicalVolume* logicWCLAPPD;
	if(isANNIE) logicWCLAPPD = ConstructLAPPD(WCLAPPDName, WCIDCollectionName2);
	
	// If using RayTracer and want to view the detector without caps, comment out the top and bottom PMT's

  G4double xoffset;
  G4double yoffset;
  G4double xoffset2;
  G4double yoffset2;
  G4int    icopy = 0;
  G4int    icopylappd = 0;

  G4RotationMatrix* WCCapPMTRotation = new G4RotationMatrix;
  if(zflip==-1){
    WCCapPMTRotation->rotateY(180.*deg);
  }

  // loop over the cap
  G4int CapNCell = WCCapEdgeLimit/WCCapPMTSpacing + 2;
  for ( int i = -CapNCell ; i <  CapNCell; i++) {
    for (int j = -CapNCell ; j <  CapNCell; j++)   {

       
      xoffset = i*WCCapPMTSpacing + WCCapPMTSpacing*0.5;
      yoffset = j*WCCapPMTSpacing + WCCapPMTSpacing*0.5;

      
      G4ThreeVector cellpos = G4ThreeVector(xoffset, yoffset, 0);     
      //      G4double WCBarrelEffRadius = WCIDDiameter/2. - WCCapPMTSpacing;
      //      double comp = xoffset*xoffset + yoffset*yoffset 
      //	- 2.0 * WCBarrelEffRadius * sqrt(xoffset*xoffset+yoffset*yoffset)
      //	+ WCBarrelEffRadius*WCBarrelEffRadius;
      //      if ( (comp > WCPMTRadius*WCPMTRadius) && ((sqrt(xoffset*xoffset + yoffset*yoffset) + WCPMTRadius) < WCCapEdgeLimit) ) {
     if((WCDetectorName=="ANNIEp2")&&((i%2==0 && j%2==0)||(i%2!=0 && j%2!=0))){
       if (((sqrt(xoffset*xoffset + yoffset*yoffset) + WCLAPPDRadius) < WCCapEdgeLimit + WCLAPPDRadius ) ) {
        //G4cout<<"LAPPD at: "<<i<<" j= "<<j<<" cellpos= "<<cellpos<<G4endl;
        //add LAPPDs in cap:
        G4ThreeVector cellposlappd = G4ThreeVector(xoffset, yoffset, (WCBarrelPMTOffset/2.)*zflip);
        G4VPhysicalVolume* physiCapLAPPD =
            new G4PVPlacement(WCCapPMTRotation,
                              cellposlappd,                   // its position
                              logicWCLAPPD,                   // its logical volume
                              "WCLAPPD",                      // its name 
                              logicWCCap,                     // its mother volume
                              false,                          // no boolean os
                              icopylappd,                     // every LAPPD need a unique copy id
                              false);                         // don't check overlaps
         icopylappd++;
       }
     } else {

	if (((sqrt(xoffset*xoffset + yoffset*yoffset) + WCPMTRadius) < WCCapEdgeLimit) ) {
	G4VPhysicalVolume* physiCapPMT =
	  new G4PVPlacement(WCCapPMTRotation,
			    cellpos,                   // its position
			    logicWCPMT,                // its logical volume
			    "WCPMT",                   // its name 
			    logicWCCap,                // its mother volume
			    false,                     // no boolean os
			    icopy);                    // every PMT need a unique id.
	
 // logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
    // daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
	// this is still the case.

	icopy++;
	}
      }
     }
   }
  
  
/*
  // loop over the cap for LAPPDs   << now included above.
  G4int CapNCell2 = WCCapEdgeLimit/WCCapLAPPDSpacing + 2;
  for ( int i = -CapNCell2 ; i <  CapNCell2; i++) {
    for (int j = -CapNCell2 ; j <  CapNCell2; j++)   {
       xoffset2 = i*WCCapLAPPDSpacing + WCCapLAPPDSpacing*0.5;
       yoffset2 = j*WCCapLAPPDSpacing + WCCapLAPPDSpacing*0.5;
       G4ThreeVector cellpos = G4ThreeVector(xoffset2, yoffset2, 0);
       if (((sqrt(xoffset2*xoffset2 + yoffset2*yoffset2) + WCLAPPDRadius) < WCCapEdgeLimit) ) {
         G4VPhysicalVolume* physiCapLAPPD =
            new G4PVPlacement(WCCapPMTRotation,
                              cellpos,                   // its position
                              logicWCLAPPD,              // its logical volume
                              "WCLAPPD",                 // its name 
                              logicWCCap,                // its mother volume
                              false,                     // no boolean os
                              icopylappd,                // every PMT need a unique id.
                              false);                    // don't check overlaps
         icopylappd++;
       }
    }
  }
*/
  G4cout << "total on cap: " << icopy <<" total lappds on cap "<<icopylappd<< "\n";
  G4cout << "Coverage was calculated to be: " << (icopy*WCPMTRadius*WCPMTRadius/(WCIDRadius*WCIDRadius)) << "\n";
  if(isANNIE) G4cout << "LAPPD Coverage was calculated to be: " << (icopylappd*WCLAPPDRadius*WCLAPPDRadius/(WCIDRadius*WCIDRadius)) << "\n";

    ///////////////   Barrel PMT placement
  G4RotationMatrix* WCPMTRotation = new G4RotationMatrix;
  WCPMTRotation->rotateY(90.*deg);

  G4double compressionfactor;
  (WCDetectorName=="ANNIEp2") ? compressionfactor=1.0 : compressionfactor=0.9;
  // squeeze PMTs closer to prevent geometry overlap in ANNIE
  G4double barrelCellWidth = 2.*WCIDRadius*tan(dPhi/2.)*compressionfactor;
  G4double horizontalSpacing   = barrelCellWidth/WCPMTperCellHorizontal;
  G4double verticalSpacing     = barrelCellHeight/WCPMTperCellVertical;
  
  if(WCDetectorName=="ANNIEp2v2"){
    if(zflip>0) 
      thecollectionName = WCTankCollectionNames.at(0);   // one outermost ring is 8x 10" WB/LUX PMTs
    else 
      thecollectionName = WCTankCollectionNames.at(2);   // other outermost ring is 8x 8" HQE PMTs
    WCPMTName = WCPMTNameMap.at(thecollectionName);
    logicWCPMT = ConstructPMT(WCPMTName, thecollectionName, "tank");
  }

  for(G4double i = 0; i < WCPMTperCellHorizontal; i++){
    for(G4double j = 0; j < WCPMTperCellVertical; j++){
	if((WCDetectorName!="ANNIEp2")||(((int)i)%2==0)){
      G4ThreeVector PMTPosition =  G4ThreeVector(WCIDRadius,
						 -barrelCellWidth/2.+(i+0.5)*horizontalSpacing,
						 (-barrelCellHeight/2.+(j+0.5)*verticalSpacing)*zflip);
      if(WCDetectorName=="ANNIEp2v2") PMTPosition = G4ThreeVector(WCIDRadius,0,0);

		 G4VPhysicalVolume* physiWCBarrelBorderPMT =
		new G4PVPlacement(WCPMTRotation,                      // its rotation
				  PMTPosition,
				  logicWCPMT,                // its logical volume
				  "WCPMT",             // its name
				  logicWCBarrelBorderCell,         // its mother volume
				  false,                     // no boolean operations
				  (int)(i*WCPMTperCellVertical+j)
				  ,true);                      // no particular field
      if(WCDetectorName=="ANNIEp2v2") break; // only 1 ring in borders
	} else {
      G4ThreeVector PMTPosition =  G4ThreeVector(WCIDRadius-((outerAnnulusRadius-innerAnnulusRadius)/2.),
						 -barrelCellWidth/2.+(i+0.5)*horizontalSpacing,
						 (-barrelCellHeight/2.+(j+0.5)*verticalSpacing)*zflip);
		G4VPhysicalVolume* physiWCBarrelBorderLAPPD =
		new G4PVPlacement(WCPMTRotation,                      // its rotation
				  PMTPosition,                                // its position
				  logicWCLAPPD,                               // its logical volume
				  "WCLAPPD",                                  // its name
				  logicWCBarrelBorderCell,                    // its mother volume
				  false,                                      // no boolean operations
				  (int)(i*WCPMTperCellVertical+j),            // a unique copy number for the LAPPD
				  false);                                     // don't check overlaps
	}


   // logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
     // daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
		// this is still the case.
    }
    if(WCDetectorName=="ANNIEp2v2") break; // only 1 pmt per cell in border rings
  }
  //-------------------------------------------------------------
  // Add PMTs in extra Tower if necessary
  //------------------------------------------------------------


  if(!(WCBarrelRingNPhi*WCPMTperCellHorizontal == WCBarrelNumPMTHorizontal)
    && (WCDetectorName!="ANNIEp2v2")){

    G4RotationMatrix* WCPMTRotation = new G4RotationMatrix;
    WCPMTRotation->rotateY(90.*deg);
    WCPMTRotation->rotateX((2*pi-totalAngle)/2.);//align the PMT with the Cell
                                                 
    G4double towerWidth = WCIDRadius*tan(2*pi-totalAngle);

    G4double horizontalSpacing   = towerWidth/(WCBarrelNumPMTHorizontal-WCBarrelRingNPhi*WCPMTperCellHorizontal);
    G4double verticalSpacing     = barrelCellHeight/WCPMTperCellVertical;

    for(G4double i = 0; i < (WCBarrelNumPMTHorizontal-WCBarrelRingNPhi*WCPMTperCellHorizontal); i++){
      for(G4double j = 0; j < WCPMTperCellVertical; j++){
	G4ThreeVector PMTPosition =  G4ThreeVector(WCIDRadius/cos(dPhi/2.)*cos((2.*pi-totalAngle)/2.),
				towerWidth/2.-(i+0.5)*horizontalSpacing,
			       (-barrelCellHeight/2.+(j+0.5)*verticalSpacing)*zflip);
	PMTPosition.rotateZ(-(2*pi-totalAngle)/2.); // align with the symmetry 
	                                            //axes of the cell 
	
	G4VPhysicalVolume* physiWCBarrelBorderPMT =
	  new G4PVPlacement(WCPMTRotation,                          // its rotation
			    PMTPosition,
			    logicWCPMT,                // its logical volume
			    "WCPMT",             // its name
			    logicWCExtraBorderCell,         // its mother volume
			    false,                     // no boolean operations
			    (int)(i*WCPMTperCellVertical+j)
			    ,true);                        // no particular field

		// logicWCPMT->GetDaughter(0),physiCapPMT is the glass face. If you add more 
		// daugter volumes to the PMTs (e.g. a acryl cover) you have to check, if
		// this is still the case.
      }
    }

  }

return logicCapAssembly;

}


