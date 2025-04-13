#pragma once

#include <type_traits>
#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <memory>

// Control block based safe pointer system, based on: https://www.codeproject.com/Articles/5316026/5316026/ptr_to_unique.zip

namespace pptk
{
    class SafeControlBlock
    {
    public:
        void invalidate() { valid.store(false, std::memory_order_release); }
        bool isValid() const { return valid.load(std::memory_order_acquire); }

    private:
        std::atomic<bool> valid{true};
    };

    class SafeObject
    {
    public:
        SafeObject() : controlBlock(std::make_shared<SafeControlBlock>()){}

        virtual ~SafeObject()
        {
            if (controlBlock)
            {
                controlBlock->invalidate(); // Mark as invalid before destroying
                controlBlock.reset();
            }
        }

        std::shared_ptr<SafeControlBlock>  getControlBlock() const
        {
            return controlBlock;
        }

    private:
        std::shared_ptr<SafeControlBlock> controlBlock;
    };

    template <typename T>
    class SafePointer
    {
    public:
        SafePointer() = default;

        explicit SafePointer(T* p) { assign(p); }

        SafePointer(const SafePointer& other)
        {
            assign(other.ptr);
        }

        SafePointer& operator=(const SafePointer& other)
        {
            if (this != &other)
                assign(other.ptr);
            return *this;
        }

        SafePointer(SafePointer&& other) noexcept
        {
            ptr = other.ptr;
            block = other.block;
            other.ptr = nullptr;
            other.block = nullptr;
        }

        SafePointer& operator=(SafePointer&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                ptr = other.ptr;
                block = other.block;
                other.ptr = nullptr;
                other.block = nullptr;
            }
            return *this;
        }

        virtual ~SafePointer() { reset(); }

        SafePointer& operator=(T* newPtr)
        {
            if (!newPtr) {
                reset(); // Just reset if nullptr
            } else {
                assign(newPtr); // Otherwise use assign with validity checks
            }
            return *this;
        }

        void assign(T* newPtr)
        {
            ptr = nullptr;
            block.reset();

            if (newPtr != nullptr)
            {
                if constexpr (std::is_base_of_v<SafeObject, T>)
                {
                    SafeObject* safeObj = static_cast<SafeObject*>(newPtr);
                    std::shared_ptr<SafeControlBlock> newBlock;

                    try {
                        newBlock = safeObj->getControlBlock();
                    } catch (...) {
                        return;
                    }

                    if (newBlock && newBlock->isValid())
                    {
                        block = std::move(newBlock);
                        ptr = newPtr;
                    }
                }
            }
        }

        void reset()
        {
            ptr = nullptr;
            block.reset(); // shared_ptr::reset() releases the reference
        }

        T* get() const
        {
            // Check if we have a valid control block before returning the pointer
            return (block && block->isValid()) ? ptr : nullptr;
        }

        T* operator->() const
        {
            T* p = get();
            if (!p)
                return nullptr; // Return nullptr instead of throwing, caller needs to check
            return p;
        }

        T& operator*() const
        {
            T* p = get();
            if (!p)
                throw std::runtime_error("Attempt to dereference null SafePointer");
            return *p;
        }

        explicit operator bool() const { return get() != nullptr; }

    private:
        T* ptr = nullptr;
        std::shared_ptr<SafeControlBlock> block;
    };

    template <typename T>
    SafePointer<T> makeSafePointer(T* ptr)
    {
        return SafePointer<T>(ptr);
    }

} // namespace pptk