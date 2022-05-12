#include "IndexBuffer.h"

namespace Babylon
{
    IndexBuffer::IndexBuffer(Napi::Reference<Napi::ArrayBuffer>&& ref, gsl::span<uint8_t> bytes, uint16_t flags, bool dynamic)
        : m_data{nullptr, std::move(ref)}
        , m_bytes{bytes}
        , m_flags{flags}
        , m_dynamic{dynamic}
    {
    }

    IndexBuffer::~IndexBuffer()
    {
        Dispose();
    }

    void IndexBuffer::Dispose()
    {
        if (m_disposed)
        {
            return;
        }

        if (bgfx::isValid(m_handle))
        {
            if (m_dynamic)
            {
                bgfx::destroy(m_dynamicHandle);
            }
            else
            {
                bgfx::destroy(m_handle);
            }
        }
        m_bytes = {};
        m_disposed = true;
    }

    void IndexBuffer::Update(Napi::Env env, gsl::span<uint8_t> bytes, uint32_t startIndex)
    {
        if (!m_dynamic)
        {
            throw Napi::Error::New(env, "Cannot update non-dynamic index buffer.");
        }

        if (bgfx::isValid(m_dynamicHandle))
        {
            // Buffer was already created, do a real update operation.
            bgfx::update(m_dynamicHandle, startIndex, bgfx::copy(bytes.data(), static_cast<uint32_t>(bytes.size())));
        }
        else
        {
            // Buffer hasn't been finalized yet, all that's necessary is to swap out the bytes.
            m_bytes = {bytes.data(), bytes.data() + bytes.size()};
        }
    }

    bool IndexBuffer::CreateHandle(JsRuntime* runtime)
    {
        if (bgfx::isValid(m_handle))
        {
            return true;
        }

        m_data.m_runtime = runtime;
        auto releaseFn = [](void*, void* userData)
        {
            auto* data = reinterpret_cast<decltype(m_data)*>(userData);
            data->m_runtime->Dispatch([ref = &data->m_ref](auto)
                { ref->Unref(); });
        };

        const bgfx::Memory* memory = bgfx::makeRef(m_bytes.data(), static_cast<uint32_t>(m_bytes.size()), releaseFn, &m_data);

        if (m_dynamic)
        {
            m_dynamicHandle = bgfx::createDynamicIndexBuffer(memory, m_flags);
        }
        else
        {
            m_handle = bgfx::createIndexBuffer(memory, m_flags);
        }

        return bgfx::isValid(m_handle);
    }

    void IndexBuffer::Set(bgfx::Encoder* encoder, uint32_t firstIndex, uint32_t numIndices)
    {
        if (bgfx::isValid(m_handle))
        {
            if (m_dynamic)
            {
                encoder->setIndexBuffer(m_dynamicHandle, firstIndex, numIndices);
            }
            else
            {
                encoder->setIndexBuffer(m_handle, firstIndex, numIndices);
            }
        }
    }
}
