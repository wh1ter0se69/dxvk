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
    , m_retired (new sync::Fence())
    , m_newEpoch(env::getEnvVar("DXVK_GX_NEW_EPOCH") == "1")
    , m_suspend (env::getEnvVar("DXVK_GX_SUSPEND") == "1")
    , m_inPass  (env::getEnvVar("DXVK_GX_INPASS") == "1") {
    // DXVK_GX_NEW_EPOCH=1 is a TEST-ONLY fail arm: the stock beginExternalRendering, whose new
    // tracking epoch lets a later upload be hoisted above earlier external draws.
    if (m_newEpoch)
      Logger::warn("D3D9GxExternal: DXVK_GX_NEW_EPOCH=1 (fail arm) - external commands start a new tracking epoch");
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

    // DXVK's own attachment views - the formats, aspects and view types its framebuffer uses
    // (D3D8 has no sRGB write state, so the non-sRGB view is the bound one).
    D3D9Surface* rt = m_device->m_state.renderTargets[0].ptr();
    D3D9Surface* ds = m_device->m_state.depthStencil.ptr();

    if (rt == nullptr || rt->GetCommonTexture()->GetImage() == nullptr)
      return D3DERR_INVALIDCALL;

    Rc<DxvkImageView> colour = rt->GetRenderTargetView(false);
    Rc<DxvkImageView> depth  = (ds != nullptr && ds->GetCommonTexture()->GetImage() != nullptr) ? ds->GetDepthStencilView() : nullptr;

    if (colour == nullptr)
      return D3DERR_INVALIDCALL;

    // A colour target and a depth surface of different sample counts cannot form one
    // rendering instance (the water reflection binds a 1x target over the MSAA depth,
    // and DXVK drops that depth too, d3d9_device.cpp:6781).
    if (depth != nullptr && depth->image()->info().sampleCount != colour->image()->info().sampleCount)
      return D3DERR_INVALIDCALL;

    std::array<Rc<DxvkImageView>, MaxReads> reads;

    for (UINT i = 0; i < ReadCount; i++) {
      D3D9CommonTexture* texture = ppReads[i] != nullptr
        ? static_cast<D3D9VkInteropTexture*>(ppReads[i])->GetCommonTexture()
        : nullptr;

      if (texture == nullptr || texture->GetImage() == nullptr)
        return D3DERR_INVALIDCALL;

      // A texture that is also an attachment of this instance would be a feedback loop:
      // refused, the caller keeps that mesh classic.
      if (texture->GetImage() == colour->image() || (depth != nullptr && texture->GetImage() == depth->image()))
        return D3DERR_INVALIDCALL;

      // What PrepareDraw would have done for a bound MANAGED texture: pending
      // uploads go into the CS stream ahead of the recorder.
      if (texture->IsManaged() && texture->NeedsAnyUpload())
        texture->PreLoadAll();

      // DXVK's own sampling view: its format, swizzle (X8R8G8B8 reads alpha as one) and view type.
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
      cRetired  = m_retired,
      cNewEpoch = m_newEpoch,
      cSuspend  = m_suspend,
      cInPass   = m_inPass
    ] (DxvkContext* ctx) {
      // In the current recording epoch (see beginGxExternalCommands): DXVK's pass ends,
      // attachments return to their default layouts, pending barriers are recorded, and
      // DXVK re-binds all of its state after us.
      DxvkContextObjects objects;

      // DXVK_GX_INPASS=1: draw inside the pass DXVK has bound, when one is. Reads take the break
      // path: a read may need an upload or a layout DXVK only provides outside a pass.
      bool inPass = cInPass && !cCount && !cNewEpoch && !cSuspend
        && ctx->beginGxExternalCommandsInPass(objects, cColour, cDepth);

      if (inPass) {
        // nothing to end or transition
      } else if (unlikely(cNewEpoch)) {
        VkImageLayout layout = cColour->image()->info().layout;
        ctx->transformImage(cColour->image(), cColour->image()->getAvailableSubresources(), layout, layout);
        objects = ctx->beginExternalRendering();
      } else {
        objects = ctx->beginGxExternalCommands(cSuspend);
      }

      std::array<DxvkGxImage, MaxReads> readInfo = { };

      for (uint32_t i = 0; i < cCount; i++)
        readInfo[i] = GxDescribe(cReads[i]);

      DxvkGxRecordInfo info = { };
      info.cmd       = objects.cmd->getExecCmdBuffer();
      info.colour    = GxDescribe(cColour);
      info.depth     = GxDescribe(cDepth);
      info.readCount = cCount;
      info.reads     = readInfo.data();
      info.initCmd   = inPass ? objects.cmd->getInitCmdBuffer() : VK_NULL_HANDLE;	// the break path records its uploads inline
      info.inPass    = inPass ? 1u : 0u;

      cRecord(&info, cUser);

      // Lifetime, CPU-access tracking and the implicit-resolve cache of what we wrote.
      objects.cmd->track(cColour);
      objects.cmd->track(cColour->image(), DxvkAccess::Write);
      ctx->notifyGxExternalWrite(cColour);

      if (cDepth != nullptr) {
        objects.cmd->track(cDepth);
        objects.cmd->track(cDepth->image(), DxvkAccess::Write);
        ctx->notifyGxExternalWrite(cDepth);
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


  ////////////////////////////////
  // GeneralsX DrawRun
  ///////////////////////////////

  HRESULT STDMETHODCALLTYPE D3D9GxExternal::DrawRun(
          D3DPRIMITIVETYPE          PrimitiveType,
          UINT                      TaskCount,
    const DxvkGxTask*               pTasks,
          UINT                      LightSetCount,
    const DxvkGxLightSet*           pLightSets,
          UINT                      Flags) {
    D3D9DeviceLock lock = m_device->LockDevice();

    m_runStats.calls++;

    // Everything is checked before the first setter: a refused run draws and changes nothing, and
    // the caller then issues the same calls through the d3d8 device. A state block being recorded
    // (d3d8's records on d3d9 too) would capture our setters instead of applying them.
    bool valid = TaskCount != 0 && pTasks != nullptr
      && (LightSetCount == 0 || pLightSets != nullptr)
      && !m_device->ShouldRecord()
      && m_device->m_state.vertexDecl != nullptr
      && m_device->m_state.indices != nullptr
      && !m_device->UseProgrammableVS();

    for (UINT i = 0; valid && i < TaskCount; i++)
      valid = pTasks[i].lightSet < LightSetCount;

    if (!valid) {
      m_runStats.refused++;
      return D3DERR_INVALIDCALL;
    }

    if (Flags & DXVK_GX_RUN_VALIDATE_ONLY)
      return D3D_OK;

    // Per task, DXVK's own setters in the classic per-task order, then DXVK's own draw.
    UINT lastSet = ~0u;

    for (UINT i = 0; i < TaskCount; i++) {
      const DxvkGxTask& task = pTasks[i];

      if (task.lightSet != lastSet) {
        const DxvkGxLightSet& set = pLightSets[task.lightSet];

        for (DWORD l = 0; l < 4; l++) {
          if (set.enableMask & (1u << l))
            m_device->SetLight(l, &set.lights[l]);

          m_device->LightEnable(l, (set.enableMask >> l) & 1u);
        }

        lastSet = task.lightSet;
        m_runStats.lightSetsApplied++;
      }

      if (likely(!(Flags & DXVK_GX_RUN_MUTANT_NOAMB)))
        m_device->SetRenderState(D3DRS_AMBIENT, task.ambient);

      m_device->SetTransform(D3DTS_WORLD, &task.world);

      if (task.normalizeNormals)
        m_device->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

      if (i != 0 && task.primCount != 0 && (Flags & (DXVK_GX_RUN_FAST | DXVK_GX_RUN_CHECK)) && RunFastEligible()) {
        RunDrawFast(PrimitiveType, task, Flags);
      } else {
        m_device->DrawIndexedPrimitive(PrimitiveType, task.baseVertex, task.minIndex, task.numVertices, task.startIndex, task.primCount);
        m_runDormant = m_device->m_flags.raw() & ~GxFFVSFlags.raw();
      }

      if (task.normalizeNormals)
        m_device->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

      m_runStats.tasks++;
    }

    return D3D_OK;
  }


  HRESULT STDMETHODCALLTYPE D3D9GxExternal::GetRunStats(
          DxvkGxRunStats*           pStats) {
    if (pStats == nullptr)
      return D3DERR_INVALIDCALL;

    D3D9DeviceLock lock = m_device->LockDevice();
    *pStats = m_runStats;
    return D3D_OK;
  }


  // The fixed-function VS's own state (key, constants, vertex blend) - the only device flags a task's setters dirty.
  static const D3D9DeviceFlags GxFFVSFlags(
    D3D9DeviceFlag::DirtyFFVertexData,
    D3D9DeviceFlag::DirtyFFVertexBlend,
    D3D9DeviceFlag::DirtyFFVertexShader);


  // Whether PrepareDraw would find nothing to do but the fixed-function VS's own state. A flag the run's
  // last FULL preparation left set is dormant for this state (fog density with linear fog, point scale on
  // triangle lists - Codex rw1x-design-r12b #1) and stays so: between full preparations only the tasks'
  // FF-VS setters run. Anything set since, or anything per-draw pending, takes the full path. The C2b
  // check (DXVK_GX_RUN_CHECK) and the input digest prove it per task.
  bool D3D9GxExternal::RunFastEligible() {
    D3D9DeviceEx* d = m_device;

    if (d->m_flags.raw() & ~GxFFVSFlags.raw() & ~m_runDormant)
      return false;

    if (d->m_activeHazardsRT || d->m_activeHazardsDS || d->m_lastHazardsRT || d->m_lastHazardsDS)
      return false;

    const uint32_t usedSamplerMask = d->m_psShaderMasks.samplerMask | d->m_vsShaderMasks.samplerMask;
    const uint32_t usedTextureMask = d->m_activeTextures & usedSamplerMask;
    const uint32_t streamMask      = d->m_state.vertexDecl->GetStreamMask();
    const uint32_t usedBuffersMask = streamMask & d->m_activeVertexBuffers;

    if ((d->m_activeTexturesToUpload | d->m_activeTexturesToGen | d->m_dirtySamplerStates) & usedTextureMask)
      return false;

    if ((d->m_dirtyTextures & usedSamplerMask) || (d->m_activeVertexBuffersToUpload & streamMask))
      return false;

    // UploadPerDrawData's system-memory paths (its dynamicSysmemVBOs / dynamicSysmemIBO tests)
    if (usedBuffersMask == d->m_activeVertexBuffersToUploadPerDraw)
      return false;

    D3D9CommonBuffer* ibo = GetCommonBuffer(d->m_state.indices);
    return ibo != nullptr && !ibo->NeedsUpload() && !ibo->DoPerDrawUpload() && !d->CanOnlySWVP();
  }


  void D3D9GxExternal::RunDrawFast(
          D3DPRIMITIVETYPE          PrimitiveType,
    const DxvkGxTask&               Task,
          UINT                      Flags) {
    D3D9DeviceEx* d = m_device;

    // The fast path's own work: UpdateFixedFunctionVS handles the VS key, the constants and vertex blend.
    // TEST-ONLY NOKEY forgets the key (the flag survives, so the check below must see the shader re-bound).
    const bool key = d->m_flags.test(D3D9DeviceFlag::DirtyFFVertexShader);

    if (key)
      m_runStats.fastKeyTasks++;

    if (unlikely(Flags & DXVK_GX_RUN_MUTANT_NOKEY))
      d->m_flags.clr(D3D9DeviceFlag::DirtyFFVertexShader);

    d->UpdateFixedFunctionVS();

    if (unlikely((Flags & DXVK_GX_RUN_MUTANT_NOKEY) && key))
      d->m_flags.set(D3D9DeviceFlag::DirtyFFVertexShader);

    UINT minIndex   = Task.minIndex;
    UINT startIndex = Task.startIndex;
    INT  baseVertex = Task.baseVertex;

    if (Flags & DXVK_GX_RUN_CHECK) {
      // C2b: after the fast path's own work, DXVK's full preparation must emit nothing. The draw below
      // then rests on the full preparation, so check mode changes no pixel.
      bool dynamicVBOs = false;
      bool dynamicIBO  = false;

      d->UploadPerDrawData(minIndex, Task.numVertices, startIndex,
        GetVertexCount(PrimitiveType, Task.primCount), baseVertex, &dynamicVBOs, &dynamicIBO);

      const uint64_t before = d->m_gxEmitCount;
      d->PrepareDraw(PrimitiveType, !dynamicVBOs, !dynamicIBO);

      m_runStats.checkedTasks++;

      if (d->m_gxEmitCount != before)
        m_runStats.checkFailed++;
    } else {
      m_runStats.fastTasks++;
    }

    // DrawIndexedPrimitive's own CS command
    d->EmitCs([d,
      cPrimType        = PrimitiveType,
      cPrimCount       = Task.primCount,
      cStartIndex      = startIndex,
      cBaseVertexIndex = baseVertex,
      cInstanceCount   = d->GetInstanceCount()
    ](DxvkContext* ctx) {
      auto drawInfo = d->GenerateDrawInfo(cPrimType, cPrimCount, cInstanceCount);

      d->ApplyPrimitiveType(ctx, cPrimType);

      VkDrawIndexedIndirectCommand draw = { };
      draw.indexCount    = drawInfo.vertexCount;
      draw.instanceCount = drawInfo.instanceCount;
      draw.firstIndex    = cStartIndex;
      draw.vertexOffset  = cBaseVertexIndex;

      ctx->drawIndexed(1u, &draw);
    });
  }

}
