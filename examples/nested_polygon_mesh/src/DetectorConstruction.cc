#include <G4VPhysicalVolume.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4Box.hh>
#include <G4SystemOfUnits.hh>
#include <G4LogicalVolume.hh>
#include <G4VisAttributes.hh>
#include <G4PVPlacement.hh>
#include <G4ThreeVector.hh>
#include <G4TessellatedSolid.hh>
#include <G4Colour.hh>
#include <G4RotationMatrix.hh>

#include "cadmesh.hh"

#include "DetectorConstruction.hh"


DetectorConstruction::DetectorConstruction()
  : fWorldSolid(nullptr), fWorldLogical(nullptr), fWorldPhysical(nullptr),
    fTopLevelLogical(nullptr), fTopLevelPhysical(nullptr), fPath("")
{
}

DetectorConstruction::~DetectorConstruction()
{
  // deletion of member data through Geant4's object stores
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4NistManager* nistManager = G4NistManager::Instance();
  G4Material* air = nistManager->FindOrBuildMaterial("G4_AIR");
  G4Material* adiposeTissue = nistManager->FindOrBuildMaterial("G4_ADIPOSE_TISSUE_ICRP");
  G4Material* heartMuscle = nistManager->FindOrBuildMaterial("G4_MUSCLE_STRIATED_ICRU");

  fWorldSolid = new G4Box("world_solid", 50*cm, 50*cm, 50*cm);
  fWorldLogical = new G4LogicalVolume(fWorldSolid, air, "world_logical");
  fWorldLogical->SetVisAttributes(G4VisAttributes::Invisible);  

  fWorldPhysical = new G4PVPlacement(nullptr,           // rotation
                                     G4ThreeVector(),   // translation
                                     fWorldLogical,     // current logical
                                     "world_physical",  // name 
                                     nullptr,           // mother volume
                                     false,
                                     0);                // copy number
    
  //**************************************************************************************
  //    CADMesh Part
  //**************************************************************************************

  // Set verbosity of CADMesh's log output, i.e. from the run managers verbosity.
  cadmesh::SetVerbosityLevel(cadmesh::DEBUG);

  // surface attributes as used in the input file
  constexpr G4int SKIN = 1;
  constexpr G4int HEART = 0;

  // region attributes
  cadmesh::RegionAttributeMap regionAttributeMap;
  regionAttributeMap[SKIN] = {G4Colour::White(), adiposeTissue};
  regionAttributeMap[HEART] = {G4Colour::Red(), heartMuscle};

  // surface hierarchy
  cadmesh::SurfaceNode skinNode(SKIN);
  {
    cadmesh::SurfaceNode heartNode(HEART);
    skinNode.children.push_back(heartNode);
  }
  
  // set up reader instance
  cadmesh::PolygonMeshFileReader fileReader;
  fileReader.SetUnitOfLength(mm);
  fileReader.SetPermuteFacetPoints(false);
  
  // parse CAD file and retrieve all meshes contained in that file
  std::vector<cadmesh::PolygonMesh> meshCollection = fileReader.ReadMeshCollection(fPath);
  
  // establish hierarchy of nested meshes
  fTopLevelLogical = cadmesh::NestMeshes(meshCollection, regionAttributeMap, skinNode);
  
  //**************************************************************************************

  // placement of nested meshes
  G4RotationMatrix* rotation = new G4RotationMatrix();
  rotation->rotateX(90*deg);
  G4ThreeVector translation = G4ThreeVector(0, 0, 0);
    
  fTopLevelPhysical = new G4PVPlacement(rotation,           // rotation  
                                        translation,        // translation
                                        fTopLevelLogical,   // current logical
                                        "skin_physical",    // name
                                        fWorldLogical,      // mother volume
                                        false,
                                        0);                 // copy number

  return fWorldPhysical;
}
