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
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMathematics.h>
#include <vtkImageReslice.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include <vtkImageCanvasSource2D.h>
#include <vtkImageBlend.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkAxesActor.h>
#include <vtkPNGWriter.h>

int main(int, char**)
{
  // Read the Volume file from the Data directory next to exe file
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName("Data/CTHead.vti");
  reader->Update();

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

  double radius = dims[0]/2.0 - 50;

  // Create a cylindrical implicit function centered at the center of the mask
  // and with a custom radius
  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(center);
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
  reslice->SetResliceAxesDirectionCosines( 0,-1, 0,
                                           0, 0,-1,
                                          -1, 0,0);
  reslice->SetResliceAxesOrigin(center);
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
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetMaskInput(mask.GetPointer());
  volumeMapper->SetMaskTypeToBinary();

  // Create color transfer function
  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.31, 0.34, 0.43);
  ctf->AddRGBPoint(556.24, 0, 0.0, 1);
  ctf->AddRGBPoint(1112.48, 0, 1, 1);
  ctf->AddRGBPoint(1636, 0, 1, 0);
  ctf->AddRGBPoint(2192.24, 1, 1, 0);
  ctf->AddRGBPoint(2748.48, 1, 0, 0);
  ctf->AddRGBPoint(3272, 0.88, 0, 1);

  // Scalar opacity function
  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(3272, 1);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  vtkNew<vtkVolume> originalVolume;
  originalVolume->SetMapper(originalVolumeMapper.GetPointer());
  originalVolume->SetProperty(volumeProperty.GetPointer());
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Use the same color function for slice
  vtkNew<vtkImageMapToColors> lut;
  lut->SetInputConnection(imMath->GetOutputPort());
  lut->SetLookupTable(ctf.GetPointer());
  lut->Update();
  vtkNew<vtkImageMapToColors> originalLut;
  originalLut->SetInputData(reslicedVolume.GetPointer());
  originalLut->SetLookupTable(ctf.GetPointer());

  // Image Canvas for drawing circles on top of slices
  int slice_extent[3];
  reslicedVolume->GetDimensions(slice_extent);

  int circle1_center[2], circle2_center[2];
  int circle_radius = 20;
  for (int i = 0; i < 2; ++i)
    {
    circle1_center[i] = (/*slice_extent[2*i+1] +*/ slice_extent[i])/3.0;
    circle2_center[i] = (/*slice_extent[2*i+1] +*/ slice_extent[i])*2.0/3.0;
    }
  std::cout << slice_extent[0] << " " << slice_extent[1] << " " <<
    slice_extent[2] << " " << slice_extent[3] << std::endl;
  std::cout << circle2_center[0] << " " << circle2_center[1] << std::endl;

  vtkNew<vtkImageCanvasSource2D> drawing;
  drawing->SetNumberOfScalarComponents(
    lut->GetOutput()->GetNumberOfScalarComponents());
  drawing->SetScalarType(lut->GetOutput()->GetScalarType());
  drawing->SetExtent(0, 255,
                     0, 255,
                     1,1);
  drawing->DrawImage(0,0, lut->GetOutput());
//  drawing->SetDrawColor(0.0, 0.0, 0.0);
//  drawing->FillBox(slice_extent[0],
//                   slice_extent[1],
//                   slice_extent[2],
//                   slice_extent[3]);
  drawing->SetDrawColor(255.0, 255.0, 255.0, 255.0);
  drawing->DrawCircle(circle1_center[0], circle1_center[1], circle_radius);
  drawing->DrawCircle(circle2_center[0], circle2_center[1], circle_radius);

  vtkNew<vtkImageBlend> blend;
  blend->AddInputConnection(lut->GetOutputPort());
  blend->AddInputConnection(drawing->GetOutputPort());
  blend->SetOpacity(0, 0.5);
  blend->SetOpacity(1, 1.0);

  vtkNew<vtkImageActor> slice;
  slice->GetMapper()->SetInputConnection(drawing->GetOutputPort());
  vtkNew<vtkImageActor> originalSlice;
  originalSlice->GetMapper()->SetInputConnection(originalLut->GetOutputPort());

  vtkNew<vtkPNGWriter> writer;
  writer->SetInputConnection(drawing->GetOutputPort());
  writer->SetFileName("drawing.png");
  writer->Write();

  writer->SetInputConnection(lut->GetOutputPort());
  writer->SetFileName("lut.png");
  writer->Write();

  writer->SetInputConnection(imMath->GetOutputPort());
  writer->SetFileName("imMath.png");
  writer->Write();

  // Create an outline for the volume
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper.GetPointer());

  // Render
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600,600);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0,0.5,0.5,1);
  ren1->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren1.GetPointer());
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5,0.5,1,1);
  ren2->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren2.GetPointer());
  vtkNew<vtkRenderer> ren3;
  ren3->SetViewport(0, 0, 0.5, 0.5);
  ren3->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren3.GetPointer());
  vtkNew<vtkRenderer> ren4;
  ren4->SetViewport(0.5,0,1,0.5);
  //ren4->SetBackground(0.31,0.34,0.43);
  renWin->AddRenderer(ren4.GetPointer());

  ren1->AddVolume(originalVolume.GetPointer());
  ren1->AddActor(outlineActor.GetPointer());
  ren1->ResetCamera();
  ren2->AddVolume(volume.GetPointer());
  ren2->AddActor(outlineActor.GetPointer());
  ren2->ResetCamera();
  ren1->SetActiveCamera(ren2->GetActiveCamera());
  ren3->AddActor(originalSlice.GetPointer());
  ren3->ResetCamera();
  ren4->AddActor(slice.GetPointer());
  ren4->ResetCamera();
  ren4->SetActiveCamera(ren3->GetActiveCamera());

  // Orientation marker
  vtkNew<vtkAxesActor> axes;
  vtkNew<vtkOrientationMarkerWidget> widget;
  widget->SetOrientationMarker(axes.GetPointer());
  widget->SetInteractor(iren.GetPointer());
  widget->SetEnabled(1);
  widget->InteractiveOn();

  renWin->Render();
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
