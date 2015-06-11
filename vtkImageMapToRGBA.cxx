/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToRGBA.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMapToRGBA.h"

#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLookupTable.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>

vtkStandardNewMacro(vtkImageMapToRGBA);

//-----------------------------------------------------------------------------
vtkImageMapToRGBA::vtkImageMapToRGBA()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->ColorFunction = NULL;
  this->OpacityFunction = NULL;
  this->NumberOfColors = 256;

  this->LookupTable = vtkLookupTable::New();
  this->LookupTable->SetNumberOfTableValues(256);
  this->LookupTable->Build();
}

//-----------------------------------------------------------------------------
vtkImageMapToRGBA::~vtkImageMapToRGBA()
{
  if (this->ColorFunction)
    {
    this->ColorFunction->UnRegister(this);
    this->ColorFunction = NULL;
    }

  if (this->OpacityFunction)
    {
    this->OpacityFunction->UnRegister(this);
    this->OpacityFunction = NULL;
    }

  this->LookupTable->Delete();
  this->LookupTable = NULL;
}

//----------------------------------------------------------------------------
void vtkImageMapToRGBA::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->ColorFunction)
    {
    os << indent << "ColorFunction: ";
    this->ColorFunction->PrintSelf(os, indent.GetNextIndent());
    }
  if (this->OpacityFunction)
    {
    os << indent << "OpacityFunction: ";
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkImageMapToRGBA::SetColorFunction(
                                        vtkColorTransferFunction* cf)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " <<
                "ColorFunction to " << cf);
  if (this->ColorFunction != cf)
    {
    if (this->ColorFunction != NULL)
      {
      this->ColorFunction->UnRegister(this);
      }
    this->ColorFunction = cf;
    if (this->ColorFunction != NULL)
      {
      this->ColorFunction->Register(this);
      this->ColorFunction->AddObserver(vtkCommand::ModifiedEvent, this,
                          &vtkImageMapToRGBA::UpdateLookupTable);
      this->UpdateLookupTable();
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageMapToRGBA::SetOpacityFunction(
                                        vtkPiecewiseFunction* pwf)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " <<
                "OpacityFunction to " << pwf);
  if (this->OpacityFunction != pwf)
    {
    if (this->OpacityFunction != NULL)
      {
      this->OpacityFunction->UnRegister(this);
      }
    this->OpacityFunction = pwf;
    if (this->OpacityFunction != NULL)
      {
      this->OpacityFunction->Register(this);
      this->OpacityFunction->AddObserver(vtkCommand::ModifiedEvent, this,
                          &vtkImageMapToRGBA::UpdateLookupTable);
      this->UpdateLookupTable();
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkImageMapToRGBA::RequestData(vtkInformation* vtkNotUsed(request),
                                                 vtkInformationVector** inputVector,
                                                 vtkInformationVector* outputVector)
{
  // During RequestData, this filter examines the color and opacity
  // functions and fills in that empty lookup table with real
  // data values
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData* input = vtkImageData::SafeDownCast(
                            inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(
                              outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkImageMapToColors* imageMap = vtkImageMapToColors::New();
  imageMap->SetInputData(input);
  imageMap->SetLookupTable(this->LookupTable);
  imageMap->SetOutputFormatToRGBA();
  imageMap->Update();

  output->DeepCopy(imageMap->GetOutput());

  imageMap->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageMapToRGBA::UpdateLookupTable(void)
{
  this->LookupTable->SetNumberOfTableValues(this->NumberOfColors);
  this->LookupTable->Build();

  if (this->ColorFunction)
    {
    double colorRange[2];
    this->ColorFunction->GetRange(colorRange);
    this->LookupTable->SetRange(colorRange);
    for (vtkIdType i = 0; i < this->NumberOfColors; ++i)
      {
      double value = i*(colorRange[1] - colorRange[0]) / 
                          (this->NumberOfColors - 1) + colorRange[0];
      double rgb[3];
      this->ColorFunction->GetColor(value, rgb);
      double o = 1.0;
      if (this->OpacityFunction)
        {
        o = this->OpacityFunction->GetValue(value);
        }
      this->LookupTable->SetTableValue(i, rgb[0], rgb[1], rgb[2], o);
      }
    }

  this->Modified();
}
