#include <PCH.h>
#include <GameEngine/Components/GreyBoxComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGreyBoxShape, 1)
EZ_ENUM_CONSTANTS(ezGreyBoxShape::Box)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_COMPONENT_TYPE(ezGreyBoxComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Shape", ezGreyBoxShape, m_Shape),
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_ACCESSOR_PROPERTY("SizeNegX", GetSizeNegX, SetSizeNegX),
    EZ_ACCESSOR_PROPERTY("SizePosX", GetSizePosX, SetSizePosX),
    EZ_ACCESSOR_PROPERTY("SizeNegY", GetSizeNegY, SetSizeNegY),
    EZ_ACCESSOR_PROPERTY("SizePosY", GetSizePosY, SetSizePosY),
    EZ_ACCESSOR_PROPERTY("SizeNegZ", GetSizeNegZ, SetSizeNegZ),
    EZ_ACCESSOR_PROPERTY("SizePosZ", GetSizePosZ, SetSizePosZ),
  }
    EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("General"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezGreyBoxComponent::ezGreyBoxComponent()
{
}

ezGreyBoxComponent::~ezGreyBoxComponent()
{
}

void ezGreyBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_Shape;
  s << m_fSizeNegX;
  s << m_fSizePosX;
  s << m_fSizeNegY;
  s << m_fSizePosY;
  s << m_fSizeNegZ;
  s << m_fSizePosZ;
}

void ezGreyBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_Shape;
  s >> m_fSizeNegX;
  s >> m_fSizePosX;
  s >> m_fSizeNegY;
  s >> m_fSizePosY;
  s >> m_fSizeNegZ;
  s >> m_fSizePosZ;
}

ezResult ezGreyBoxComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  GenerateRenderMesh();

  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezGreyBoxComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  GenerateRenderMesh();

  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh);
  const ezDynamicArray<ezMeshResourceDescriptor::SubMesh>& parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = { uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding };
    ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

    ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;

      pRenderData->m_uiPartIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueID() | (uiMaterialIndex << 24);
    }

    // Determine render data category.
    ezRenderData::Category category;
    if (msg.m_OverrideCategory != ezInvalidIndex)
    {
      category = msg.m_OverrideCategory;
    }
    else
    {
      category = ezDefaultRenderDataCategories::LitOpaque;
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
  }
}

void ezGreyBoxComponent::SetShape(ezEnum<ezGreyBoxShape> shape)
{
  m_Shape = shape;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezGreyBoxComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezGreyBoxComponent::SetSizeNegX(float f)
{
  m_fSizeNegX = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosX(float f)
{
  m_fSizePosX = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizeNegY(float f)
{
  m_fSizeNegY = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosY(float f)
{
  m_fSizePosY = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizeNegZ(float f)
{
  m_fSizeNegZ = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::SetSizePosZ(float f)
{
  m_fSizePosZ = f;
  InvalidateMesh();
}

void ezGreyBoxComponent::InvalidateMesh()
{
  if (m_hMesh.IsValid())
  {
    m_hMesh.Invalidate();
    TriggerLocalBoundsUpdate();
  }
}

void ezGreyBoxComponent::GenerateRenderMesh() const
{
  if (m_hMesh.IsValid())
    return;

  ezStringBuilder sResourceName;

  switch (m_Shape)
  {
  case ezGreyBoxShape::Box:
    sResourceName.Format("Grey-Box:{0}-{1},{2}-{3},{4}-{5}", m_fSizeNegX, m_fSizePosX, m_fSizeNegY, m_fSizePosY, m_fSizeNegZ, m_fSizePosZ);
    break;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  m_hMesh = ezResourceManager::GetExistingResource<ezMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  ezGeometry geom;

  if (m_Shape == ezGreyBoxShape::Box)
  {
    ezVec3 size;
    size.x = m_fSizeNegX + m_fSizePosX;
    size.y = m_fSizeNegY + m_fSizePosY;
    size.z = m_fSizeNegZ + m_fSizePosZ;

    ezVec3 offset(0);
    offset.x = (m_fSizePosX - m_fSizeNegX) * 0.5f;
    offset.y = (m_fSizePosY - m_fSizeNegY) * 0.5f;
    offset.z = (m_fSizePosZ - m_fSizeNegZ) * 0.5f;

    ezMat4 t;
    t.SetTranslationMatrix(offset);

    geom.AddTexturedBox(size, ezColor::White, t);
  }

  geom.ComputeFaceNormals();
  geom.TriangulatePolygons();
  geom.ComputeTangents();

  ezMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeGrey.ezMaterialAsset
  desc.SetMaterial(0, "{ 6bd5e7e6-b7be-9801-e032-14226cba1e96 }");

  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
  desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResourceName, desc, sResourceName);
}

//////////////////////////////////////////////////////////////////////////

//EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_ConvertGreyBoxComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_ConvertGreyBoxComponents>)
//EZ_END_DYNAMIC_REFLECTED_TYPE
//
//
//void ezSceneExportModifier_ConvertGreyBoxComponents::ModifyWorld(ezWorld& world)
//{
//  EZ_LOCK(world.GetWriteMarker());
//
//  ezGreyBoxComponentManager* pMan = world.GetComponentManager<ezGreyBoxComponentManager>();
//
//  if (pMan == nullptr)
//    return;
//
//  ezUInt32 num = 0;
//
//  for (auto it = pMan->GetComponents(); it.IsValid(); )
//  {
//    ezGreyBoxComponent* pComp = &(*it);
//    it.Next();
//
//    pMan->DeleteComponent(pComp->GetHandle());
//
//    ++num;
//  }
//}