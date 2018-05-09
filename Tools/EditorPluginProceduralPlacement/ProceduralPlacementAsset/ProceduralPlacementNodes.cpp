#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementNodes.h>

namespace
{
  ezExpressionAST::Node* CreateRandom(float fSeed, ezExpressionAST& out_Ast)
  {
    auto pPointIndex = out_Ast.CreateInput(ezPPInternal::ExpressionInputs::PointIndex);
    auto pSeedConstant = out_Ast.CreateConstant(fSeed);

    auto pSeed = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Add, pPointIndex, pSeedConstant);

    auto pFunctionCall = out_Ast.CreateFunctionCall("Random");
    pFunctionCall->m_Arguments.PushBack(pSeed);

    return pFunctionCall;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementNodeBase, 1, ezRTTINoAllocator)
{
  flags.Add(ezTypeFlags::Abstract);
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementLayerOutput, 1, ezRTTIDefaultAllocator<ezProceduralPlacementLayerOutput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new ezAssetBrowserAttribute("Prefab")),
    EZ_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    EZ_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    EZ_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
    EZ_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),

    EZ_MEMBER_PROPERTY("Density", m_DensityPin),
    EZ_MEMBER_PROPERTY("Scale", m_ScalePin),
    EZ_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin),
    EZ_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezExpressionAST::Node* ezProceduralPlacementLayerOutput::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  out_Ast.m_OutputNodes.Clear();

  for (auto input : inputs)
  {
    out_Ast.m_OutputNodes.PushBack(input);
  }

  // set defaults
  if (out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::Density] == nullptr)
  {
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::Density] = out_Ast.CreateConstant(1.0f);
  }

  if (out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::Scale] == nullptr)
  {
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::Scale] = CreateRandom(11.0f, out_Ast);
  }

  if (out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ColorIndex] == nullptr)
  {
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ColorIndex] = CreateRandom(13.0f, out_Ast);
  }

  if (out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ObjectIndex] == nullptr)
  {
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ObjectIndex] = CreateRandom(17.0f, out_Ast);
  }

  // scale into correct range
  {
    auto pColorIndexScale = out_Ast.CreateConstant(255.0f);
    auto pColorIndexOutput = out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ColorIndex];
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ColorIndex] = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pColorIndexScale, pColorIndexOutput);
  }

  {
    auto pObjectIndexScale = out_Ast.CreateConstant((float)m_ObjectsToPlace.GetCount());
    auto pObjectIndexOutput = out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ObjectIndex];
    out_Ast.m_OutputNodes[ezPPInternal::ExpressionOutputs::ObjectIndex] = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pObjectIndexScale, pObjectIndexOutput);
  }

  return nullptr;
}

void ezProceduralPlacementLayerOutput::Save(ezStreamWriter& stream)
{
  stream << m_sName;

  stream << m_ObjectsToPlace.GetCount();
  for (auto& object : m_ObjectsToPlace)
  {
    stream << object;
  }

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  stream << m_uiByteCodeIndex;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementRandom, 1, ezRTTIDefaultAllocator<ezProceduralPlacementRandom>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new ezClampValueAttribute(-1, ezVariant()), new ezDefaultValueAttribute(-1), new ezMinValueTextAttribute("Auto")),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezExpressionAST::Node* ezProceduralPlacementRandom::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pRandom = CreateRandom(m_iSeed, out_Ast);
  return pRandom;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementBlend, 1, ezRTTIDefaultAllocator<ezProceduralPlacementBlend>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new ezDefaultValueAttribute(1.0f)),

    EZ_MEMBER_PROPERTY("A", m_InputValueAPin),
    EZ_MEMBER_PROPERTY("B", m_InputValueBPin),
    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezExpressionAST::Node* ezProceduralPlacementBlend::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_Ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_Ast.CreateConstant(m_fInputValueB);
  }

  auto pBlend = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pInputA, pInputB);
  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementHeight, 1, ezRTTIDefaultAllocator<ezProceduralPlacementHeight>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new ezDefaultValueAttribute(0.0f)),
    EZ_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new ezDefaultValueAttribute(1000.0f)),
    EZ_MEMBER_PROPERTY("FadeFraction", m_fFadeFraction)->AddAttributes(new ezDefaultValueAttribute(0.2f)),

    EZ_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezExpressionAST::Node* ezProceduralPlacementHeight::GenerateExpressionASTNode(ezArrayPtr<ezExpressionAST::Node*> inputs, ezExpressionAST& out_Ast)
{
  auto pHeight = out_Ast.CreateInput(ezPPInternal::ExpressionInputs::PositionZ);
  auto pOffset = out_Ast.CreateConstant(m_fMinHeight);
  ezExpressionAST::Node* pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pHeight, pOffset);

  auto pScale = out_Ast.CreateConstant((m_fMaxHeight - m_fMinHeight) * m_fFadeFraction);
  pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Divide, pValue, pScale);

  auto pFadeFactor = out_Ast.CreateConstant(0.5f / m_fFadeFraction);
  pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pValue, pFadeFactor);
  pValue = out_Ast.CreateUnaryOperator(ezExpressionAST::NodeType::Absolute, pValue);
  pValue = out_Ast.CreateBinaryOperator(ezExpressionAST::NodeType::Subtract, pFadeFactor, pValue);

  return pValue;
}
