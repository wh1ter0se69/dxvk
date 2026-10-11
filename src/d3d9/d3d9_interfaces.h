#pragma once

#include "d3d9_include.h"
#include "../vulkan/vulkan_loader.h"

/**
 * \brief D3D9 interface for Vulkan interop
 *
 * Provides access to the instance and physical device
 * handles for the given D3D9 interface and adapter ordinals.
 */
MIDL_INTERFACE("3461a81b-ce41-485b-b6b5-fcf08ba6a6bd")
ID3D9VkInteropInterface : public IUnknown {
  /**
   * \brief Queries Vulkan handles used by DXVK
   * 
   * \param [out] pInstance The Vulkan instance
   */
  virtual void STDMETHODCALLTYPE GetInstanceHandle(
          VkInstance*           pInstance) = 0;

  /**
   * \brief Queries Vulkan handles used by DXVK
   * 
   * \param [in] Adapter Adapter ordinal
   * \param [out] pInstance The Vulkan instance
   */
  virtual void STDMETHODCALLTYPE GetPhysicalDeviceHandle(
          UINT                  Adapter,
          VkPhysicalDevice*     pPhysicalDevice) = 0;
};

/**
 * \brief D3D9 interface for Vulkan interop - extended
 *
 * Provides access to the instance extension lists
 * and everything provided by ID3D9VkInteropInterface
 */
MIDL_INTERFACE("d6589ed4-7a37-4096-bac2-223b25ae31d2")
ID3D9VkInteropInterface1 : public ID3D9VkInteropInterface {
  /**
   * \brief Gets a list of enabled instance extensions
   * 
   * \param [out] pExtensionCount Number of extensions
   * \param [out] ppExtensions List of extension names
   * \returns D3DERR_MOREDATA if the list was truncated
   */
  virtual HRESULT STDMETHODCALLTYPE GetInstanceExtensions(
          UINT*                       pExtensionCount,
    const char**                      ppExtensions) = 0;
};

/**
 * \brief D3D9 texture interface for Vulkan interop
 * 
 * Provides access to the backing image of a
 * D3D9 texture, surface, or volume.
 */
MIDL_INTERFACE("d56344f5-8d35-46fd-806d-94c351b472c1")
ID3D9VkInteropTexture : public IUnknown {
  /**
   * \brief Retrieves Vulkan image info
   * 
   * Retrieves both the image handle as well as the image's
   * properties. Any of the given pointers may be \c nullptr.
   * 
   * If \c pInfo is not \c nullptr, the following rules apply:
   * - \c pInfo->sType \e must be \c VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
   * - \c pInfo->pNext \e must be \c nullptr or point to a supported
   *   extension-specific structure (currently none)
   * - \c pInfo->queueFamilyIndexCount must be the length of the
   *   \c pInfo->pQueueFamilyIndices array, in \c uint32_t units.
   * - \c pInfo->pQueueFamilyIndices must point to a pre-allocated
   *   array of \c uint32_t of size \c pInfo->pQueueFamilyIndices.
   * 
   * \note As of now, the sharing mode will always be
   *       \c VK_SHARING_MODE_EXCLUSIVE and no queue
   *       family indices will be written to the array.
   * 
   * After the call, the structure pointed to by \c pInfo can
   * be used to create an image with identical properties.
   * 
   * If \c pLayout is not \c nullptr, it will receive the
   * layout that the image will be in after flushing any
   * outstanding commands on the device.
   * \param [out] pHandle The image handle
   * \param [out] pLayout Image layout
   * \param [out] pInfo Image properties
   * \returns \c S_OK on success, or \c D3DERR_INVALIDCALL
   */
  virtual HRESULT STDMETHODCALLTYPE GetVulkanImageInfo(
          VkImage*              pHandle,
          VkImageLayout*        pLayout,
          VkImageCreateInfo*    pInfo) = 0;
};


/**
 * \brief D3D9 image description
 */
struct D3D9VkExtImageDesc {
  D3DRESOURCETYPE     Type;               // Can be SURFACE, TEXTURE, CUBETEXTURE, VOLUMETEXTURE
  UINT                Width;
  UINT                Height;
  UINT                Depth;              // Can be > 1 for VOLUMETEXTURE
  UINT                MipLevels;          // Can be > 1 for TEXTURE, CUBETEXTURE, VOLUMETEXTURE
  DWORD               Usage;
  D3DFORMAT           Format;
  D3DPOOL             Pool;
  D3DMULTISAMPLE_TYPE MultiSample;        // Must be NONE unless Type is SURFACE
  DWORD               MultiSampleQuality;
  bool                Discard;            // Depth stencils only
  bool                IsAttachmentOnly;   // If false, then VK_IMAGE_USAGE_SAMPLED_BIT will be added
  bool                IsLockable;
  VkImageUsageFlags   ImageUsage;         // Additional image usage flags
};

/**
 * \brief D3D9 device interface for Vulkan interop
 *
 * Provides access to the device and instance handles
 * as well as the queue that is used for rendering.
 */
MIDL_INTERFACE("2eaa4b89-0107-4bdb-87f7-0f541c493ce0")
ID3D9VkInteropDevice : public IUnknown {
  /**
   * \brief Queries Vulkan handles used by DXVK
   * 
   * \param [out] pInstance The Vulkan instance
   * \param [out] pPhysDev The physical device
   * \param [out] pDevide The device handle
   */
  virtual void STDMETHODCALLTYPE GetVulkanHandles(
          VkInstance*           pInstance,
          VkPhysicalDevice*     pPhysDev,
          VkDevice*             pDevice) = 0;

  /**
   * \brief Queries the rendering queue used by DXVK
   * 
   * \param [out] pQueue The Vulkan queue handle
   * \param [out] pQueueIndex Queue index
   * \param [out] pQueueFamilyIndex Queue family index
   */
  virtual void STDMETHODCALLTYPE GetSubmissionQueue(
          VkQueue*              pQueue,
          uint32_t*             pQueueIndex,
          uint32_t*             pQueueFamilyIndex) = 0;

  /**
   * \brief Transitions a Texture to a given layout
   * 
   * Executes an explicit image layout transition on the
   * D3D device. Note that the image subresources \e must
   * be transitioned back to its original layout before
   * using it again from D3D9.
   * Synchronization is left up to the caller.
   * This function merely emits a call to transition the
   * texture on the DXVK internal command stream.
   * \param [in] pTexture The image to transform
   * \param [in] pSubresources Subresources to transform
   * \param [in] OldLayout Current image layout
   * \param [in] NewLayout Desired image layout
   */
  virtual void STDMETHODCALLTYPE TransitionTextureLayout(
          ID3D9VkInteropTexture*    pTexture,
    const VkImageSubresourceRange*  pSubresources,
          VkImageLayout             OldLayout,
          VkImageLayout             NewLayout) = 0;

  /**
   * \brief Flushes outstanding D3D rendering commands
   * 
   * Must be called before submitting Vulkan commands
   * to the rendering queue if those commands use the
   * backing resource of a D3D9 object.
   */
  virtual void STDMETHODCALLTYPE FlushRenderingCommands() = 0;
  
  /**
   * \brief Locks submission queue
   * 
   * Should be called immediately before submitting
   * Vulkan commands to the rendering queue in order
   * to prevent DXVK from using the queue.
   * 
   * While the submission queue is locked, no D3D9
   * methods must be called from the locking thread,
   * or otherwise a deadlock might occur.
   */
  virtual void STDMETHODCALLTYPE LockSubmissionQueue() = 0;
  
  /**
   * \brief Releases submission queue
   * 
   * Should be called immediately after submitting
   * Vulkan commands to the rendering queue in order
   * to allow DXVK to submit new commands.
   */
  virtual void STDMETHODCALLTYPE ReleaseSubmissionQueue() = 0;

  /**
   * \brief Locks the device
   * 
   * Can be called to ensure no D3D9 device methods
   * can be executed until UnlockDevice has been called.
   * 
   * This will do nothing if the D3DCREATE_MULTITHREADED
   * is not set.
   */
  virtual void STDMETHODCALLTYPE LockDevice() = 0;
  
  /**
   * \brief Unlocks the device
   * 
   * Must only be called after a call to LockDevice.
   */
  virtual void STDMETHODCALLTYPE UnlockDevice() = 0;

  /**
   * \brief Wait for a resource to finish being used
   * 
   * Waits for the GPU resource associated with the
   * resource to finish being used by the GPU.
   * 
   * Valid D3DLOCK flags:
   *  - D3DLOCK_READONLY:  Only waits for writes
   *  - D3DLOCK_DONOTWAIT: Does not wait for the resource (may flush)
   * 
   * \param [in] pResource Resource to be waited upon
   * \param [in] MapFlags D3DLOCK flags
   * \returns true if the resource is ready to use,
   *          false if the resource is till in use
   */
  virtual bool STDMETHODCALLTYPE WaitForResource(
          IDirect3DResource9*  pResource,
          DWORD                MapFlags) = 0;

  /**
   * \brief Creates a custom image/surface/texture
   * 
   * \param [in] desc Image description
   * \param [out, retval] ppResult Pointer to a resource of the D3DRESOURCETYPE given by desc.Type
   * \returns D3D_OK, D3DERR_INVALIDCALL, or D3DERR_OUTOFVIDEOMEMORY
   */
  virtual HRESULT STDMETHODCALLTYPE CreateImage(
          const D3D9VkExtImageDesc* desc,
          IDirect3DResource9**      ppResult) = 0;
};

/**
 * \brief GeneralsX: one image as DXVK holds it when the external recorder runs
 *
 * \c layout, \c stages and \c access are DXVK's default ("available to") state of the
 * image: it is in \c layout on entry, and the recorder must return it to \c layout with
 * its writes made available to \c stages / \c access before it returns. \c image is
 * VK_NULL_HANDLE when no such attachment is bound.
 */
struct DxvkGxImage {
  VkImage                 image;
  VkImageView             view;
  VkFormat                format;
  VkSampleCountFlagBits   samples;
  VkExtent3D              extent;
  VkImageSubresourceRange range;
  VkImageLayout           layout;
  VkPipelineStageFlags    stages;
  VkAccessFlags           access;
};

/**
 * \brief GeneralsX: what the external recorder gets
 *
 * \c cmd is DXVK's own execution command buffer, outside any render pass, recorded
 * in D3D order between the draws issued before and after RecordExternal. The recorder
 * records only into \c cmd, makes no D3D call and does not submit.
 */
struct DxvkGxRecordInfo {
  VkCommandBuffer         cmd;
  DxvkGxImage             colour;
  DxvkGxImage             depth;
  uint32_t                readCount;
  const DxvkGxImage*      reads;
  VkCommandBuffer         initCmd;  ///< DXVK's init buffer: uploads + their barrier (in-pass draws' data)
  uint32_t                inPass;   ///< 1: inside DXVK's bound pass - draws only, no barrier or rendering instance
};

typedef void (*PFN_DxvkGxRecord)(const DxvkGxRecordInfo* pInfo, void* pUser);

/**
 * \brief GeneralsX: DXVK's own Vulkan entry and what it enabled
 */
struct DxvkGxDeviceInfo {
  PFN_vkGetInstanceProcAddr getInstanceProcAddr;
  VkInstance              instance;
  VkPhysicalDevice        physicalDevice;
  VkDevice                device;
  uint32_t                queueFamily;
  uint32_t                apiVersion;
  VkBool32                dynamicRendering;
  VkBool32                synchronization2;
  VkBool32                timelineSemaphore;
};

/**
 * \brief GeneralsX DrawRun: the four light slots the game drives, as the device holds them at a draw
 *
 * Bit i of \c enableMask (i < 4): slot i enabled, its value in \c lights[i]; a clear bit: slot i disabled
 * (its value is not read). D3DLIGHT9 has D3DLIGHT8's layout.
 */
struct DxvkGxLightSet {
  UINT                    enableMask;
  UINT                    reserved;
  D3DLIGHT9               lights[4];
};

/**
 * \brief GeneralsX DrawRun: one classic indexed draw of a claimed run and its per-task state
 *
 * Applied in the classic per-task order: the light set (when its index differs from the
 * previous task's), D3DRS_AMBIENT, D3DTS_WORLD, D3DRS_NORMALIZENORMALS TRUE when
 * \c normalizeNormals, the draw at \c baseVertex, D3DRS_NORMALIZENORMALS FALSE when
 * \c normalizeNormals. Everything else the draw reads is the device's state at the call.
 */
struct DxvkGxTask {
  D3DMATRIX               world;
  DWORD                   ambient;
  UINT                    lightSet;
  UINT                    normalizeNormals;
  INT                     baseVertex;
  UINT                    minIndex;
  UINT                    numVertices;
  UINT                    startIndex;
  UINT                    primCount;
};

/**
 * \brief GeneralsX DrawRun: cumulative counters since device creation
 */
struct DxvkGxRunStats {
  UINT64                  calls;          ///< DrawRun calls (validate-only ones included)
  UINT64                  refused;        ///< calls that failed validation (nothing drawn)
  UINT64                  tasks;          ///< draws issued
  UINT64                  lightSetsApplied;
  UINT64                  fastTasks;      ///< draws that took the fast path
  UINT64                  checkedTasks;   ///< fast-path candidates proven in check mode
  UINT64                  checkFailed;    ///< ... where the full preparation still emitted work
  UINT64                  fastKeyTasks;   ///< fast-path candidates whose VS key was dirty (NOKEY's arm needs some)
};

#define DXVK_GX_RUN_FAST          0x0001u   ///< tasks after the first skip PrepareDraw when only FF VS state is dirty
#define DXVK_GX_RUN_CHECK         0x0002u   ///< prove each fast-path candidate: full PrepareDraw after it must emit nothing
#define DXVK_GX_RUN_MUTANT_NOKEY  0x0100u   ///< TEST-ONLY: the fast path forgets the VS key (the check must fire)
#define DXVK_GX_RUN_MUTANT_NOAMB  0x0200u   ///< TEST-ONLY: DrawRun skips the ambient setter (the picture must differ)
#define DXVK_GX_RUN_VALIDATE_ONLY 0x8000u   ///< TEST-ONLY: validate, count, return (the per-call overhead toggle)

/**
 * \brief GeneralsX: record foreign Vulkan commands inside DXVK's command list
 *
 * The declaration is mirrored by the game (GXRender/Interop/GxDxvkExternal.h);
 * any change to it takes a new IID.
 */
MIDL_INTERFACE("703eff94-9fbc-4fce-a0aa-ed0a25773e95")
IDxvkGxExternal : public IUnknown {
  /**
   * \brief DXVK's loader entry point, handles, queue family and enabled features
   */
  virtual HRESULT STDMETHODCALLTYPE GetDeviceInfo(
          DxvkGxDeviceInfo*         pInfo) = 0;

  /**
   * \brief Runs \c pfnRecord on DXVK's CS thread, in D3D order
   *
   * Ends DXVK's render pass with every attachment back in its default layout,
   * uploads pending MANAGED data of the read textures first, hands the recorder
   * DXVK's command buffer plus the current colour target 0, the depth-stencil
   * surface and the read textures, then keeps all of them alive on DXVK's command
   * list. \c pfnRecord is called exactly once, later, on another thread; \c pUser
   * must stay valid until then. A non-zero \c RetireValue is signalled once the
   * GPU has finished the command list (see GetRetiredValue).
   * \returns D3DERR_INVALIDCALL when no colour target is bound, the colour and depth
   *          sample counts differ, or a read texture has no image
   */
  virtual HRESULT STDMETHODCALLTYPE RecordExternal(
          UINT                      ReadCount,
          ID3D9VkInteropTexture* const* ppReads,
          PFN_DxvkGxRecord          pfnRecord,
          void*                     pUser,
          UINT64                    RetireValue) = 0;

  /**
   * \brief Highest RetireValue whose command list the GPU has finished
   */
  virtual UINT64 STDMETHODCALLTYPE GetRetiredValue() = 0;

  /**
   * \brief Draws a claimed run of classic indexed draws through DXVK's own d3d9 path
   *
   * Per task: DXVK's own setters for the task's lights / ambient / world / NORMALIZENORMALS
   * bracket and DXVK's own DrawIndexedPrimitive at the task's base vertex, on the calling
   * thread, in D3D order. Validates everything first.
   * \returns D3DERR_INVALIDCALL with nothing drawn or changed: no tasks, a null array, a light
   *          set index out of range, a state block being recorded, a programmable vertex shader
   *          bound, no vertex declaration or no index buffer
   */
  virtual HRESULT STDMETHODCALLTYPE DrawRun(
          D3DPRIMITIVETYPE          PrimitiveType,
          UINT                      TaskCount,
    const DxvkGxTask*               pTasks,
          UINT                      LightSetCount,
    const DxvkGxLightSet*           pLightSets,
          UINT                      Flags) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetRunStats(
          DxvkGxRunStats*           pStats) = 0;
};

/**
 * \brief D3D9 current output metadata
 */
struct D3D9VkExtOutputMetadata {
  float RedPrimary[2];
  float GreenPrimary[2];
  float BluePrimary[2];
  float WhitePoint[2];
  float MinLuminance;
  float MaxLuminance;
  float MaxFullFrameLuminance;
};

/**
 * \brief D3D9 extended swapchain
 */
MIDL_INTERFACE("13776e93-4aa9-430a-a4ec-fe9e281181d5")
ID3D9VkExtSwapchain : public IUnknown {
  virtual BOOL STDMETHODCALLTYPE CheckColorSpaceSupport(
          VkColorSpaceKHR           ColorSpace) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetColorSpace(
          VkColorSpaceKHR           ColorSpace) = 0;

  virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(
    const VkHdrMetadataEXT          *pHDRMetadata) = 0;

  virtual HRESULT STDMETHODCALLTYPE GetCurrentOutputDesc(
          D3D9VkExtOutputMetadata   *pOutputDesc) = 0;

  virtual void STDMETHODCALLTYPE UnlockAdditionalFormats() = 0;
};

#ifndef _MSC_VER
__CRT_UUID_DECL(ID3D9VkInteropInterface,   0x3461a81b,0xce41,0x485b,0xb6,0xb5,0xfc,0xf0,0x8b,0xa6,0xa6,0xbd);
__CRT_UUID_DECL(ID3D9VkInteropInterface1,  0xd6589ed4,0x7a37,0x4096,0xba,0xc2,0x22,0x3b,0x25,0xae,0x31,0xd2);
__CRT_UUID_DECL(ID3D9VkInteropTexture,     0xd56344f5,0x8d35,0x46fd,0x80,0x6d,0x94,0xc3,0x51,0xb4,0x72,0xc1);
__CRT_UUID_DECL(ID3D9VkInteropDevice,      0x2eaa4b89,0x0107,0x4bdb,0x87,0xf7,0x0f,0x54,0x1c,0x49,0x3c,0xe0);
__CRT_UUID_DECL(ID3D9VkExtSwapchain,       0x13776e93,0x4aa9,0x430a,0xa4,0xec,0xfe,0x9e,0x28,0x11,0x81,0xd5);
__CRT_UUID_DECL(IDxvkGxExternal,           0x703eff94,0x9fbc,0x4fce,0xa0,0xaa,0xed,0x0a,0x25,0x77,0x3e,0x95);
#endif
