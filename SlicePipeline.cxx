// This example illustrates the slicing and masking of a volume
// using a combination of the following VTK algorithms:
//
// vtkCutter: To slice the volume
//
// vtkCylinder: Implicit function used to clip the volume
//
// vtkClipDataSet: Clip algorithm that clips the slice to the shape of the
// implicit function.


// VTK includes
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkClipDataSet.h>
#include <vtkColorTransferFunction.h>
#include <vtkCutter.h>
#include <vtkCylinder.h>
#include <vtkDataSetMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkXMLImageDataReader.h>

int main(int, char**)
{
  // Read the volume file from the Data directory next to exe file
  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName("Data/Volume.vti");
  reader->Update();

  // Fetch the data parameters
  double origin[3], spacing[3];
  int dims[3], extent[6];
  reader->GetOutput()->GetOrigin(origin);
  reader->GetOutput()->GetSpacing(spacing);
  reader->GetOutput()->GetDimensions(dims);
  reader->GetOutput()->GetExtent(extent);

  // Calculate center of data for cylindrical mask center
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

  // Set up slice plane
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetNormal(0, 0, -1);
  slicePlane->SetOrigin(18.5, 17.5, 69.3);

  // Slice the volume
  vtkNew<vtkCutter> cutter;
  cutter->SetInputConnection(reader->GetOutputPort());
  cutter->SetCutFunction(slicePlane.GetPointer());

  // Clip the slice with the cylindrical function
  vtkNew<vtkClipDataSet> clipData;
  clipData->SetInputConnection(cutter->GetOutputPort());
  clipData->SetClipFunction(cylinder.GetPointer());
  clipData->InsideOutOn();

  // Create color transfer function
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(1096.0, 0.7, 0.015, 0.15);
  ctf->AddRGBPoint(2777, 0.86, 0.86, 0.86);
  ctf->AddRGBPoint(4458, 0.23, 0.3, 0.75);

  // Setup the slice mapper with the color transfer function
  // NOTE: No need to set opacity here
  vtkNew<vtkDataSetMapper> sliceMapper;
  sliceMapper->SetInputConnection(clipData->GetOutputPort());
  sliceMapper->SetLookupTable(ctf.GetPointer());

  vtkNew<vtkActor> slice;
  slice->SetMapper(sliceMapper.GetPointer());

  // Setup the render
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(slice.GetPointer());
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(500, 500);
  renderWindow->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());
  iren->Initialize();

  renderWindow->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
