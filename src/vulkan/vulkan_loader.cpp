#include <tuple>

// GeneralsX @bugfix Claude 19/08/2026 TARGET_OS_IPHONE is only defined once this is included.
// Without it the iOS branch below is invisible to the preprocessor and iOS silently compiles the
// macOS library list, which omits the bundle-relative MoltenVK path iOS cannot start without.
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include "vulkan_loader.h"

#include "../util/log/log.h"

#include "../util/util_string.h"
#include "../util/util_win32_compat.h"

namespace dxvk::vk {

  static std::pair<HMODULE, PFN_vkGetInstanceProcAddr> loadVulkanLibrary() {
#ifdef _WIN32
    static const std::array<const char*, 2> dllNames = {{
      "winevulkan.dll",
      "vulkan-1.dll",
    }};
#elif defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    // iOS: there is no Vulkan loader on the platform - MoltenVK IS the implementation, and it is
    // embedded in the app bundle's Frameworks directory where a bare-name dlopen cannot see it.
    static const std::array<const char*, 5> dllNames = {{
      "@executable_path/Frameworks/MoltenVK.framework/MoltenVK",
      "@executable_path/Frameworks/libMoltenVK.dylib",
      "libvulkan.dylib",
      "libvulkan.1.dylib",
      "libMoltenVK.dylib",
    }};
#elif defined(__APPLE__)
    // macOS: prefer the Vulkan LOADER, and never name MoltenVK by a path.
    //
    // GeneralsX @bugfix Claude 19/08/2026 THE @executable_path ENTRIES ARE NOT HARMLESS HERE, which
    // is what the previous version of this comment claimed. When dlopen fails on a path, dyld
    // retries the LEAF NAME against DYLD_LIBRARY_PATH - and the macOS bundle's run.sh puts
    // Contents/Resources/lib on that path, which contains libMoltenVK.dylib. So the "iOS-only"
    // entry resolved, and DXVK bound vkGetInstanceProcAddr straight to MoltenVK while SDL3 had
    // already created the surface through libvulkan. Two Vulkan implementations in one process:
    //
    //   info:  Vulkan: Found vkGetInstanceProcAddr in @executable_path/Frameworks/libMoltenVK.dylib
    //   [Vulkan Loader] ERROR: vkGetInstanceProcAddr: Invalid instance
    //   zsh: abort
    //
    // Measured on a MacBook Pro 14" M1 Pro. The loader must come first and stay first: it is what
    // SDL_Vulkan_LoadLibrary picks up, and the instance has to come from the same place as the
    // entry points that will be used on it.
    static const std::array<const char*, 3> dllNames = {{
      "libvulkan.dylib",
      "libvulkan.1.dylib",
      "libMoltenVK.dylib",
    }};
#else
    static const std::array<const char*, 2> dllNames = {{
      "libvulkan.so",
      "libvulkan.so.1",
    }};
#endif

    for (auto dllName : dllNames) {
      HMODULE library = LoadLibraryA(dllName);

      if (!library)
        continue;

      auto proc = GetProcAddress(library, "vkGetInstanceProcAddr");

      if (!proc) {
        FreeLibrary(library);
        continue;
      }

      Logger::info(str::format("Vulkan: Found vkGetInstanceProcAddr in ", dllName, " @ 0x", std::hex, reinterpret_cast<uintptr_t>(proc)));
      return std::make_pair(library, reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc));
    }

    Logger::err("Vulkan: vkGetInstanceProcAddr not found");
    return { };
  }

  LibraryLoader::LibraryLoader() {
    std::tie(m_library, m_getInstanceProcAddr) = loadVulkanLibrary();
  }

  LibraryLoader::LibraryLoader(PFN_vkGetInstanceProcAddr loaderProc) {
    m_getInstanceProcAddr = loaderProc;
  }

  LibraryLoader::~LibraryLoader() {
    if (m_library)
      FreeLibrary(m_library);
  }

  PFN_vkVoidFunction LibraryLoader::sym(VkInstance instance, const char* name) const {
    return m_getInstanceProcAddr(instance, name);
  }

  PFN_vkVoidFunction LibraryLoader::sym(const char* name) const {
    return sym(nullptr, name);
  }

  bool LibraryLoader::valid() const {
    return m_getInstanceProcAddr != nullptr;
  }
  
  
  InstanceLoader::InstanceLoader(const Rc<LibraryLoader>& library, bool owned, VkInstance instance)
  : m_library(library), m_instance(instance), m_owned(owned) { }
  
  
  PFN_vkVoidFunction InstanceLoader::sym(const char* name) const {
    return m_library->sym(m_instance, name);
  }
  
  
  DeviceLoader::DeviceLoader(const Rc<InstanceLoader>& library, bool owned, VkDevice device)
  : m_library(library)
  , m_getDeviceProcAddr(reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      m_library->sym("vkGetDeviceProcAddr"))),
    m_device(device), m_owned(owned) { }
  
  
  PFN_vkVoidFunction DeviceLoader::sym(const char* name) const {
    return m_getDeviceProcAddr(m_device, name);
  }
  
  
  LibraryFn::LibraryFn() { }
  LibraryFn::LibraryFn(PFN_vkGetInstanceProcAddr loaderProc)
  : LibraryLoader(loaderProc) { }
  LibraryFn::~LibraryFn() { }
  
  
  InstanceFn::InstanceFn(const Rc<LibraryLoader>& library, bool owned, VkInstance instance)
  : InstanceLoader(library, owned, instance) { }
  InstanceFn::~InstanceFn() {
    if (m_owned)
      this->vkDestroyInstance(m_instance, nullptr);
  }
  
  
  DeviceFn::DeviceFn(const Rc<InstanceLoader>& library, bool owned, VkDevice device)
  : DeviceLoader(library, owned, device) { }
  DeviceFn::~DeviceFn() {
    if (m_owned)
      this->vkDestroyDevice(m_device, nullptr);
  }
  
}