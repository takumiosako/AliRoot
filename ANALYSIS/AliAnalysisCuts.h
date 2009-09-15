#ifndef ALIANALYSISCUTS_H
#define ALIANALYSISCUTS_H
 
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

// Base class for analysis cuts
// Author Andreas Morsch
// andreas.morsch@cern.ch

#include <TNamed.h>
class TList;

class AliAnalysisCuts : public TNamed
{
 public:
    AliAnalysisCuts();
    AliAnalysisCuts(const char* name, const char* title);
    AliAnalysisCuts(const AliAnalysisCuts& obj);  
    virtual ~AliAnalysisCuts() {;}
    virtual Bool_t IsSelected(TObject* obj)  = 0;
    virtual Bool_t IsSelected(TList*  list)  = 0;
    virtual void   Init() {;}
    virtual void   SetFilterMask(UInt_t mask) {fFilterMask = mask;}
    virtual UInt_t GetFilterMask()   const    {return fFilterMask;}	    
 private:
    UInt_t fFilterMask; // Mask to use one of the previous decisions inside a filter
    ClassDef(AliAnalysisCuts, 4); // Base class for filter decisions on ESD objects
};
 
#endif
