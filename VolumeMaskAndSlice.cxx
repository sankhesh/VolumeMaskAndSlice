// This example illustrates the masking a vtkImageData for volume rendering and
// slicing it. The sample code applies the mask to the slices as well.

// VTK includes
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCylinder.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageActor.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMathematics.h>
#include <vtkImageReslice.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

int main(int, char**)
{
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName("Data/CTHead.vti");
  reader->Update();

  double origin[3], spacing[3];
  int dims[3], extent[6];
  reader->GetOutput()->GetOrigin(origin);
  reader->GetOutput()->GetSpacing(spacing);
  reader->GetOutput()->GetDimensions(dims);
  reader->GetOutput()->GetExtent(extent);

  double center[3];
  for (int i = 0; i < 3; ++i)
    {
    center[i] = origin[i] + spacing[i] * 0.5 * (extent[2*i] + extent[2*i+1]);
    }

  vtkNew<vtkImageData> mask;
  mask->SetDimensions(dims);
  mask->SetOrigin(extent[0], extent[2], extent[4]);
  mask->SetSpacing(spacing);
  mask->SetExtent(extent);
  mask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  char * ptr = static_cast<char *> (mask->GetScalarPointer(0,0,0));

  double radius = dims[0]/2.0 - 50;

  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(center);
  cylinder->SetRadius(radius);

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

  double elements[16] = {
    0, 0,-1, 0,
    1, 0, 0, 0,
    0,-1, 0, 0,
    0, 0, 0, 1};

  vtkNew<vtkMatrix4x4> resliceAxes;
  resliceAxes->DeepCopy(elements);

  vtkNew<vtkImageReslice> reslice;
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetResliceAxesDirectionCosines( 0,-1, 0,
                                           0, 0,-1,
                                          -1, 0,0);
  reslice->SetResliceAxesOrigin(center);
  reslice->SetInterpolationModeToLinear();
  reslice->Update();

  vtkNew<vtkImageData> reslicedVolume;
  reslicedVolume->DeepCopy(reslice->GetOutput());

  reslice->SetInputData(mask.GetPointer());
  reslice->SetResliceAxesOrigin(center);
  reslice->Update();
  vtkNew<vtkImageData> reslicedMask;
  reslicedMask->DeepCopy(reslice->GetOutput());

  vtkNew<vtkImageShiftScale> shiftScale;
  shiftScale->SetInputData(reslicedMask.GetPointer());
  shiftScale->SetShift(0.0);
  shiftScale->SetScale(1/255.0);
  shiftScale->SetOutputScalarType(reslicedVolume->GetScalarType());
  shiftScale->Update();

  vtkNew<vtkImageMathematics> imMath;
  imMath->SetInput1Data(reslicedVolume.GetPointer());
  imMath->SetInput2Data(shiftScale->GetOutput());
  imMath->SetOperationToMultiply();

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetMaskInput(mask.GetPointer());
  volumeMapper->SetMaskTypeToBinary();

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.31, 0.34, 0.43);
  ctf->AddRGBPoint(556.24, 0, 0.0, 1);
  ctf->AddRGBPoint(1112.48, 0, 1, 1);
  ctf->AddRGBPoint(1636, 0, 1, 0);
  ctf->AddRGBPoint(2192.24, 1, 1, 0);
  ctf->AddRGBPoint(2748.48, 1, 0, 0);
  ctf->AddRGBPoint(3272, 0.88, 0, 1);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(3272, 1);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkImageMapToColors> lut;
  lut->SetInputConnection(imMath->GetOutputPort());
  lut->SetLookupTable(ctf.GetPointer());

  vtkNew<vtkImageActor> slice;
  slice->GetMapper()->SetInputConnection(lut->GetOutputPort());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600,600);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0,0,0.5,1);
  ren1->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren1.GetPointer());
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5,0,1,1);
  ren2->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren2.GetPointer());

  ren1->AddVolume(volume.GetPointer());
  ren1->ResetCamera();
  ren2->AddActor(slice.GetPointer());
  ren2->ResetCamera();

  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
