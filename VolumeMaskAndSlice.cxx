// This example illustrates the masking a vtkImageData for volume rendering and
// slicing it. The sample code applies the mask to the slices as well.

// VTK includes
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCylinder.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRTAnalyticSource.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkMatrix4x4.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>

int main(int, char**)
{
  // Here, we use the RTAnalyticSource to create the vtkImageData
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128,
                          -127, 128,
                          -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  double origin[3], spacing[3];
  int dims[3], extent[6];
  wavelet->GetOutput()->GetOrigin(origin);
  wavelet->GetOutput()->GetSpacing(spacing);
  wavelet->GetOutput()->GetDimensions(dims);
  wavelet->GetOutput()->GetExtent(extent);

  vtkNew<vtkImageData> mask;
  mask->SetExtent(extent);
  mask->SetOrigin(origin);
  mask->SetSpacing(spacing);
  mask->SetDimensions(dims);
  mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  char * ptr = static_cast<char *> (mask->GetScalarPointer(0,0,0));

  double radius = dims[0]/2.0 - 10;

  double center[3];
  for (int i = 0; i < 3; ++i)
    {
    center[i] = origin[i] + spacing[i] * 0.5 * (extent[2*i] + extent[2*i+1]);
    }
  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(mask->GetCenter());
  cylinder->SetRadius(radius);

  for ( int z = 0; z < dims[2]; ++z)
    {
    for ( int y = 0; y < dims[1]; ++y)
      {
      for (int x = 0; x < dims[0]; ++x)
        {
        if (cylinder->vtkImplicitFunction::EvaluateFunction(x,y,z) > 0)
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

  double elements[16] = {
    0, 0,-1, 0,
    1, 0, 0, 0,
    0,-1, 0, 0,
    0, 0, 0, 1};

  vtkNew<vtkMatrix4x4> resliceAxes;
  resliceAxes->DeepCopy(elements);

  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputConnection(wavelet->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetResliceAxesDirectionCosines( 1, 0, 0,
                                           0,-1, 0,
                                           0, 0,-1);
  reslice->SetResliceAxesOrigin(mask->GetCenter());
  reslice->SetInterpolationModeToLinear();
  reslice->Update();

  vtkNew<vtkImageData> reslicedWavelet;
  reslicedWavelet->DeepCopy(reslice->GetOutput());

  reslice->SetInputData(mask.GetPointer());
  reslice->Update();
  vtkNew<vtkImageData> reslicedMask;
  reslicedMask->DeepCopy(reslice->GetOutput());

  vtkNew<vtkImageActor> slice;
  slice->GetMapper()->SetInputData(reslicedWavelet.GetPointer());

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());
  volumeMapper->SetMaskInput(mask.GetPointer());
  volumeMapper->SetMaskTypeToBinary();

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 0.02);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400,400);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->AddActor(slice.GetPointer());
  ren->ResetCamera();

  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
