// This example illustrates the masking a vtkImageData for volume rendering and
// slicing it. The sample code applies the mask to the slices as well.

// VTK includes
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCylinder.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageActor.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMathematics.h>
#include <vtkImageProperty.h>
#include <vtkImageReslice.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTriangleFilter.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include "vtkImageMapToRGBA.h"

int main(int, char**)
{
  // Read the Volume file from the Data directory next to exe file
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName("Data/Volume.vti");
  reader->Update();

  vtkNew<vtkTriangleFilter> tf;
  tf->SetInputConnection(reader->GetOutputPort());

  // Fetch volume parameters
  double origin[3], spacing[3];
  int dims[3], extent[6];
  reader->GetOutput()->GetOrigin(origin);
  reader->GetOutput()->GetSpacing(spacing);
  reader->GetOutput()->GetDimensions(dims);
  reader->GetOutput()->GetExtent(extent);

  // Calculate center of volume for cylindrical mask center
  double center[3];
  for (int i = 0; i < 3; ++i)
    {
    center[i] = origin[i] + spacing[i] * 0.5 * (extent[2*i] + extent[2*i+1]);
    }

  // Create a mask image data with the same parameters as the volume
  vtkNew<vtkImageData> mask;
  mask->SetDimensions(dims);
  mask->SetOrigin(extent[0], extent[2], extent[4]);
  mask->SetSpacing(spacing);
  mask->SetExtent(extent);
  mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  char * ptr = static_cast<char *> (mask->GetScalarPointer(0,0,0));

  double radius = dims[0]/2.0 - 5.0;

  // Create a cylindrical implicit function centered at the center of the mask
  // and with a custom radius
  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(center[0], center[2], center[1]);
  cylinder->SetRadius(radius);

  // Set all values of the mask within and on the cylinder = 255 and all other
  // values = 0.
  // NOTE: We set 255 since that is the requirement for the GPU
  // volume mapper binary mask.
  for ( int z = 0; z < dims[2]; ++z)
    {
    for ( int y = 0; y < dims[1]; ++y)
      {
      for (int x = 0; x < dims[0]; ++x)
        {
        if (cylinder->vtkImplicitFunction::EvaluateFunction(x,z,y) > 0)
          {
          // point is outide cylinder
          *ptr++ = 0;
          }
        else
          {
          *ptr++ = 255;
          }
        }
      }
    }

  // Create a reslice filter with center at origin and slice as sagittal plane
  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetResliceAxesDirectionCosines( 1,0, 0,
                                           0,1,0,
                                           0,0,-1);
  reslice->SetResliceAxesOrigin(18.5, 17.5, 69.3);
  reslice->SetInterpolationModeToLinear();
  reslice->Update();

  // Slice the volume
  vtkNew<vtkImageData> reslicedVolume;
  reslicedVolume->DeepCopy(reslice->GetOutput());

  // Slice the mask
  reslice->SetInputData(mask.GetPointer());
  reslice->Update();
  vtkNew<vtkImageData> reslicedMask;
  reslicedMask->DeepCopy(reslice->GetOutput());

  // Scale the mask to have values either 1 or 0 and the same scalar type as the
  // volume slice. This is necessary to mask slice values
  vtkNew<vtkImageShiftScale> shiftScale;
  shiftScale->SetInputData(reslicedMask.GetPointer());
  shiftScale->SetShift(0.0);
  shiftScale->SetScale(1/255.0);
  shiftScale->SetOutputScalarType(reslicedVolume->GetScalarType());
  shiftScale->Update();

  // Multiply the volume slice with the scaled mask slice. The result of this
  // operation is a masked volume slice.
  vtkNew<vtkImageMathematics> imMath;
  imMath->SetInput1Data(reslicedVolume.GetPointer());
  imMath->SetInput2Data(shiftScale->GetOutput());
  imMath->SetOperationToMultiply();

  // Create the GPU mapper and set the mask on it
  vtkNew<vtkGPUVolumeRayCastMapper> originalVolumeMapper;
  originalVolumeMapper->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  //volumeMapper->SetInputData(mask.GetPointer());
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetMaskInput(mask.GetPointer());
  volumeMapper->SetMaskTypeToBinary();

  // Create color transfer function
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.0, 1.0, 0.0);
  ctf->AddRGBPoint(255.0, 0.0, 1.0, 1.0);
  ctf->AddRGBPoint(1096.0, 0.7, 0.015, 0.15);
  ctf->AddRGBPoint(2777, 0.86, 0.86, 0.86);
  ctf->AddRGBPoint(4458, 0.23, 0.3, 0.75);

  // Scalar opacity function
  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(255.0, 1.0);
  pwf->AddPoint(1096.0, 0.0);
  pwf->AddPoint(4458.0, 1.0);

  // Create the volume property
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());
  volumeProperty->SetScalarOpacityUnitDistance(3.87);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOff();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Use the same color function for slice
  // but a new opacity function to mask background
  vtkNew<vtkPiecewiseFunction> pwf1;
  pwf1->AddPoint(1096.0, 0.0);
  pwf1->AddPoint(3900.0, 0.0);
  pwf1->AddPoint(3900.0, 1.0);
  pwf1->AddPoint(4458.0, 1.0);

  vtkNew<vtkImageMapToRGBA> imageMapToRGBA;
  imageMapToRGBA->SetInputConnection(imMath->GetOutputPort());
  imageMapToRGBA->SetColorFunction(ctf.GetPointer());
  imageMapToRGBA->SetOpacityFunction(pwf1.GetPointer());

  vtkNew<vtkImageProperty> imProp;
  imProp->SetInterpolationTypeToNearest();
  vtkNew<vtkImageActor> slice;
  slice->GetMapper()->SetInputConnection(imageMapToRGBA->GetOutputPort());
  slice->SetProperty(imProp.GetPointer());

  // Create an outline for the volume
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper.GetPointer());

  // Render
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(800,400);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0,0,0.5,1);
  renWin->AddRenderer(ren1.GetPointer());
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5,0,1,1);
  renWin->AddRenderer(ren2.GetPointer());

  ren1->AddVolume(volume.GetPointer());
  ren1->AddActor(outlineActor.GetPointer());
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Azimuth(5);
  ren1->GetActiveCamera()->Zoom(5);
  ren2->AddActor(slice.GetPointer());
  ren2->ResetCamera();

  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
