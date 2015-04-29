// This example illustrates the masking a vtkImageData for volume rendering and
// slicing it. The sample code applies the mask to the slices as well.

// VTK includes
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRTAnalyticSource.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkCylinder.h>

int main(int, char**)
{
  // Here, we use the RTAnalyticSource to create the vtkImageData
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128,
                          -127, 128,
                          -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  int dims[3];
  wavelet->GetOutput()->GetDimensions(dims);

  vtkNew<vtkImageData> mask;
  mask->SetExtent(wavelet->GetOutput()->GetExtent());
  mask->SetOrigin(wavelet->GetOutput()->GetOrigin());
  mask->SetSpacing(wavelet->GetOutput()->GetSpacing());
  mask->SetDimensions(dims);
  mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  char * ptr = static_cast<char *> (mask->GetScalarPointer(0,0,0));

  double radius = dims[0]/2.0 - 10;

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
          *ptr++ = 255;
          }
        else
          {
          *ptr++ = 0;
          }
        }
      }
    }


  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());
  volumeMapper->SetMaskInput(mask.GetPointer());

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
  ren->ResetCamera();

  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
