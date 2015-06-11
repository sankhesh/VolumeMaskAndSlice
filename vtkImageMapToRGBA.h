/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToRGBA.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMapToRGBA - a filter that takes a vtkImageData as input,
// a vtkColorTransferFunction and a vtkPiecewiseFunction to create an RGBA
// output image.
//
// .SECTION Description
// vtkImageMapToRGBA - a filter that takes a vtkImageData as input,
// a vtkColorTransferFunction and a vtkPiecewiseFunction to create an RGBA
// output image.
//
// This class is helpful when slicing a volume and applying the
// same color and opacity functions to the slice. The output of
// this filter is a vtkImageData.
//
// This class leverages the vtkImageMapToColors functionality by supporting
// vtkPiecewiseFunction for opacity values.
//
// .SECTION see also
// vtkLookupTable vtkColorTransferFunction vtkPiecewiseFunction
// vtkImageMapToColors

#ifndef __vtkImageMapToRGBA_h
#define __vtkImageMapToRGBA_h

#include <vtkImageAlgorithm.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

// Forward declarations
class vtkInformation;
class vtkInformationVector;
class vtkLookupTable;

class vtkImageMapToRGBA : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageMapToRGBA, vtkAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Create a default vtkLookupTable with 256 colors and
  // values ranging from 0.0 to 1.0
  static vtkImageMapToRGBA* New();

  // Description:
  // Set/Get the color transfer function
  virtual void SetColorFunction(vtkColorTransferFunction* cf);
  vtkGetObjectMacro(ColorFunction, vtkColorTransferFunction);

  // Description:
  // Set/Get the opacity function
  virtual void SetOpacityFunction(vtkPiecewiseFunction* pwf);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

  // Description:
  // Set/Get number of colors in the output image (default: 256)
  // \sa vtkLookupTable::SetNumberOfTableValues()
  vtkSetMacro(NumberOfColors, int);
  vtkGetMacro(NumberOfColors, int);

protected:
  vtkImageMapToRGBA();
  ~vtkImageMapToRGBA();

  // Description:
  // This is called by the superclass
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Update internal lookup table based on functions provided
  void UpdateLookupTable(void);

  vtkColorTransferFunction* ColorFunction;
  vtkPiecewiseFunction* OpacityFunction;
  vtkLookupTable* LookupTable;

  int NumberOfColors;

private:
  vtkImageMapToRGBA(const vtkImageMapToRGBA&); // Not implemented
  void operator=(const vtkImageMapToRGBA); // Not implemented
};

#endif //__vtkImageMapToRGBA_h
