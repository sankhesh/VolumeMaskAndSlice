/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunctionOpacity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorTransferFunctionOpacity.h"

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>

vtkStandardNewMacro(vtkColorTransferFunctionOpacity);

//-----------------------------------------------------------------------------
vtkColorTransferFunctionOpacity::vtkColorTransferFunctionOpacity()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->ColorFunction = NULL;
  this->OpacityFunction = NULL;
}

//-----------------------------------------------------------------------------
vtkColorTransferFunctionOpacity::~vtkColorTransferFunctionOpacity()
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
}

//----------------------------------------------------------------------------
void vtkColorTransferFunctionOpacity::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->ColorFunction)
    {
    os << indent << " ColorFunction " << endl;
    this->ColorFunction->PrintSelf(os, indent.GetNextIndent());
    }
  if (this->OpacityFunction)
    {
    os << indent << " OpacityFunction " << std::endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
vtkLookupTable* vtkColorTransferFunctionOpacity::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkLookupTable* vtkColorTransferFunctionOpacity::GetOutput(int port)
{
  return vtkLookupTable::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void vtkColorTransferFunctionOpacity::SetOutput(vtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int vtkColorTransferFunctionOpacity::ProcessRequest(vtkInformation* request,
                                                    vtkInformationVector** inputVector,
                                                    vtkInformationVector* outputVector)
{
  // Create an output object of the correct type
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkColorTransferFunctionOpacity::RequestData(vtkInformation* vtkNotUsed(request),
                                                 vtkInformationVector** vtkNotUsed(inputVector),
                                                 vtkInformationVector* outputVector)
{
  // Later RD happens.
  // During RD, each filter examines the color and opacity
  // functions and fills in that empty lookup table with real
  // data values
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkLookupTable* output = vtkLookupTable::SafeDownCast(
                              outInfo->Get(vtkDataObject::DATA_OBJECT()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkColorTransferFunctionOpacity::FillOutputPortInformation(int vtkNotUsed(port),
                                                               vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkLookupTable");
  return 1;
}
