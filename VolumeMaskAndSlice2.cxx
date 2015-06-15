// This example illustrates the masking of a vtkImageData for volume rendering
// and slicing it using the unstructured grid approach. This approach should be
// preferred when it is required to mask parts of voxels for a smoother edge.
//

// VTK includes
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkClipDataSet.h>
#include <vtkColorTransferFunction.h>
#include <vtkCutter.h>
#include <vtkCylinder.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProjectedTetrahedraMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#include <vtkTransform.h>
#include <vtkPlane.h>
//#include <vtkXMLUnstructuredGridWriter.h>

int main (int, char **)
{
  // Read the volume file from the Data directory next to exe file
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName("Data/Volume.vti");
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

  double radius = (dims[0]/2.0 - 5.0)*spacing[0];

  // Create a cylindrical implicit function centered at the center of the mask
  // and with a custom radius
  vtkNew<vtkTransform> t;
  t->PostMultiply();
  t->Translate(-center[0], -center[1], -center[2]);
  t->RotateX(90);
  t->Translate(center[0], center[1], center[2]);
  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(center);
  cylinder->SetRadius(radius);
  cylinder->SetTransform(t.GetPointer());

  // Clip the data with the cylinder function
  vtkNew<vtkClipDataSet> clipData;
  clipData->SetInputConnection(reader->GetOutputPort());
  clipData->SetClipFunction(cylinder.GetPointer());
  clipData->InsideOutOn();

//  vtkNew<vtkXMLUnstructuredGridWriter> w;
//  w->SetFileName("c.vtu");
//  w->SetInputConnection(clipData->GetOutputPort());
//  w->Write();

  // Tetrahedralize the clipped unstructured grid
  vtkNew<vtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(clipData->GetOutputPort());

//  w->SetFileName("tt.vtu");
//  w->SetInputConnection(tetrahedralize->GetOutputPort());
//  w->Write();

  // Create the volume mapper
  vtkNew<vtkProjectedTetrahedraMapper> clippedVolumeMapper;
  clippedVolumeMapper->SetInputConnection(tetrahedralize->GetOutputPort());
  vtkNew<vtkSmartVolumeMapper> originalVolumeMapper;
  originalVolumeMapper->SetInputConnection(reader->GetOutputPort());

  // Create color transfer function
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(1096.0, 0.7, 0.015, 0.15);
  ctf->AddRGBPoint(2777, 0.86, 0.86, 0.86);
  ctf->AddRGBPoint(4458, 0.23, 0.3, 0.75);

  // Scalar opacity function
  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(1096, 0.0);
  pwf->AddPoint(4458, 1.0);

  // Create the volume property
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());
  volumeProperty->SetScalarOpacityUnitDistance(3.87);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOff();

  // Create the volume
  vtkNew<vtkVolume> clippedVolume;
  clippedVolume->SetMapper(clippedVolumeMapper.GetPointer());
  clippedVolume->SetProperty(volumeProperty.GetPointer());

  // Create the outline for the volumes
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outline->GetOutputPort());
  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(outlineMapper.GetPointer());

  // Now implement the slicing pipeline
  // Set up slice plane
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetNormal(0, 0, -1);
  slicePlane->SetOrigin(18.5, 17.5, 69.3);

  vtkNew<vtkCutter> cutter;
  cutter->SetInputConnection(clipData->GetOutputPort());
  cutter->SetCutFunction(slicePlane.GetPointer());

  vtkNew<vtkPolyDataMapper> sliceMapper;
  sliceMapper->SetInputConnection(cutter->GetOutputPort());
  sliceMapper->SetLookupTable(ctf.GetPointer());

  vtkNew<vtkActor> slice;
  slice->SetMapper(sliceMapper.GetPointer());

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

  ren1->AddVolume(clippedVolume.GetPointer());
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
