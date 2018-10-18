#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Textures/Texture2DResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeResource, 1, ezRTTIDefaultAllocator<ezKrautTreeResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeResource::ezKrautTreeResource()
    : ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire KrautTree
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezKrautTreeResource::UpdateContent(ezStreamReader* Stream)
{
  ezKrautTreeResourceDescriptor desc;
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(desc);
}

void ezKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezKrautTreeResource::CreateResource(const ezKrautTreeResourceDescriptor& desc)
{
  m_TreeLODs.Clear();
  m_Bounds = desc.m_Bounds;

  ezHybridArray<ezMaterialResourceHandle, 16> allMaterials;

  ezStringBuilder sMatName;

  for (const auto& mat : desc.m_Materials)
  {
    ezUInt32 uiTexHash = mat.m_uiMaterialType;
    uiTexHash = ezHashing::xxHash32(&mat.m_VariationColor, sizeof(mat.m_VariationColor), uiTexHash);
    uiTexHash = ezHashing::xxHash32(mat.m_sDiffuseTexture.GetData(), mat.m_sDiffuseTexture.GetElementCount(), uiTexHash);
    uiTexHash = ezHashing::xxHash32(mat.m_sNormalMapTexture.GetData(), mat.m_sNormalMapTexture.GetElementCount(), uiTexHash);

    sMatName.Format("KrautMaterial_{0}", uiTexHash);

    auto hMaterial = ezResourceManager::GetExistingResource<ezMaterialResource>(sMatName);

    if (!hMaterial.IsValid())
    {
      ezMaterialResourceDescriptor md;

      switch (mat.m_uiMaterialType)
      {
        case 0:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Branch.ezMaterial");
          break;

        case 1:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Frond.ezMaterial");
          break;

        case 2:
          md.m_hBaseMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Kraut/Leaf.ezMaterial");
          break;
      }

      auto& m1 = md.m_Texture2DBindings.ExpandAndGetRef();
      m1.m_Name.Assign("BaseTexture");
      m1.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(mat.m_sDiffuseTexture);

      auto& m2 = md.m_Texture2DBindings.ExpandAndGetRef();
      m2.m_Name.Assign("NormalTexture");
      m2.m_Value = ezResourceManager::LoadResource<ezTexture2DResource>(mat.m_sNormalMapTexture);

      hMaterial = ezResourceManager::CreateResource<ezMaterialResource>(sMatName, md, mat.m_sDiffuseTexture);
    }

    allMaterials.PushBack(hMaterial);
  }

  ezStringBuilder sResName, sResDesc;

  for (ezUInt32 lodIdx = 0; lodIdx < desc.m_Lods.GetCount(); ++lodIdx)
  {
    const auto& lodSrc = desc.m_Lods[lodIdx];
    auto& lodDst = m_TreeLODs.ExpandAndGetRef();

    lodDst.m_fMinLodDistance = lodSrc.m_fMinLodDistance;
    lodDst.m_fMaxLodDistance = lodSrc.m_fMaxLodDistance;

    ezMeshResourceDescriptor md;
    auto& buffer = md.MeshBufferDesc();

    const ezUInt32 uiNumVertices = lodSrc.m_Vertices.GetCount();
    const ezUInt32 uiNumTriangles = lodSrc.m_Triangles.GetCount();
    const ezUInt32 uiSubMeshes = lodSrc.m_SubMeshes.GetCount();

    buffer.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::XYZFloat);
    buffer.AddStream(ezGALVertexAttributeSemantic::Color, ezGALResourceFormat::RGBAUByteNormalized);
    buffer.AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (ezUInt32 v = 0; v < uiNumVertices; ++v)
    {
      const auto& vtx = lodSrc.m_Vertices[v];

      buffer.SetVertexData<ezVec3>(0, v, vtx.m_vPosition);
      buffer.SetVertexData<ezVec2>(1, v, ezVec2(vtx.m_vTexCoord.x, vtx.m_vTexCoord.y));
      buffer.SetVertexData<ezVec2>(2, v, ezVec2(vtx.m_vTexCoord.z, 1.0f / vtx.m_vTexCoord.z));
      buffer.SetVertexData<ezVec3>(3, v, vtx.m_vNormal);
      buffer.SetVertexData<ezVec3>(4, v, vtx.m_vTangent);
      buffer.SetVertexData<ezColorGammaUB>(5, v, vtx.m_VariationColor);
    }

    for (ezUInt32 t = 0; t < uiNumTriangles; ++t)
    {
      const auto& tri = lodSrc.m_Triangles[t];

      buffer.SetTriangleIndices(t, tri.m_uiVertexIndex[0], tri.m_uiVertexIndex[1], tri.m_uiVertexIndex[2]);
    }

    for (ezUInt32 sm = 0; sm < uiSubMeshes; ++sm)
    {
      const auto& subMesh = lodSrc.m_SubMeshes[sm];

      md.AddSubMesh(subMesh.m_uiNumTriangles, subMesh.m_uiFirstTriangle, subMesh.m_uiMaterialIndex);
    }

    md.ComputeBounds();

    for (ezUInt32 mat = 0; mat < desc.m_Materials.GetCount(); ++mat)
    {
      md.SetMaterial(mat, allMaterials[mat].GetResourceID());
    }

    sResName.Format("{0}_{1}_LOD{2}", GetResourceID(), GetCurrentResourceChangeCounter(), lodIdx);
    sResDesc.Format("{0}_{1}_LOD{2}", GetResourceDescription(), GetCurrentResourceChangeCounter(), lodIdx);

    lodDst.m_hMesh = ezResourceManager::CreateResource<ezMeshResource>(sResName, md, sResDesc);
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeResourceDescriptor::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 4;

  stream << uiVersion;

  stream << m_Bounds;

  const ezUInt8 uiNumLods = m_Lods.GetCount();
  stream << uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    const auto& lod = m_Lods[lodIdx];

    stream << lod.m_fMinLodDistance;
    stream << lod.m_fMaxLodDistance;
    stream << lod.m_Vertices.GetCount();
    stream << lod.m_Triangles.GetCount();
    stream << lod.m_SubMeshes.GetCount();

    for (const auto& vtx : lod.m_Vertices)
    {
      stream << vtx.m_vPosition;
      stream << vtx.m_vTexCoord;
      stream << vtx.m_vNormal;
      stream << vtx.m_vTangent;
      stream << vtx.m_VariationColor;
    }

    for (const auto& tri : lod.m_Triangles)
    {
      stream << tri.m_uiVertexIndex[0];
      stream << tri.m_uiVertexIndex[1];
      stream << tri.m_uiVertexIndex[2];
    }

    for (const auto& sm : lod.m_SubMeshes)
    {
      stream << sm.m_uiFirstTriangle;
      stream << sm.m_uiNumTriangles;
      stream << sm.m_uiMaterialIndex;
    }
  }

  const ezUInt8 uiNumMats = m_Materials.GetCount();
  stream << uiNumMats;

  for (const auto& mat : m_Materials)
  {
    stream << mat.m_uiMaterialType;
    stream << mat.m_sDiffuseTexture;
    stream << mat.m_sNormalMapTexture;
    stream << mat.m_VariationColor;
  }
}

ezResult ezKrautTreeResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;

  if (uiVersion != 4)
    return EZ_FAILURE;

  stream >> m_Bounds;

  ezUInt8 uiNumLods = 0;
  stream >> uiNumLods;

  for (ezUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    auto& lod = m_Lods.ExpandAndGetRef();

    stream >> lod.m_fMinLodDistance;
    stream >> lod.m_fMaxLodDistance;

    ezUInt32 numVertices, numTriangles, numSubMeshes;

    stream >> numVertices;
    stream >> numTriangles;
    stream >> numSubMeshes;

    lod.m_Vertices.SetCountUninitialized(numVertices);
    lod.m_Triangles.SetCountUninitialized(numTriangles);
    lod.m_SubMeshes.SetCount(numSubMeshes); // initialize this one because of the material handle

    for (auto& vtx : lod.m_Vertices)
    {
      stream >> vtx.m_vPosition;
      stream >> vtx.m_vTexCoord;
      stream >> vtx.m_vNormal;
      stream >> vtx.m_vTangent;
      stream >> vtx.m_VariationColor;
    }

    for (auto& tri : lod.m_Triangles)
    {
      stream >> tri.m_uiVertexIndex[0];
      stream >> tri.m_uiVertexIndex[1];
      stream >> tri.m_uiVertexIndex[2];
    }

    for (auto& sm : lod.m_SubMeshes)
    {
      stream >> sm.m_uiFirstTriangle;
      stream >> sm.m_uiNumTriangles;
      stream >> sm.m_uiMaterialIndex;
    }
  }

  ezUInt8 uiNumMats = 0;
  ;
  stream >> uiNumMats;
  m_Materials.SetCount(uiNumMats);

  for (auto& mat : m_Materials)
  {
    stream >> mat.m_uiMaterialType;
    stream >> mat.m_sDiffuseTexture;
    stream >> mat.m_sNormalMapTexture;
    stream >> mat.m_VariationColor;
  }

  return EZ_SUCCESS;
}
