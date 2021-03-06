/*
  macro to extract the OCDB entries

  input: CalibObjects.root
  ouput: TimeGain and TimeVdrift calibration objects for TPC and TRD

  Example:
  .L $ALICE_PHYSICS/PWGPP/CalibMacros/CPass1/makeOCDB.C
  makeOCDB("105160");

*/

void makeOCDB(Int_t runNumber, TString  targetOCDBstorage="", TString sourceOCDBstorage="raw://", Int_t detectorBitsQualityFlag = -1)
{
  //
  // extract OCDB entries for detectors participating in the calibration for the current run
  //

  // in case there are some specific storages (for local testing)
  if (gSystem->AccessPathName("localOCDBaccessConfig.C", kFileExists)==0) {

    Printf("Loading localOCDBaccessConfig");
    gROOT->LoadMacro("localOCDBaccessConfig.C");
  }
  
  // config calib train
  gROOT->LoadMacro("$ALICE_PHYSICS/PWGPP/CalibMacros/CPass1/ConfigCalibTrain.C");

  // main macro
  gROOT->Macro(TString::Format("$ALIDPG_ROOT/DataProc/CPass1/main_makeOCDB.C(%d, \"%s\", \"%s\", %d)", runNumber, targetOCDBstorage.Data(), sourceOCDBstorage.Data(), detectorBitsQualityFlag));

}


