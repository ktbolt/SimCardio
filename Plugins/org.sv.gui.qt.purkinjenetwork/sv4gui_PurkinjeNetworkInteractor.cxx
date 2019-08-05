/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sv4gui_PurkinjeNetworkInteractor.h"
#include "sv4gui_PurkinjeNetworkMeshContainer.h"
#include "sv4gui_PurkinjeNetworkMeshMapper.h"

#include "mitkInteractionPositionEvent.h"
#include "mitkInternalEvent.h"
#include "mitkBaseRenderer.h"
#include "mitkRenderingManager.h"

#include <usModuleRegistry.h>
#include <usGetModuleContext.h>
#include <usModule.h>
#include <usModuleContext.h>

#include <mitkVtkPropRenderer.h>

#include <vtkCellPicker.h>
#include <vtkIdList.h>

#include "sv_polydatasolid_utils.h"

sv4guiPurkinjeNetworkInteractor::sv4guiPurkinjeNetworkInteractor(){
  m_selectedSeed.push_back(-1);
  m_selectedSeed.push_back(-1);
}

sv4guiPurkinjeNetworkInteractor::~sv4guiPurkinjeNetworkInteractor(){

}

void sv4guiPurkinjeNetworkInteractor::ConnectActionsAndFunctions()
{
  MITK_INFO << "connect actions and functions\n";
  CONNECT_FUNCTION("add_start", AddStart);
  CONNECT_FUNCTION("get_position", GetPosition);
  CONNECT_FUNCTION("select_point", SelectPoint);
  CONNECT_FUNCTION("select_single_face",SelectSingleFace);
}

//-------------
// SelectPoint
//-------------
// Process a select point event.

void sv4guiPurkinjeNetworkInteractor::SelectPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
  std::string msgPrefix = "[sv4guiPurkinjeNetworkInteractor::SelectPoint] ";
  MITK_INFO << msgPrefix; 

  // Get the mesh container storing mesh data.
  sv4guiPurkinjeNetworkMeshContainer* meshContainer =
        static_cast< sv4guiPurkinjeNetworkMeshContainer* >( GetDataNode()->GetData() );
  if (meshContainer == NULL) {
      return;
  }

  // Don't select a point if no face is selected.
  if (!meshContainer->HaveSelectedFace()) {
    return;
  }

  // Get the position event.
  const mitk::InteractionPositionEvent* positionEvent = 
    dynamic_cast<const mitk::InteractionPositionEvent*>(interactionEvent);
  if (positionEvent == NULL) {
    return;
  }

  // Get the picked point and set it for the mesh container..
  mitk::Point3D point3d = positionEvent->GetPositionInWorld();
  m_currentPickedPoint = point3d;
  MITK_INFO << msgPrefix << "Picked point " << point3d[0] << " " << point3d[1] << "  " << point3d[2]; 
  meshContainer->SetPickedPoint(point3d);

  // Update all mappers (e.g. sv4guiPurkinjeNetworkMeshMapper).
  interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();

  // Update GUI.
  // meshContainer->InvokeEvent( sv4guiPurkinjeNetworkMeshSelectStartPointFaceEvent() );
  
}

void sv4guiPurkinjeNetworkInteractor::AddStart(mitk::StateMachineAction*, 
    mitk::InteractionEvent* interactionEvent)
{
  //IsOverSeed(interactionEvent);
  sv4guiPurkinjeNetworkMeshContainer* mesh =
        static_cast< sv4guiPurkinjeNetworkMeshContainer* >( GetDataNode()->GetData() );

  if (mesh == NULL) {
      return;
  }

/*
  seeds->addStartSeed((double)m_currentPickedPoint[0],
    (double)m_currentPickedPoint[1],
  (double)m_currentPickedPoint[2]);

  m_currentStartSeed += 1;
  std::cout << m_currentStartSeed << "\n";
*/
  interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();
}

//------------------
// SelectSingleFace
//------------------
// Process a single face select event.
//
// The select event is generated by pressing the 'S' key in the 
// graphics window.
//
// The select event tries to select the face actors (geometry) stored in the 
// sv4guiPurkinjeNetworkMeshMapper object using vtkCellPicker with the current 
// cursor 2D position 'm_CurrentPickedDisplayPoint'.

void sv4guiPurkinjeNetworkInteractor::SelectSingleFace(mitk::StateMachineAction*, 
    mitk::InteractionEvent* interactionEvent)
{
  std::string msgPrefix = "[sv4guiPurkinjeNetworkInteractor::SelectSingleFace] ";
  MITK_INFO << msgPrefix; 

  mitk::VtkPropRenderer* renderer = (mitk::VtkPropRenderer*)interactionEvent->GetSender();
  if (renderer == nullptr) {
    MITK_INFO << msgPrefix << "Renderer is null"; 
  }

  sv4guiPurkinjeNetworkMeshMapper* mapper = dynamic_cast<sv4guiPurkinjeNetworkMeshMapper*>(GetDataNode()->GetMapper(renderer->GetMapperID()));
  if (mapper == nullptr) {
    MITK_INFO << msgPrefix << "Mapper is null"; 
    return;
  }

  // Get the face actors (geometry) we want to select.
  std::vector<vtkSmartPointer<vtkActor>> faceActors = mapper->GetFaceActors(renderer);
  if (faceActors.size() == 0) { 
    MITK_INFO << msgPrefix << "Number of face actore is 0"; 
    return;
  }
  MITK_INFO << msgPrefix << "Number of face actors " << faceActors.size(); 
  auto meshNode = mapper->GetDataNode();
  auto meshContainer = dynamic_cast<sv4guiPurkinjeNetworkMeshContainer*>(meshNode->GetData());

  meshContainer->SetSelectedFaceIndex(-1);
  meshContainer->SetSelectedFaceName("");

  // Create a vtkCellPicker object to determine the closest geomety under the cursor.
  //
  // Attempt to pick the face actors in the pick list and the current cursor
  // position stored in 'm_CurrentPickedDisplayPoint'.
  //
  vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
  
  for (const auto& face : faceActors) {
    //face->GetProperty()->GetColor(rgb);
    cellPicker->AddPickList(face);
  }

  // Execute the pick operation.
  cellPicker->PickFromListOn();
  cellPicker->Pick(m_CurrentPickedDisplayPoint[0], m_CurrentPickedDisplayPoint[1], 0.0, 
      renderer->GetVtkRenderer());
  cellPicker->PickFromListOff();

  // Get the face selected.
  vtkPolyData* selectedFacePolyData = (vtkPolyData*)cellPicker->GetDataSet();
  if (selectedFacePolyData == nullptr) {
    meshContainer->ResetNetworkPoints();
    // Trigger events to unset selected face and point. 
    meshContainer->InvokeEvent( sv4guiPurkinjeNetworkMeshSelectFaceEvent() );
    meshContainer->InvokeEvent( sv4guiPurkinjeNetworkMeshSelectStartPointFaceEvent() );
    return;
  }

  // Determine which face was selected.
  //
  // Faces are matched using pointers to their poly data.
  //
  int selectedFaceIndex = -1;
  meshContainer->SetSelectedFaceIndex(selectedFaceIndex);
  meshContainer->SetSelectedFaceName("");
  meshContainer->ResetNetworkPoints();
  std::vector<sv4guiModelElement::svFace*> modelFaces = meshContainer->GetModelFaces();
  std::vector<vtkSmartPointer<vtkPolyData>> facesPolyData = mapper->GetFacePolyData(renderer);

  for (int i = 0; i < facesPolyData.size(); ++i) {
    auto polyData = facesPolyData[i];
    auto face = modelFaces[i];
    if (polyData == selectedFacePolyData) {
      selectedFaceIndex = i;
      MITK_INFO << msgPrefix << "Select face '" << face->name << "'"; 
      meshContainer->SetSelectedFaceName(face->name);
      meshContainer->SetSelectedFaceIndex(selectedFaceIndex);
      meshContainer->SetSelectedFacePolyData(polyData);
      break;
    }
  }

  // Trigger an event to highlight selected face. 
  meshContainer->InvokeEvent( sv4guiPurkinjeNetworkMeshSelectFaceEvent() );
}

//------------
// GetPosition
//------------
// Process a mouse move event. The current position of the cursor is stored
// in m_CurrentPickedDisplayPoint. 

void sv4guiPurkinjeNetworkInteractor::GetPosition(mitk::StateMachineAction*, 
    mitk::InteractionEvent* interactionEvent)
{
  //MITK_INFO << "[sv4guiPurkinjeNetworkInteractor::GetPosition] ";
  const mitk::InteractionPositionEvent* positionEvent = dynamic_cast<const mitk::InteractionPositionEvent*>(interactionEvent);
  if(positionEvent == NULL) {
    return;
  }
  m_CurrentPickedDisplayPoint = positionEvent->GetPointerPositionOnScreen();
}



