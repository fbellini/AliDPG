/*
   rec.C to be used for pass0
   - reconstruction of raw data
   - QA information switched off
   - store all friends
   - default OCDB storage set to "raw://"

   Example:
   aliroot -b -q 'recCPass0.C("raw.root",100)'
*/

void main_recCPass0(const char *filename="raw.root",Int_t nevents=-1, const char *ocdb="raw://", const char* options="?Trigger=kCalibBarrel")
{

  if (gSystem->Getenv("ALIROOT_FORCE_COREDUMP"))
  {
    printf("ALIROOT_FORCE_COREDUMP set\n");
    gSystem->ResetSignal(kSigFloatingException);
    gSystem->ResetSignal(kSigSegmentationViolation);
  }
  // addopt errors to account for calibration imprefection before cpass0
  // 1) For TPC
  Double_t tpcSystematicErrors[5]={1,1.5,1./100.,1./100.,0.1};
  Double_t tpcSystematicErrorClusters[2]={1.5,1.5};
  Double_t tpcExtendedRoads[2]={2.5,2.5};
  Double_t tpcPrimDCACuts[2] = {10.,30.}; // Y,Z
  TVectorD *vectpcSystematicErrors=new TVectorD(5, tpcSystematicErrors);
  TVectorD *vectpcSystematicErrorClusters=new TVectorD(2, tpcSystematicErrorClusters);
  TVectorD *vectpcExtendedRoads= new TVectorD(2, tpcExtendedRoads);
  TVectorD *vectpcPrimDCACuts = new TVectorD(2, tpcPrimDCACuts);
  const double kZOutSectorCut = 3.; // cut on clusters on wrong side of CE (added to extendedRoadZ)
  const double kPrimaryZ2XCut = 1.2; // cut on clusters Z/X (large eta)
  AliTPCReconstructor::SetSystematicError(vectpcSystematicErrors);
  AliTPCReconstructor::SetSystematicErrorCluster(vectpcSystematicErrorClusters);
  AliTPCReconstructor::SetExtendedRoads(vectpcExtendedRoads);
  AliTPCReconstructor::SetPrimaryDCACut(vectpcPrimDCACuts);
  AliTPCReconstructor::SetZOutSectorCut(kZOutSectorCut);
  AliTPCReconstructor::SetPrimaryZ2XCut(kPrimaryZ2XCut);
  //
  if (gSystem->Getenv("streamLevel")){
    AliTPCReconstructor::SetStreamLevel( AliTPCtracker::kStreamErrParam| AliTPCtracker::kStreamTransform);
  }
  // 2) For ITS
  AliITSReconstructor::SetCheckInvariant(kFALSE); // no invariant check with extended TPC errors
  // 3) For TOF
  // extra tolerance on top of default from recoparam
  AliTOFReconstructor::SetExtraTolerance(5.0);
  // 4) For TRD
  AliTRDReconstructor::SetExtraMaxClPerLayer(3); // to allow >6 cluster candidates per layer
  AliTRDReconstructor::SetExtraBoundaryTolerance(3); // relax boundary check
  AliTRDReconstructor::SetExtraRoadY(4); // extra road in Y
  AliTRDReconstructor::SetExtraRoadZ(6); // extra road in Z
  AliTRDReconstructor::SetExtraChi2Out(25); // extra chi2 tolerance on backpropagation
  //
  // Load some system libs for Grid and monitoring
  // Set the CDB storage location
  AliCDBManager * man = AliCDBManager::Instance();
  man->SetDefaultStorage(ocdb);
  // Reconstruction settings
  AliReconstruction rec;
  //
  // do we extract the TPC recpoints in advance
  Int_t preclustTPC = TString(gSystem->Getenv("preclusterizeTPC")).Atoi();
  Bool_t noTPCLocalRec = preclustTPC==1;
  if (noTPCLocalRec) printf("preclustTPC = %d: TPC local reconstruction assumed to be already done\n", preclustTPC);
  else printf("preclustTPC = %d: TPC local reconstruction assumed to NOT be already done\n", preclustTPC);
  //
  if (gSystem->Getenv("disableOuter")!=NULL){
    TString disOuter = gSystem->Getenv("disableOuter");
    TString disOuterLoc = disOuter;
    if (noTPCLocalRec) {
      disOuterLoc.ReplaceAll("TPC","");
      disOuterLoc.ReplaceAll("HLT","");
    }
    rec.SetRunReconstruction(disOuter.Data());
    rec.SetRunLocalReconstruction(disOuterLoc.Data());
  } 
  else if (noTPCLocalRec) {
    rec.SetRunReconstruction("ALL -HLT");
    rec.SetRunLocalReconstruction("ALL -TPC -HLT");
  }
  else {
    rec.SetRunLocalReconstruction("ALL");
  }

  /** fast magnetic field **/
  Bool_t useFast=kTRUE;
  if(gSystem->Getenv("ALIEN_JDL_DISABLEFASTB")){
    TString optionFast=gSystem->Getenv("ALIEN_JDL_DISABLEFASTB");
    if(optionFast!=""){
      printf("Fast magnetic field is disabled via ALIEN_JDL_DISABLEFASTB\n");
      useFast=kFALSE;
    }
  }
  if (useFast && !AliMagF::Class()->GetMethodAny("SetFastFieldDefault")) {
    printf("ERROR: fast magnetic field is requested, but AliRoot version is not compatible\n");
    useFast=kFALSE;
  }
  if(useFast){
      printf(">>>>> Using fast magnetic field \n");
      AliMagF::SetFastFieldDefault(kTRUE);
  }

  // Upload CDB entries from the snapshot (local root file) if snapshot exist
  if (gSystem->AccessPathName("OCDB.root", kFileExists)==0) {        
    rec.SetDefaultStorage("local://");
    rec.SetCDBSnapshotMode("OCDB.root");
  }
  else {
    // setup ocdb by custom (if any) or default settings
    if (gSystem->AccessPathName("localOCDBaccessConfig.C", kFileExists)==0) {        
      gInterpreter->ProcessLine("localOCDBaccessConfig();");
    }
    else { // default settings
      AliCDBManager* man = AliCDBManager::Instance();
      man->SetRaw(kTRUE);
    }
    TString cdbMode = gSystem->Getenv("OCDB_SNAPSHOT_CREATE");
    if (cdbMode == "kTRUE") {
      gInterpreter->ProcessLine(TString::Format("CreateSnapshot(\"OCDB.root\", \"%s\")", filename));
      return;
    }
  }

  // All friends
  rec.SetFractionFriends(2.0);

 // AliReconstruction settings - hardwired MB trigger for calibration

  TString newfilename = filename;
  newfilename += options;
  rec.SetInput(newfilename.Data());

  // Set protection against too many events in a chunk (should not happen)
  if (nevents>0) rec.SetEventRange(0,nevents);

  // Remove recpoints after each event
  TString delRecPoints="TPC TRD ITS";
  if (noTPCLocalRec) delRecPoints.ReplaceAll("TPC","");
  rec.SetDeleteRecPoints(delRecPoints.Data()); 
  //


  // Switch off the V0 finder - saves time!
  //  rec.SetRunMultFinder(kFALSE);
  rec.SetRunV0Finder(kFALSE); 

  //
  // QA options - all QA is off
  //
  rec.SetRunQA(":");
  rec.SetRunGlobalQA(kFALSE);

  // AliReconstruction settings
  rec.SetWriteESDfriend(kTRUE);
  rec.SetWriteAlignmentData();
  rec.SetUseTrackingErrorsForAlignment("ITS");
  rec.SetCleanESD(kFALSE);

  // ITS options (see ALIROOT-7179)
  float itsErrY2[6]={0., 0., 0.1*0.1, 0.1*.1, 0.,0.};
  TVectorF *vecITSErrY2 = new TVectorF(6, itsErrY2);
  AliITSReconstructor::SetExtraErrorY2(vecITSErrY2);
  
  //Ignore SetStopOnError
  rec.SetStopOnError(kFALSE);

  AliLog::Flush();
  rec.Run();
}

