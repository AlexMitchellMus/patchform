#pragma once

#include <type_traits>
#include <algorithm>
#include <atomic>
#include <stdexcept>

// Control block based safe pointer system, based on: https://www.codeproject.com/Articles/5316026/5316026/ptr_to_unique.zip

namespace pptk
{
    class SafeControlBlock
    {
    public:
        SafeControlBlock() = default;

        // Non-copyable
        SafeControlBlock(const SafeControlBlock&) = delete;
        SafeControlBlock& operator=(const SafeControlBlock&) = delete;

        void invalidate() { valid.store(false, std::memory_order_release); }
        bool isValid() const { return valid.load(std::memory_order_acquire); }

        void addRef() { refCount.fetch_add(1, std::memory_order_relaxed); }

        bool release()
        {
            // Decrement reference count and check if it's the last reference
            if (refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                delete this;
                return true;
            }
            return false;
        }

    private:
        std::atomic<int> refCount{0};
        std::atomic<bool> valid{true};
    };

    class SafeObject
    {
    public:
        SafeObject() : controlBlock(new SafeControlBlock())
        {
            controlBlock->addRef();
        }

        virtual ~SafeObject()
        {
            if (controlBlock)
            {
                controlBlock->invalidate(); // Mark as invalid before destroying
                controlBlock->release();    // Properly release the reference
                controlBlock = nullptr;
            }
        }

        SafeControlBlock* getControlBlock() const
        {
            return controlBlock;
        }

    private:
        SafeControlBlock* controlBlock;
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
            assign(newPtr);
            return *this;
        }

        void assign(T* newPtr)
        {
            // Store old values before changing them
            SafeControlBlock* oldBlock = block;

            // Reset internal pointers
            ptr = nullptr;
            block = nullptr;

            // Release the old control block after nullifying our pointers
            if (oldBlock)
            {
                oldBlock->release();
            }

            // Now assign the new pointer
            if (newPtr != nullptr)
            {
                ptr = newPtr;

                if constexpr (std::is_base_of_v<SafeObject, T>)
                {
                    // Use a static_cast when we know it's a SafeObject
                    SafeObject* safeObj = static_cast<SafeObject*>(newPtr);
                    block = safeObj->getControlBlock();

                    // Only add reference if the control block is valid
                    if (block && block->isValid())
                    {
                        block->addRef();
                    }
                    else
                    {
                        // If control block is invalid, don't store the pointer
                        ptr = nullptr;
                        block = nullptr;
                    }
                }
            }
        }

        void reset()
        {
            // Create local copies of the pointers
            T* oldPtr = ptr;
            SafeControlBlock* oldBlock = block;

            // Clear our member pointers first to avoid reentrance issues
            ptr = nullptr;
            block = nullptr;

            // Now release the block if we have one
            if (oldBlock)
            {
                // We don't need to check isValid here - we just release our reference
                oldBlock->release();
            }
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
        SafeControlBlock* block = nullptr;
    };

    template <typename T>
    SafePointer<T> makeSafePointer(T* ptr)
    {
        return SafePointer<T>(ptr);
    }

} // namespace pptk