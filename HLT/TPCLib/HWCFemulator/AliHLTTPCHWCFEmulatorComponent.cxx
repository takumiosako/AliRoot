//****************************************************************************
//* This file is property of and copyright by the ALICE HLT Project          * 
//* ALICE Experiment at CERN, All rights reserved.                           *
//*                                                                          *
//* Primary Authors: Sergey Gorbunov, Torsten Alt                            *
//* Developers:      Sergey Gorbunov <sergey.gorbunov@fias.uni-frankfurt.de> *
//*                  Torsten Alt <talt@cern.ch>                              *
//*                  for The ALICE HLT Project.                              *
//*                                                                          *
//* Permission to use, copy, modify and distribute this software and its     *
//* documentation strictly for non-commercial purposes is hereby granted     *
//* without fee, provided that the above copyright notice appears in all     *
//* copies and that both the copyright notice and this permission notice     *
//* appear in the supporting documentation. The authors make no claims       *
//* about the suitability of this software for any purpose. It is            *
//* provided "as is" without express or implied warranty.                    *
//****************************************************************************

//  @file   AliHLTTPCHWCFEmulatorComponent.cxx
//  @author Sergey Gorbunov <sergey.gorbunov@fias.uni-frankfurt.de>
//  @author Torsten Alt <talt@cern.ch> 
//  @brief  HLT Component interface for for FPGA ClusterFinder Emulator for TPC
//  @brief  ( see AliHLTTPCHWCFEmulator class )
//  @note


#if __GNUC__>= 3
using namespace std;
#endif
#include "AliHLTTPCHWCFEmulatorComponent.h"

#include "AliHLTTPCDefinitions.h"
#include "AliHLTTPCHWCFDataTypes.h"
#include "AliHLTTPCClusterMCData.h"

#include "AliGRPObject.h"
#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliRawDataHeader.h"
#include <cstdlib>
#include <cerrno>
#include "TString.h"
#include "TObjString.h"
#include "TObjArray.h"


#include <sys/time.h>
#include "TFile.h"

AliHLTTPCHWCFEmulatorComponent::AliHLTTPCHWCFEmulatorComponent()
  :
  AliHLTProcessor(),
  fDoDeconvTime(1),
  fDoDeconvPad(1),
  fDoMC(1),
  fDoFlowControl(0),
  fDoSinglePadSuppression(1),
  fBypassMerger(0),
  fClusterLowerLimit(0),
  fSingleSeqLimit(0),
  fDebug(0),
  fCFSupport(),
  fCFEmulator(),
  fBenchmark("TPCHWClusterFinderEmulator")
{
  // see header file for class documentation
  // or
  // refer to README to build package
  // or
  // visit http://web.ift.uib.no/~kjeks/doc/alice-hlt
}


AliHLTTPCHWCFEmulatorComponent::AliHLTTPCHWCFEmulatorComponent(const AliHLTTPCHWCFEmulatorComponent&)
  :
  AliHLTProcessor(),
  fDoDeconvTime(1),
  fDoDeconvPad(1),
  fDoMC(1),
  fDoFlowControl(0),
  fDoSinglePadSuppression(1),
  fBypassMerger(0),
  fClusterLowerLimit(0),
  fSingleSeqLimit(0),
  fDebug(0),
  fCFSupport(),
  fCFEmulator(),
  fBenchmark("TPCHWClusterFinderEmulator")
{
  // dummy
}

AliHLTTPCHWCFEmulatorComponent& AliHLTTPCHWCFEmulatorComponent::operator=(const AliHLTTPCHWCFEmulatorComponent&)
{
  // dummy
  return *this;
}

AliHLTTPCHWCFEmulatorComponent::~AliHLTTPCHWCFEmulatorComponent()
{
  // see header file for class documentation
}

// Public functions to implement AliHLTComponent's interface.
// These functions are required for the registration process

const char* AliHLTTPCHWCFEmulatorComponent::GetComponentID()
{
  // see header file for class documentation
  return "TPCHWClusterFinderEmulator";
}

void AliHLTTPCHWCFEmulatorComponent::GetInputDataTypes( vector<AliHLTComponentDataType>& list)
{
  // see header file for class documentation
  list.clear();
  list.push_back( AliHLTTPCDefinitions::fgkUnpackedRawDataType ); 	 
  list.push_back( kAliHLTDataTypeDDLRaw | kAliHLTDataOriginTPC );
}

AliHLTComponentDataType AliHLTTPCHWCFEmulatorComponent::GetOutputDataType()
{
  // see header file for class documentation
  return kAliHLTMultipleDataType;
}

int AliHLTTPCHWCFEmulatorComponent::GetOutputDataTypes(AliHLTComponentDataTypeList& tgtList)
{
  // see header file for class documentation
  tgtList.clear();
  tgtList.push_back(AliHLTTPCDefinitions::fgkHWClustersDataType | kAliHLTDataOriginTPC );
  tgtList.push_back(AliHLTTPCDefinitions::fgkAliHLTDataTypeClusterMCInfo | kAliHLTDataOriginTPC );
  return tgtList.size();
}


void AliHLTTPCHWCFEmulatorComponent::GetOutputDataSize( unsigned long& constBase, double& inputMultiplier )
{
  // see header file for class documentation
  // XXX TODO: Find more realistic values.  
  constBase = 0;
  inputMultiplier = (6 * 0.4);
}


AliHLTComponent* AliHLTTPCHWCFEmulatorComponent::Spawn()
{
  // see header file for class documentation
  return new AliHLTTPCHWCFEmulatorComponent();
}

void AliHLTTPCHWCFEmulatorComponent::GetOCDBObjectDescription( TMap* const targetMap){
// Get a list of OCDB object description needed for the particular component
  
  if (!targetMap) return;
  
  // OCDB entries for component arguments
  targetMap->Add(new TObjString("HLT/ConfigTPC/TPCHWClusterFinder"), new TObjString("component arguments, empty at the moment"));
}


int AliHLTTPCHWCFEmulatorComponent::DoInit( int argc, const char** argv )
{
  // see header file for class documentation

  TString arguments = "";
  for ( int i = 0; i < argc; i++ ) {
    if ( !arguments.IsNull() ) arguments += " ";
    arguments += argv[i];
  }

  return Configure( NULL, NULL, arguments.Data()  );
}

int AliHLTTPCHWCFEmulatorComponent::Reconfigure( const char* cdbEntry, const char* chainId )
{
  // Reconfigure the component from OCDB

  return Configure( cdbEntry, chainId, NULL );
}

int AliHLTTPCHWCFEmulatorComponent::ScanConfigurationArgument(int argc, const char** argv)
{
  // see header file for class documentation
  TString arguments = "";
  for ( int i = 0; i < argc; i++ ) {
    if ( !arguments.IsNull() ) arguments += " ";
    arguments += argv[i];
  }
  return ReadConfigurationString(arguments);
}



void AliHLTTPCHWCFEmulatorComponent::SetDefaultConfiguration()
{
  // Set default configuration for the FPGA ClusterFinder Emulator component
  // Some parameters can be later overwritten from the OCDB

  fDoDeconvTime = 0;
  fDoDeconvPad = 0;
  fDoMC = 1;
  fDoFlowControl = 0;
  fDoSinglePadSuppression = 1;
  fBypassMerger = 0;
  fClusterLowerLimit = 0;
  fSingleSeqLimit = 0;
  fBenchmark.Reset();
  fBenchmark.SetTimer(0,"total");
  fBenchmark.SetTimer(1,"reco");    
}

int AliHLTTPCHWCFEmulatorComponent::ReadConfigurationString(  const char* arguments )
{
  // Set configuration parameters for the FPGA ClusterFinder Emulator component
  // from the string

  int iResult = 0;
  if ( !arguments ) return iResult;

  TString allArgs = arguments;
  TString argument;
  int bMissingParam = 0;

  TObjArray* pTokens = allArgs.Tokenize( " " );

  int nArgs =  pTokens ? pTokens->GetEntries() : 0;

  for ( int i = 0; i < nArgs; i++ ) {
    argument = ( ( TObjString* )pTokens->At( i ) )->GetString();
    if ( argument.IsNull() ) continue;

    if ( argument.CompareTo( "-deconvolute-time" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoDeconvTime  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Time deconvolution is set to: %d", fDoDeconvTime );
      continue;
    }

    if ( argument.CompareTo( "-deconvolute-pad" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoDeconvPad  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Pad deconvolution is set to: %d", fDoDeconvPad );
      continue;
    }

    if ( argument.CompareTo( "-deconvolute" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoDeconvTime  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      fDoDeconvPad  = fDoDeconvTime;
      HLTInfo( "Time and pad deconvolution is set to: %d", fDoDeconvPad );
      continue;
    }
 
    if ( argument.CompareTo( "-do-mc" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoMC  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "MC processing is set to: %d", fDoMC );
      continue;
    }

    if ( argument.CompareTo( "-flow-control" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoFlowControl  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Flow control is set to: %d", fDoFlowControl );
      continue;
    }

    if ( argument.CompareTo( "-single-pad-suppression" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDoSinglePadSuppression  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Single pad suppression is set to: %d", fDoSinglePadSuppression );
      continue;
    }
    
    if ( argument.CompareTo( "-bypass-merger" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fBypassMerger  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Bypassing merger is set to: %d", fBypassMerger );
      continue;
    }

    if ( argument.CompareTo( "-cluster-lower-limit" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fClusterLowerLimit  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Cluster lower limit is set to: %d", fClusterLowerLimit );
      continue;
    }

    if ( argument.CompareTo( "-single-sequence-limit" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fSingleSeqLimit  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Single sequence limit is set to: %d", fSingleSeqLimit );
      continue;
    }
    
    if ( argument.CompareTo( "-debug-level" ) == 0 ) {
      if ( ( bMissingParam = ( ++i >= pTokens->GetEntries() ) ) ) break;
      fDebug  = ( ( TObjString* )pTokens->At( i ) )->GetString().Atoi();
      HLTInfo( "Debug level is set to: %d", fDebug );
      continue;
    }
    
    HLTError( "Unknown option \"%s\"", argument.Data() );
    iResult = -EINVAL;
  }
  delete pTokens;

  if ( bMissingParam ) {
    HLTError( "Specifier missed for parameter \"%s\"", argument.Data() );
    iResult = -EINVAL;
  }

  return iResult;
}


int AliHLTTPCHWCFEmulatorComponent::ReadCDBEntry( const char* cdbEntry, const char* chainId )
{
  // Read configuration from OCDB

  const char* defaultNotify = "";

  if ( !cdbEntry ){
    cdbEntry = "HLT/ConfigTPC/TPCHWClusterFinder";
    defaultNotify = " (default)";
    chainId = 0;
  }

  HLTInfo( "configure from entry \"%s\"%s, chain id %s", cdbEntry, defaultNotify, ( chainId != NULL && chainId[0] != 0 ) ? chainId : "<none>" );
  AliCDBEntry *pEntry = AliCDBManager::Instance()->Get( cdbEntry );//,GetRunNo());
  
  if ( !pEntry ) {
    HLTError( "cannot fetch object \"%s\" from CDB", cdbEntry );
    return -EINVAL;
  }

  TObjString* pString = dynamic_cast<TObjString*>( pEntry->GetObject() );

  if ( !pString ) {
    HLTError( "configuration object \"%s\" has wrong type, required TObjString", cdbEntry );
    return -EINVAL;
  }

  HLTInfo( "received configuration object string: \"%s\"", pString->GetString().Data() );

  return  ReadConfigurationString( pString->GetString().Data() );
}


int AliHLTTPCHWCFEmulatorComponent::Configure( const char* cdbEntry, const char* chainId, const char *commandLine )
{
  // Configure the component
  // There are few levels of configuration,
  // parameters which are set on one step can be overwritten on the next step

  //* read hard-coded values

  SetDefaultConfiguration();

  //* read the default CDB entry

  int iResult1 = ReadCDBEntry( NULL, chainId );

  //* read the actual CDB entry if required

  int iResult2 = ( cdbEntry ) ? ReadCDBEntry( cdbEntry, chainId ) : 0;

  //* read extra parameters from input (if they are)

  int iResult3 = 0;

  if ( commandLine && commandLine[0] != '\0' ) {
    HLTInfo( "received configuration string from HLT framework: \"%s\"", commandLine );
    iResult3 = ReadConfigurationString( commandLine );
  }

  return iResult1 ? iResult1 : ( iResult2 ? iResult2 : iResult3 );
}


int AliHLTTPCHWCFEmulatorComponent::DoDeinit()
{
  // see header file for class documentation 
  return 0;
}


int AliHLTTPCHWCFEmulatorComponent::DoEvent( const AliHLTComponentEventData& evtData, 
							const AliHLTComponentBlockData* blocks, 
							AliHLTComponentTriggerData& /*trigData*/, AliHLTUInt8_t* outputPtr, 
							AliHLTUInt32_t& size, 
							vector<AliHLTComponentBlockData>& outputBlocks )
{
  // see header file for class documentation

  int iResult=0;
  AliHLTUInt32_t maxSize = size;
  size = 0;
  
  if(!IsDataEvent()){
    return 0;
  }

  fBenchmark.StartNewEvent();
  fBenchmark.Start(0);

  AliHLTUInt32_t configWord = AliHLTTPCHWCFEmulator::CreateConfiguration
    ( fDoDeconvTime, fDoDeconvPad, fDoFlowControl, fDoSinglePadSuppression, fBypassMerger, fClusterLowerLimit, fSingleSeqLimit );

  for ( unsigned long ndx = 0; ndx < evtData.fBlockCnt; ndx++ )
    {
      const AliHLTComponentBlockData* iter = blocks+ndx;
      
      if (  iter->fDataType != (kAliHLTDataTypeDDLRaw | kAliHLTDataOriginTPC) 
	    &&  iter->fDataType != AliHLTTPCDefinitions::fgkUnpackedRawDataType ) continue;

      int slice = AliHLTTPCDefinitions::GetMinSliceNr( *iter );
      int patch = AliHLTTPCDefinitions::GetMinPatchNr( *iter );
 
      const char *str=Form("slice %d patch %d:", slice, patch);

      fBenchmark.AddInput(iter->fSize);
 
      if (!iter->fPtr) continue;
 
      // create input block for the HW cluster finder

      const AliHLTUInt32_t *rawEvent=0;
      const AliHLTTPCClusterMCLabel *mcLabels = 0;
      AliHLTUInt32_t rawEventSize32 = 0;
      AliHLTUInt32_t nMCLabels = 0;

      if( fCFSupport.CreateRawEvent( iter, rawEvent, rawEventSize32, mcLabels, nMCLabels )<0 ) continue; 
      if( !fDoMC ){
	mcLabels = 0;
	nMCLabels = 0;
      }

      // book memory for the output
      
      AliHLTUInt32_t maxNClusters = rawEventSize32 + 1; // N 32-bit words in input
      AliHLTUInt32_t clustersSize32 = maxNClusters*5;
      AliHLTUInt32_t nOutputMC = maxNClusters;

      AliHLTUInt32_t headerSize = sizeof(AliRawDataHeader);                   
      AliHLTUInt8_t *outBlock = new AliHLTUInt8_t[ headerSize+clustersSize32*sizeof(AliHLTUInt32_t) ];
      
      AliHLTTPCClusterMCData *outMC = reinterpret_cast<AliHLTTPCClusterMCData *>(new AliHLTTPCClusterMCLabel[nOutputMC+1]);
      
      if( !outBlock || !outMC ){
	HLTWarning("%s Not enouth memory!!!", str);
	delete[] outBlock;
	delete[] outMC;
	continue;	
      }
      
      // fill CDH header here, since the HW clusterfinder does not receive it
      
      AliRawDataHeader *cdhHeader = reinterpret_cast<AliRawDataHeader*>(iter->fPtr);
      AliRawDataHeader *outCDHHeader = reinterpret_cast<AliRawDataHeader*>(outBlock);      
      *outCDHHeader = *cdhHeader;
      outCDHHeader->fSize = 0xFFFFFFFF;

      AliHLTUInt32_t *outClusters = reinterpret_cast<AliHLTUInt32_t*> (outBlock + headerSize);
     
      fBenchmark.Start(1);
      fCFEmulator.Init
	( fCFSupport.GetMapping(patch), configWord );
      
      int err = fCFEmulator.FindClusters( rawEvent, rawEventSize32, 
					  outClusters, clustersSize32, 
					  mcLabels, nMCLabels,
					  outMC );
      fBenchmark.Stop(1);
      if( err==-1 ){ HLTWarning("NULL input pointer (warning %d)",err);}
      else if( err==-2 ){  HLTWarning("No space left in the output buffer (warning %d)",err); }
      else if( err<0 ){ HLTWarning("HWCF emulator finished with error code %d",err); }
      if( err<0 ){
	delete[] outBlock;
	delete[] outMC;
	continue;
      }

      if( fDebug ){
	printf("\nHWCF Emulator: output clusters for slice%d patch %d:\n",slice,patch);
	for( AliHLTUInt32_t i=0; i<clustersSize32; i+=5 ){
	  AliHLTUInt32_t *c = outClusters+i;
	  AliHLTUInt32_t flag = (c[0]>>30);  	  
	  if( flag == 0x3){ //beginning of a cluster
	    int padRow  = (c[0]>>24)&0x3f;
	    int q  = (c[0]&0xFFFFFF)>>6; 
	    double p   = *((AliHLTFloat32_t*)&c[1]);
	    double t  = *((AliHLTFloat32_t*)&c[2]);
	    AliHLTFloat32_t p2 = *((AliHLTFloat32_t*)&c[3]);
	    AliHLTFloat32_t t2 = *((AliHLTFloat32_t*)&c[4]);
	    printf("N: %3d    R: %3d    C: %4d    P:  %7.4f    T:  %8.4f    DP: %6.4f    DT: %6.4f\n", 
		   i/5+1, padRow, q, p, t, sqrt(fabs(p2-p*p)), sqrt(fabs(t2-t*t)));

	    if( outMC && outMC->fCount>0 ){
	      printf("        MC: (%3d,%6.1f) (%3d,%6.1f) (%3d,%6.1f)\n",
		     outMC->fLabels[i/5].fClusterID[0].fMCID,outMC->fLabels[i/5].fClusterID[0].fWeight,
		     outMC->fLabels[i/5].fClusterID[1].fMCID,outMC->fLabels[i/5].fClusterID[1].fWeight,
		     outMC->fLabels[i/5].fClusterID[2].fMCID,outMC->fLabels[i/5].fClusterID[2].fWeight
		     );
	    }
	  }
	}
      }
          

      AliHLTUInt32_t outSize = headerSize + clustersSize32*sizeof(AliHLTUInt32_t);
      
      if( size + outSize <= maxSize ){
	
	memcpy( outputPtr, outBlock, outSize );
	
	AliHLTComponentBlockData bd;
	FillBlockData( bd );
	bd.fOffset = size;
	bd.fSize = outSize;
	bd.fSpecification = iter->fSpecification;
	bd.fDataType = AliHLTTPCDefinitions::fgkHWClustersDataType | kAliHLTDataOriginTPC;
	outputBlocks.push_back( bd );
	fBenchmark.AddOutput(bd.fSize);
	size+= bd.fSize;
	outputPtr+=bd.fSize;
      } else {
	HLTWarning( "Output buffer (%db) is too small, required %db", maxSize, size+outSize);
	iResult=-ENOSPC;
      }

      if( fDoMC && outMC && outMC->fCount>0 ){
	int s = sizeof(AliHLTTPCClusterMCData) + outMC->fCount*sizeof(AliHLTTPCClusterMCLabel);
	if( size + s <= maxSize ){
	  memcpy( outputPtr, outMC, s );	  	  
	  AliHLTComponentBlockData bdMCInfo;
	  FillBlockData( bdMCInfo );
	  bdMCInfo.fOffset = size;
	  bdMCInfo.fSize = s;
	  bdMCInfo.fSpecification = iter->fSpecification;
	  bdMCInfo.fDataType = AliHLTTPCDefinitions::fgkAliHLTDataTypeClusterMCInfo | kAliHLTDataOriginTPC;
	  outputBlocks.push_back( bdMCInfo );
	  fBenchmark.AddOutput(bdMCInfo.fSize);
	  size+=bdMCInfo.fSize;
	  outputPtr+=bdMCInfo.fSize; 
	} else {	
	  HLTWarning( "Output buffer (%db) is too small, required %db", maxSize, size+s);
	  iResult=-ENOSPC;	    
	}
      }
      
      delete[] outBlock;
      delete[] outMC;      
    }
  
  fBenchmark.Stop(0);  
  HLTInfo(fBenchmark.GetStatistics());
  return iResult;
}


