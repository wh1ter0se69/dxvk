#pragma once

#include "d3d8_include.h"

namespace dxvk {

  template <typename D3D9Type, typename D3D8Type>
  class D3D8WrappedObject : public ComObjectClamp<D3D8Type> {

  public:

    using D3D9 = D3D9Type;
    using D3D8 = D3D8Type;

    D3D8WrappedObject(Com<D3D9>&& object)
      : m_d3d9(std::move(object)) {
    }

    D3D9* GetD3D9() {
      return m_d3d9.ptr();
    }

    // For cases where the object may be null.
    static D3D9* GetD3D9Nullable(D3D8WrappedObject* self) {
      if (unlikely(self == NULL)) {
        return NULL;
      }
      return self->m_d3d9.ptr();
    }

    template <typename T>
    static D3D9* GetD3D9Nullable(Com<T>& self) {
      return GetD3D9Nullable(self.ptr());
    }

    virtual IUnknown* GetInterface(REFIID riid) {
      if (riid == __uuidof(IUnknown))
        return this;
      if (riid == __uuidof(D3D8))
        return this;

      throw DxvkError("D3D8WrappedObject::QueryInterface: Unknown interface query");
    }

    // GeneralsX: the d3d9 Vulkan interop interfaces (ID3D9VkInteropDevice, ID3D9VkInteropTexture) and the GeneralsX
    // external-recording extension (IDxvkGxExternal) live on the wrapped d3d9 object; a d3d8 client reaches them through
    // its own objects. Compared by value so this header needs no d3d9 interface declarations.
    static bool IsD3D9InteropIid(REFIID riid) {
      static const GUID iids[] = {
        { 0x2eaa4b89, 0x0107, 0x4bdb, { 0x87, 0xf7, 0x0f, 0x54, 0x1c, 0x49, 0x3c, 0xe0 } },  // ID3D9VkInteropDevice
        { 0xd56344f5, 0x8d35, 0x46fd, { 0x80, 0x6d, 0x94, 0xc3, 0x51, 0xb4, 0x72, 0xc1 } },  // ID3D9VkInteropTexture
        { 0x703eff92, 0x9fbc, 0x4fce, { 0xa0, 0xaa, 0xed, 0x0a, 0x25, 0x77, 0x3e, 0x95 } },  // IDxvkGxExternal
      };
      for (const GUID& iid : iids) {
        if (riid == iid)
          return true;
      }
      return false;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) final {
      if (ppvObject == nullptr)
        return E_POINTER;

      *ppvObject = nullptr;

      if (IsD3D9InteropIid(riid))
        return m_d3d9.ptr() != nullptr ? m_d3d9->QueryInterface(riid, ppvObject) : E_NOINTERFACE;

      try {
        *ppvObject = ref(this->GetInterface(riid));
        return S_OK;
      } catch (const DxvkError& e) {
        Logger::warn(e.message());
        Logger::warn(str::format(riid));
        return E_NOINTERFACE;
      }
    }

  private:

    Com<D3D9> m_d3d9;

  };

}