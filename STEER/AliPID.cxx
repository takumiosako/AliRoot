/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// particle id probability densities                                         //
//                                                                           //
// The AliPID class stores the probability densities for the different       //
// particle type hypotheses electron, muon, pion, kaon, proton, photon,      //
// pi0, neutron, K0 and electron conversion. These probability densities     //
// are determined from the detector response functions.                      //
// The * and *= operators are overloaded for AliPID to combine the PIDs      //
// from different detectors.                                                 //
//                                                                           //
// The Bayesian probability to be a particle of a given type can be          //
// calculated from the probability densities, if the a priori probabilities  //
// (or abundences, concentrations) of particle species are known. These      //
// priors can be given as argument to the GetProbability or GetMostProbable  //
// method or they can be set globally by calling the static method           //
// SetPriors().                                                              //
//                                                                           //
// The implementation of this class is based on the note ...                 //
// by Iouri Belikov and Karel Safarik.                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#include "AliPID.h"
#include "AliLog.h"
#include <TPDGCode.h>
#include <TDatabasePDG.h>
#include <TClass.h>


ClassImp(AliPID)


Float_t AliPID::fgkParticleMass[AliPID::kSPECIESN+1] = {
  0.00051,    // electron
  0.10566,    // muon
  0.13957,    // pion
  0.49360,    // kaon
  0.93827,    // proton
  0.00000,    // photon
  0.13498,    // pi0
  0.93957,    // neutron
  0.49767,    // kaon0
  0.00000,    // electron conversion
  0.00000     // unknown
};

const char* AliPID::fgkParticleName[AliPID::kSPECIESN+1] = {
  "electron",
  "muon",
  "pion",
  "kaon",
  "proton",
  "photon",
  "pi0",
  "neutron",
  "kaon0",
  "eleCon",
  "unknown"
};

const Int_t AliPID::fgkParticleCode[AliPID::kSPECIESN+1] = {
  ::kElectron, 
  ::kMuonMinus, 
  ::kPiPlus, 
  ::kKPlus, 
  ::kProton,
  ::kGamma,
  ::kPi0,
  ::kNeutron,
  ::kK0,
  ::kGamma,
  0
};


Double_t AliPID::fgPrior[kSPECIESN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


//_______________________________________________________________________
AliPID::AliPID()
{
// set default values (= equal probabilities)

  for (Int_t i = 0; i < kSPECIESN; i++) {
    fProbDensity[i] = 1./kSPECIESN;
  }
}

//_______________________________________________________________________
AliPID::AliPID(const Double_t* probDensity, Bool_t charged)
{
// set given probability densities

  fCharged = charged;
  for (Int_t i = 0; i < kSPECIES; i++) {
    fProbDensity[i] = probDensity[i];
  }
  for (Int_t i = kSPECIES; i < kSPECIESN; i++) {
    fProbDensity[i] = ((charged) ? 0 : probDensity[i]);
  }
}

//_______________________________________________________________________
AliPID::AliPID(const Float_t* probDensity, Bool_t charged)
{
// set given probability densities

  fCharged = charged;
  for (Int_t i = 0; i < kSPECIES; i++) {
    fProbDensity[i] = probDensity[i];
  }
  for (Int_t i = kSPECIES; i < kSPECIESN; i++) {
    fProbDensity[i] = ((charged) ? 0 : probDensity[i]);
  }
}

//_______________________________________________________________________
AliPID::AliPID(const AliPID& pid) : 
  TObject(pid),
  fCharged(pid.fCharged)
{
// copy constructor

  for (Int_t i = 0; i < kSPECIESN; i++) {
    fProbDensity[i] = pid.fProbDensity[i];
  }
}

//_______________________________________________________________________
AliPID& AliPID::operator = (const AliPID& pid)
{
// assignment operator

  fCharged = pid.fCharged;
  for (Int_t i = 0; i < kSPECIESN; i++) {
    fProbDensity[i] = pid.fProbDensity[i];
  }
  return *this;
}


//_____________________________________________________________________________
Double_t AliPID::GetProbability(EParticleType iType,
				const Double_t* prior) const
{
// get the probability to be a particle of type "iType"
// assuming the a priori probabilities "prior"

  Double_t sum = 0.;
  Int_t nSpecies = ((fCharged) ? kSPECIES : kSPECIESN);
  for (Int_t i = 0; i < nSpecies; i++) {
    sum += fProbDensity[i] * prior[i];
  }
  if (sum <= 0) {
    AliError("Invalid probability densities or priors");
    return -1;
  }
  return fProbDensity[iType] * prior[iType] / sum;
}

//_____________________________________________________________________________
Double_t AliPID::GetProbability(EParticleType iType) const
{
// get the probability to be a particle of type "iType"
// assuming the globaly set a priori probabilities

  return GetProbability(iType, fgPrior);
}

//_____________________________________________________________________________
void AliPID::GetProbabilities(Double_t* probabilities,
			      const Double_t* prior) const
{
// get the probabilities to be a particle of given type
// assuming the a priori probabilities "prior"

  Double_t sum = 0.;
  Int_t nSpecies = ((fCharged) ? kSPECIES : kSPECIESN);
  for (Int_t i = 0; i < nSpecies; i++) {
    sum += fProbDensity[i] * prior[i];
  }
  if (sum <= 0) {
    AliError("Invalid probability densities or priors");
    for (Int_t i = 0; i < nSpecies; i++) probabilities[i] = -1;
    return;
  }
  for (Int_t i = 0; i < nSpecies; i++) {
    probabilities[i] = fProbDensity[i] * prior[i] / sum;
  }
}

//_____________________________________________________________________________
void AliPID::GetProbabilities(Double_t* probabilities) const
{
// get the probabilities to be a particle of given type
// assuming the globaly set a priori probabilities

  GetProbabilities(probabilities, fgPrior);
}

//_____________________________________________________________________________
AliPID::EParticleType AliPID::GetMostProbable(const Double_t* prior) const
{
// get the most probable particle id hypothesis
// assuming the a priori probabilities "prior"

  Double_t max = 0.;
  EParticleType id = kPion;
  Int_t nSpecies = ((fCharged) ? kSPECIES : kSPECIESN);
  for (Int_t i = 0; i < nSpecies; i++) {
    Double_t prob = fProbDensity[i] * prior[i];
    if (prob > max) {
      max = prob;
      id = EParticleType(i);
    }
  }
  if (max == 0) {
    AliError("Invalid probability densities or priors");
  }
  return id;
}

//_____________________________________________________________________________
AliPID::EParticleType AliPID::GetMostProbable() const
{
// get the most probable particle id hypothesis
// assuming the globaly set a priori probabilities

  return GetMostProbable(fgPrior);
}


//_____________________________________________________________________________
void AliPID::SetPriors(const Double_t* prior, Bool_t charged)
{
// use the given priors as global a priori probabilities

  Double_t sum = 0;
  for (Int_t i = 0; i < kSPECIESN; i++) {
    if (charged && (i >= kSPECIES)) {
      fgPrior[i] = 0;      
    } else {
      if (prior[i] < 0) {
	AliWarningClass(Form("negative prior (%g) for %ss. "
			     "Using 0 instead.", prior[i], 
			     fgkParticleName[i]));
	fgPrior[i] = 0;
      } else {
	fgPrior[i] = prior[i];
      }
    }
    sum += prior[i];
  }
  if (sum == 0) {
    AliWarningClass("all priors are zero.");
  }
}

//_____________________________________________________________________________
void AliPID::SetPrior(EParticleType iType, Double_t prior)
{
// use the given prior as global a priori probability for particles
// of type "iType"

  if (prior < 0) {
    AliWarningClass(Form("negative prior (%g) for %ss. Using 0 instead.", 
			 prior, fgkParticleName[iType]));
    prior = 0;
  }
  fgPrior[iType] = prior;
}


//_____________________________________________________________________________
void AliPID::Init()
{
// initialize the mass values from the PDG database

  for (Int_t i = 0; i < kSPECIESN; i++) {
    fgkParticleMass[i] =
      TDatabasePDG::Instance()->GetParticle(fgkParticleCode[i])->Mass();
  }
}


//_____________________________________________________________________________
AliPID& AliPID::operator *= (const AliPID& pid)
{
// combine this probability densities with the one of "pid"

  for (Int_t i = 0; i < kSPECIESN; i++) {
    fProbDensity[i] *= pid.fProbDensity[i];
  }
  return *this;
}

//_____________________________________________________________________________
AliPID operator * (const AliPID& pid1, const AliPID& pid2)
{
// combine the two probability densities

  AliPID result;
  result *= pid1;
  result *= pid2;
  return result;
}
