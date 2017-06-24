﻿
#pragma once

#include <WindowsMixedReality/Basics.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <System/Window/Window.h>

namespace ABI
{
  namespace Windows
  {
    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicCameraRenderingParameters;
      }
    }
  }
}

struct IDXGISwapChain;
class ezGALHolographicDeviceDX11;


/// \brief Swapchain for ezGALHolographicDeviceDX11.
///
/// In Windows Holographic there are no swap chains, only cameras.
/// Therefore, each swap chain is associated with a ezWindowsHolographicCamera.
///
/// Conceptually we're splitting the "surface part" of windows holographic camera into this object.
///
/// \see ezWindowsHolographicCamera
class ezGALHolographicSwapChainDX11 : public ezGALSwapChain
{
public:
  // TODO: Not implemented yet, keeping default plane.
  /// \brief Sets the reprojection plane for late stage reprojection.
  //void SetReprojectionFocusPlane(const ezPlane& reprojectionPlane) { m_reprojectionPlane = reprojectionPlane; }

  // TODO: Note also that in Win Creators update there is also "CommitDirect3D11DepthBuffer" for depth based stabilization.
  // We should consider jumping directly to that one instead of doing SetReprojectionFocusPlane anywhere ourselves.

protected:

  friend class ezGALHolographicDeviceDX11;
  friend class ezWindowsHolographicCamera;
  friend class ezMemoryUtils;

  class ezHoloMockWindow : public ezWindowBase
  {
  public:
    virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(0,0); }
    virtual ezWindowHandle GetNativeWindowHandle() const override { return nullptr; };
    virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode) const override { return true; }
    virtual void ProcessWindowMessages() override {}
  };

  /// This mock window is used in the desc of all holographic swap chains.
  /// The holographic device validates whether a new sap chain desc uses this window.
  /// It allows to enforce that holographic swap chains can only be created by the camera.
  static ezHoloMockWindow s_mockWindow;


  ezGALHolographicSwapChainDX11(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALHolographicSwapChainDX11();

  /// \brief Makes sure the backbuffer texture exists and has the correct size.
  ezResult EnsureBackBufferResources(ezGALHolographicDeviceDX11* pDevice, ABI::Windows::Graphics::Holographic::IHolographicCameraRenderingParameters* parameters);

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  //ezPlane m_reprojectionPlane;
}; 
