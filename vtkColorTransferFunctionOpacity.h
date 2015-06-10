/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunctionOpacity.h 

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkColorTransferFunctionOpacity - a filter that takes a
// vtkColorTransferFunction and a vtkPiecewiseFunction to creates a RGBA
// lookup table.
//
// .SECTION Description
// vtkColorTransferFunctionOpacity - a filter that takes a
// vtkColorTransferFunction and a vtkPiecewiseFunction to creates a RGBA
// lookup table.
//
// This class is helpful when slicing a volume and applying the
// same color and opacity functions to the slice. The output of
// this filter is a vtkLookupTable that can then be fed as input
// to vtkImageMapToColors.
//
// .SECTION see also
// vtkLookupTable vtkColorTransferFunction vtkPiecewiseFunction
// vtkImageMapToColors

#ifndef __vtkColorTransferFunctionOpacity_h
#define __vtkColorTransferFunctionOpacity_h

#include <vtkAlgorithm.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

// Forward declarations
class vtkInformation;
class vtkInformationVector;

class vtkColorTransferFunctionOpacity : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkColorTransferFunctionOpacity, vtkAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Create a default vtkLookupTable
  static vtkColorTransferFunctionOpacity* New();

  // Description:
  // Get the output lookup table for a port on this filter
  vtkLookupTable* GetOutput();
  vtkLookupTable* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Set/Get the color transfer function
  vtkSetObjectMacro(ColorFunction, vtkColorTransferFunction);
  vtkGetObjectMacro(ColorFunction, vtkColorTransferFunction);

  // Description:
  // Set/Get the opacity function
  vtkSetObjectMacro(OpacityFunction, vtkPiecewiseFunction);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

protected:
  vtkColorTransferFunctionOpacity();
  ~vtkColorTransferFunctionOpacity();

  // Description:
  // This is called by the superclass
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Fill output port information
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  vtkColorTransferFunction* ColorFunction;
  vtkPiecewiseFunction* OpacityFunction;

private:
  vtkColorTransferFunctionOpacity(const vtkColorTransferFunctionOpacity&); // Not implemented
  void operator=(const vtkColorTransferFunctionOpacity); // Not implemented
};

#endif //__vtkColorTransferFunctionOpacity_h
