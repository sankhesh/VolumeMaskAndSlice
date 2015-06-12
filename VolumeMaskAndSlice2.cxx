// This example illustrates the masking of a vtkImageData for volume rendering
// and slicing it using the unstructured grid approach. This approach should be
// preferred when it is required to mask parts of voxels for a smoother edge.
//

// VTK includes
#include <vtkXMLImageDataReader.h>
#include <vtkCylinder.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>
#include <vtkClipDataSet.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>

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

  double radius = dims[0]/2.0 - 50;

  // Create a cylindrical implicit function centered at the center of the mask
  // and with a custom radius
  vtkNew<vtkCylinder> cylinder;
  cylinder->SetCenter(center);
  cylinder->SetRadius(radius);

  vtkNew<vtkClipDataSet> clipData;
  clipData->SetInputConnection(reader->GetOutputPort());
  clipData->SetClipFunction(cylinder.GetPointer());
}
