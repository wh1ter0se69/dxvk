#include "d3d9_interop.h"
#include "d3d9_interface.h"
#include "d3d9_common_texture.h"
#include "d3d9_device.h"
#include "d3d9_texture.h"
#include "d3d9_surface.h"
#include "d3d9_buffer.h"
#include "d3d9_initializer.h"

namespace dxvk {

  ////////////////////////////////
  // Interface Interop
  ///////////////////////////////

  D3D9VkInteropInterface::D3D9VkInteropInterface(
          D3D9InterfaceEx*      pInterface)
    : m_interface(pInterface) {

  }

  D3D9VkInteropInterface::~D3D9VkInteropInterface() {

  }

  ULONG STDMETHODCALLTYPE D3D9VkInteropInterface::AddRef() {
    return m_interface->AddRef();
  }
  
  ULONG STDMETHODCALLTYPE D3D9VkInteropInterface::Release() {
    return m_interface->Release();
  }
  
  HRESULT STDMETHODCALLTYPE D3D9VkInteropInterface::QueryInterface(
          REFIID                riid,
          void**                ppvObject) {
    return m_interface->QueryInterface(riid, ppvObject);
  }

  void STDMETHODCALLTYPE D3D9VkInteropInterface::GetInstanceHandle(
          VkInstance*           pInstance) {
    if (pInstance != nullptr)
      *pInstance = m_interface->GetInstance()->handle();
  }

  void STDMETHODCALLTYPE D3D9VkInteropInterface::GetPhysicalDeviceHandle(
          UINT                  Adapter,
          VkPhysicalDevice*     pPhysicalDevice) {
    if (pPhysicalDevice != nullptr) {
      D3D9Adapter* adapter = m_interface->GetAdapter(Adapter);
      *pPhysicalDevice = adapter ? adapter->GetDXVKAdapter()->handle() : nullptr;
    }
  }

  HRESULT STDMETHODCALLTYPE D3D9VkInteropInterface::GetInstanceExtensions(
          UINT* pExtensionCount,
    const char** ppExtensions) {
    if (pExtensionCount == nullptr)
      return D3DERR_INVALIDCALL;

    const DxvkNameList& extensions = m_interface->GetInstance()->extensionNameList();

    if (ppExtensions == nullptr) {
      *pExtensionCount = extensions.count();
      return D3D_OK;
    }

    // Write 
    UINT count = 0;
    UINT maxCount = *pExtensionCount;
    for (uint32_t i = 0; i < extensions.count() && i < maxCount; i++) {
      ppExtensions[i] = extensions.name(i);
      count++;
    }

    *pExtensionCount = count;
    return (count < maxCount) ? D3DERR_MOREDATA : D3D_OK;
  }

  ////////////////////////////////
  // Texture Interop
  ///////////////////////////////

  D3D9VkInteropTexture::D3D9VkInteropTexture(
          IUnknown*             pInterface,
          D3D9CommonTexture*    pTexture)
    : m_interface(pInterface)
    , m_texture  (pTexture) {

  }

  D3D9VkInteropTexture::~D3D9VkInteropTexture() {

  }

  ULONG STDMETHODCALLTYPE D3D9VkInteropTexture::AddRef() {
    return m_interface->AddRef();
  }
  
  ULONG STDMETHODCALLTYPE D3D9VkInteropTexture::Release() {
    return m_interface->Release();
  }
  
  HRESULT STDMETHODCALLTYPE D3D9VkInteropTexture::QueryInterface(
          REFIID                riid,
          void**                ppvObject) {
    return m_interface->QueryInterface(riid, ppvObject);
  }

  HRESULT STDMETHODCALLTYPE D3D9VkInteropTexture::GetVulkanImageInfo(
          VkImage*              pHandle,
          VkImageLayout*        pLayout,
          VkImageCreateInfo*    pInfo) {
    const Rc<DxvkImage> image = m_texture->GetImage();

    // GeneralsX: SYSTEMMEM / SCRATCH / NULL-format resources have no image
    if (image == nullptr)
      return D3DERR_INVALIDCALL;

    const DxvkImageCreateInfo& info = image->info();
    
    if (pHandle != nullptr)
      *pHandle = image->handle();
    
    if (pLayout != nullptr)
      *pLayout = info.layout;
    
    if (pInfo != nullptr) {
      // We currently don't support any extended structures
      if (pInfo->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
       || pInfo->pNext != nullptr)
        return D3DERR_INVALIDCALL;
      
      pInfo->flags          = 0;
      pInfo->imageType      = info.type;
      pInfo->format         = info.format;
      pInfo->extent         = info.extent;
      pInfo->mipLevels      = info.mipLevels;
      pInfo->arrayLayers    = info.numLayers;
      pInfo->samples        = info.sampleCount;
      pInfo->tiling         = info.tiling;
      pInfo->usage          = info.usage;
      pInfo->sharingMode    = VK_SHARING_MODE_EXCLUSIVE;
      pInfo->queueFamilyIndexCount = 0;
      pInfo->initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    
    return S_OK;
  }

  ////////////////////////////////
  // Device Interop
  ///////////////////////////////

  D3D9VkInteropDevice::D3D9VkInteropDevice(
          D3D9DeviceEx*         pInterface)
    : m_device(pInterface) {

  }

  D3D9VkInteropDevice::~D3D9VkInteropDevice() {

  }

  ULONG STDMETHODCALLTYPE D3D9VkInteropDevice::AddRef() {
    return m_device->AddRef();
  }
  
  ULONG STDMETHODCALLTYPE D3D9VkInteropDevice::Release() {
    return m_device->Release();
  }
  
  HRESULT STDMETHODCALLTYPE D3D9VkInteropDevice::QueryInterface(
          REFIID                riid,
          void**                ppvObject) {
    return m_device->QueryInterface(riid, ppvObject);
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::GetVulkanHandles(
          VkInstance*           pInstance,
          VkPhysicalDevice*     pPhysDev,
          VkDevice*             pDevice) {
    auto device   = m_device->GetDXVKDevice();
    auto adapter  = device->adapter();
    auto instance = device->instance();
    
    if (pDevice != nullptr)
      *pDevice = device->handle();
    
    if (pPhysDev != nullptr)
      *pPhysDev = adapter->handle();
    
    if (pInstance != nullptr)
      *pInstance = instance->handle();
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::GetSubmissionQueue(
          VkQueue*              pQueue,
          uint32_t*             pQueueIndex,
          uint32_t*             pQueueFamilyIndex) {
    auto device = m_device->GetDXVKDevice();
    DxvkDeviceQueue queue = device->queues().graphics;
    
    if (pQueue != nullptr)
      *pQueue = queue.queueHandle;
    
    if (pQueueIndex != nullptr)
      *pQueueIndex = queue.queueIndex;
    
    if (pQueueFamilyIndex != nullptr)
      *pQueueFamilyIndex = queue.queueFamily;
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::TransitionTextureLayout(
          ID3D9VkInteropTexture*    pTexture,
    const VkImageSubresourceRange*  pSubresources,
          VkImageLayout             OldLayout,
          VkImageLayout             NewLayout) {
    auto texture = static_cast<D3D9VkInteropTexture *>(pTexture)->GetCommonTexture();

    m_device->EmitCs([
      cImage        = texture->GetImage(),
      cSubresources = *pSubresources,
      cOldLayout    = OldLayout,
      cNewLayout    = NewLayout
    ] (DxvkContext* ctx) {
      ctx->transformImage(
        cImage, cSubresources,
        cOldLayout, cNewLayout);
    });
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::FlushRenderingCommands() {
    m_device->Flush();
    m_device->SynchronizeCsThread(DxvkCsThread::SynchronizeAll);
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::LockSubmissionQueue() {
    m_device->GetDXVKDevice()->lockSubmission();
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::ReleaseSubmissionQueue() {
    m_device->GetDXVKDevice()->unlockSubmission();
  }

  void STDMETHODCALLTYPE D3D9VkInteropDevice::LockDevice() {
    m_lock = m_device->LockDevice();
  }
  
  void STDMETHODCALLTYPE D3D9VkInteropDevice::UnlockDevice() {
    m_lock = D3D9DeviceLock();
  }

  static Rc<DxvkPagedResource> GetDxvkResource(IDirect3DResource9 *pResource) {
    switch (pResource->GetType()) {
      case D3DRTYPE_SURFACE:       return static_cast<D3D9Surface*>     (pResource)->GetCommonTexture()->GetImage();
      // Does not inherit from IDirect3DResource9... lol.
      //case D3DRTYPE_VOLUME:        return static_cast<D3D9Volume*>      (pResource)->GetCommonTexture()->GetImage();
      case D3DRTYPE_TEXTURE:       return static_cast<D3D9Texture2D*>   (pResource)->GetCommonTexture()->GetImage();
      case D3DRTYPE_VOLUMETEXTURE: return static_cast<D3D9Texture3D*>   (pResource)->GetCommonTexture()->GetImage();
      case D3DRTYPE_CUBETEXTURE:   return static_cast<D3D9TextureCube*> (pResource)->GetCommonTexture()->GetImage();
      case D3DRTYPE_VERTEXBUFFER:  return static_cast<D3D9VertexBuffer*>(pResource)->GetCommonBuffer()->GetBuffer<D3D9_COMMON_BUFFER_TYPE_REAL>();
      case D3DRTYPE_INDEXBUFFER:   return static_cast<D3D9IndexBuffer*> (pResource)->GetCommonBuffer()->GetBuffer<D3D9_COMMON_BUFFER_TYPE_REAL>();
      default:                     return nullptr;
    }
  }

  bool STDMETHODCALLTYPE D3D9VkInteropDevice::WaitForResource(
          IDirect3DResource9*  pResource,
          DWORD                MapFlags) {
    return m_device->WaitForResource(*GetDxvkResource(pResource), DxvkCsThread::SynchronizeAll, MapFlags);
  }

  HRESULT STDMETHODCALLTYPE D3D9VkInteropDevice::CreateImage(
          const D3D9VkExtImageDesc* params,
          IDirect3DResource9**      ppResult) {
    InitReturnPtr(ppResult);

    if (unlikely(ppResult == nullptr))
      return D3DERR_INVALIDCALL;

    if (unlikely(params == nullptr))
      return D3DERR_INVALIDCALL;

    /////////////////////////////
    // Image desc validation

    // Cannot create a volume by itself, use D3DRTYPE_VOLUMETEXTURE
    if (unlikely(params->Type == D3DRTYPE_VOLUME))
      return D3DERR_INVALIDCALL;

    // Only allowed: SURFACE, TEXTURE, CUBETEXTURE, VOLUMETEXTURE
    if (unlikely(params->Type < D3DRTYPE_SURFACE || params->Type > D3DRTYPE_CUBETEXTURE))
      return D3DERR_INVALIDCALL;

    // Only volume textures can have depth > 1
    if (unlikely(params->Type != D3DRTYPE_VOLUMETEXTURE && params->Depth > 1))
      return D3DERR_INVALIDCALL;

    if (params->Type == D3DRTYPE_SURFACE) {
      // Surfaces can only have 1 mip level
      if (unlikely(params->MipLevels > 1))
        return D3DERR_INVALIDCALL;

      if (unlikely(params->MultiSample > D3DMULTISAMPLE_16_SAMPLES))
        return D3DERR_INVALIDCALL;
    } else {
      // Textures can't be multisampled
      if (unlikely(params->MultiSample != D3DMULTISAMPLE_NONE))
        return D3DERR_INVALIDCALL;
    }

    D3D9_COMMON_TEXTURE_DESC desc;
    desc.Width              = params->Width;
    desc.Height             = params->Height;
    desc.Depth              = params->Depth;
    desc.ArraySize          = params->Type == D3DRTYPE_CUBETEXTURE ? 6 : 1;
    desc.MipLevels          = params->MipLevels;
    desc.Usage              = params->Usage;
    desc.Format             = EnumerateFormat(params->Format);
    desc.Pool               = params->Pool;
    desc.Discard            = params->Discard;
    desc.MultiSample        = params->MultiSample;
    desc.MultisampleQuality = params->MultiSampleQuality;
    desc.IsBackBuffer       = FALSE;
    desc.IsAttachmentOnly   = params->IsAttachmentOnly;
    desc.IsLockable         = params->IsLockable;
    desc.ImageUsage         = params->ImageUsage;
    
    D3DRESOURCETYPE textureType = params->Type == D3DRTYPE_SURFACE ? D3DRTYPE_TEXTURE : params->Type;

    if (FAILED(D3D9CommonTexture::NormalizeTextureProperties(m_device, textureType, &desc)))
      return D3DERR_INVALIDCALL;

    switch (params->Type) {
      case D3DRTYPE_SURFACE:
        return CreateTextureResource<D3D9Surface>(desc, ppResult);

      case D3DRTYPE_TEXTURE:
        return CreateTextureResource<D3D9Texture2D>(desc, ppResult);

      case D3DRTYPE_VOLUMETEXTURE:
        return CreateTextureResource<D3D9Texture3D>(desc, ppResult);

      case D3DRTYPE_CUBETEXTURE:
        return CreateTextureResource<D3D9TextureCube>(desc, ppResult);

      default:
        return D3DERR_INVALIDCALL;
    }
  }

  template <typename ResourceType>
  HRESULT D3D9VkInteropDevice::CreateTextureResource(
          const D3D9_COMMON_TEXTURE_DESC& desc,
          IDirect3DResource9**            ppResult) {
    try {
      const Com<ResourceType> texture = new ResourceType(m_device, &desc, m_device->IsExtended());
      m_device->m_initializer->InitTexture(texture->GetCommonTexture());
      *ppResult = texture.ref();

      if (desc.Pool == D3DPOOL_DEFAULT)
        m_device->m_losableResourceCounter++;

      return D3D_OK;
    }
    catch (const DxvkError& e) {
      Logger::err(e.message());
      return D3DERR_OUTOFVIDEOMEMORY;
    }
  }


  ////////////////////////////////
  // GeneralsX external recording
  ///////////////////////////////

  namespace {

    // An image as the CS lambda captures it: the view is resolved on the CS thread,
    // inside the command list that uses it, so a relocation can never be seen half-way.
    struct GxCapturedImage {
      Rc<DxvkImage>    image;
      DxvkImageViewKey key;
    };

    GxCapturedImage GxCaptureSurface(D3D9Surface* pSurface, VkImageUsageFlags usage) {
      GxCapturedImage result = { };

      if (pSurface == nullptr)
        return result;

      D3D9CommonTexture* texture = pSurface->GetCommonTexture();
      result.image = texture->GetImage();

      if (result.image == nullptr)
        return result;

      result.key.viewType   = VK_IMAGE_VIEW_TYPE_2D;
      result.key.usage      = usage;
      result.key.format     = result.image->info().format;
      result.key.aspects    = lookupFormatInfo(result.key.format)->aspectMask;
      result.key.mipIndex   = pSurface->GetMipLevel();
      result.key.mipCount   = 1u;
      result.key.layerIndex = pSurface->GetFace();
      result.key.layerCount = 1u;
      return result;
    }

    DxvkGxImage GxDescribe(const Rc<DxvkImageView>& view) {
      DxvkGxImage result = { };

      if (view == nullptr)
        return result;

      const Rc<DxvkImage>& image = view->image();
      result.image   = image->handle();
      result.view    = view->handle();
      result.format  = view->info().format;
      result.samples = image->info().sampleCount;
      result.extent  = image->mipLevelExtent(view->info().mipIndex);
      result.range   = view->imageSubresources();
      result.layout  = image->info().layout;
      result.stages  = image->info().stages;
      result.access  = image->info().access;
      return result;
    }

  }


  D3D9GxExternal::D3D9GxExternal(
          D3D9DeviceEx*         pInterface)
    : m_device  (pInterface)
    , m_retired (new sync::Fence()) {

  }

  D3D9GxExternal::~D3D9GxExternal() {

  }

  ULONG STDMETHODCALLTYPE D3D9GxExternal::AddRef() {
    return m_device->AddRef();
  }

  ULONG STDMETHODCALLTYPE D3D9GxExternal::Release() {
    return m_device->Release();
  }

  HRESULT STDMETHODCALLTYPE D3D9GxExternal::QueryInterface(
          REFIID                riid,
          void**                ppvObject) {
    return m_device->QueryInterface(riid, ppvObject);
  }

  HRESULT STDMETHODCALLTYPE D3D9GxExternal::GetDeviceInfo(
          DxvkGxDeviceInfo*     pInfo) {
    if (pInfo == nullptr)
      return D3DERR_INVALIDCALL;

    Rc<DxvkDevice> device = m_device->GetDXVKDevice();
    const DxvkDeviceFeatures& features = device->features();

    *pInfo = DxvkGxDeviceInfo();
    pInfo->getInstanceProcAddr = device->adapter()->vki()->getLoaderProc();
    pInfo->instance            = device->instance()->handle();
    pInfo->physicalDevice      = device->adapter()->handle();
    pInfo->device              = device->handle();
    pInfo->queueFamily         = device->queues().graphics.queueFamily;
    pInfo->apiVersion          = VK_MAKE_API_VERSION(0, 1, 3, 0);
    pInfo->dynamicRendering    = features.vk13.dynamicRendering;
    pInfo->synchronization2    = features.vk13.synchronization2;
    pInfo->timelineSemaphore   = features.vk12.timelineSemaphore;
    return D3D_OK;
  }

  HRESULT STDMETHODCALLTYPE D3D9GxExternal::RecordExternal(
          UINT                      ReadCount,
          ID3D9VkInteropTexture* const* ppReads,
          PFN_DxvkGxRecord          pfnRecord,
          void*                     pUser,
          UINT64                    RetireValue) {
    constexpr UINT MaxReads = 16u;

    if (pfnRecord == nullptr || ReadCount > MaxReads || (ReadCount && ppReads == nullptr))
      return D3DERR_INVALIDCALL;

    D3D9DeviceLock lock = m_device->LockDevice();

    GxCapturedImage colour = GxCaptureSurface(m_device->m_state.renderTargets[0].ptr(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    GxCapturedImage depth  = GxCaptureSurface(m_device->m_state.depthStencil.ptr(),     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if (colour.image == nullptr)
      return D3DERR_INVALIDCALL;

    // A colour target and a depth surface of different sample counts cannot form one
    // rendering instance (the water reflection binds a 1x target over the MSAA depth).
    if (depth.image != nullptr && depth.image->info().sampleCount != colour.image->info().sampleCount)
      return D3DERR_INVALIDCALL;

    std::array<Rc<DxvkImageView>, MaxReads> reads;

    for (UINT i = 0; i < ReadCount; i++) {
      D3D9CommonTexture* texture = ppReads[i] != nullptr
        ? static_cast<D3D9VkInteropTexture*>(ppReads[i])->GetCommonTexture()
        : nullptr;

      if (texture == nullptr || texture->GetImage() == nullptr)
        return D3DERR_INVALIDCALL;

      // What PrepareDraw would have done for a bound MANAGED texture: pending
      // uploads go into the CS stream ahead of the recorder.
      if (texture->IsManaged() && texture->NeedsAnyUpload())
        texture->PreLoadAll();

      reads[i] = texture->GetSampleView(false);
    }

    m_device->EmitCs([
      cColour   = std::move(colour),
      cDepth    = std::move(depth),
      cReads    = std::move(reads),
      cCount    = ReadCount,
      cRecord   = pfnRecord,
      cUser     = pUser,
      cRetire   = RetireValue,
      cRetired  = m_retired
    ] (DxvkContext* ctx) {
      // transformImage(L -> L) is exactly spillRenderPass(false): DXVK's render pass,
      // bound or suspended, ends, deferred clears run, and every attachment returns to
      // its default layout. beginExternalRendering then finalises DXVK's barriers and
      // marks all of its state dirty, so it re-binds everything after us.
      VkImageSubresourceRange colourRange = cColour.image->getAvailableSubresources();
      VkImageLayout colourLayout = cColour.image->info().layout;
      ctx->transformImage(cColour.image, colourRange, colourLayout, colourLayout);

      DxvkContextObjects objects = ctx->beginExternalRendering();

      Rc<DxvkImageView> colourView = cColour.image->createView(cColour.key);
      Rc<DxvkImageView> depthView  = cDepth.image != nullptr ? cDepth.image->createView(cDepth.key) : nullptr;

      std::array<DxvkGxImage, MaxReads> readInfo = { };

      for (uint32_t i = 0; i < cCount; i++)
        readInfo[i] = GxDescribe(cReads[i]);

      DxvkGxRecordInfo info = { };
      info.cmd       = objects.cmd->getExecCmdBuffer();
      info.colour    = GxDescribe(colourView);
      info.depth     = GxDescribe(depthView);
      info.readCount = cCount;
      info.reads     = readInfo.data();

      cRecord(&info, cUser);

      objects.cmd->track(colourView);
      objects.cmd->track(cColour.image, DxvkAccess::Write);

      if (depthView != nullptr) {
        objects.cmd->track(depthView);
        objects.cmd->track(cDepth.image, DxvkAccess::Write);
      }

      for (uint32_t i = 0; i < cCount; i++) {
        objects.cmd->track(cReads[i]);
        objects.cmd->track(cReads[i]->image(), DxvkAccess::Read);
      }

      if (cRetire != 0u)
        ctx->signal(cRetired, cRetire);
    });

    return D3D_OK;
  }

  UINT64 STDMETHODCALLTYPE D3D9GxExternal::GetRetiredValue() {
    return m_retired->value();
  }

}
