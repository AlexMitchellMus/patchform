#pragma once

#include <vector>
#include <algorithm>
#include <iostream>

namespace pptk
{
    class SafePointerBase
    {
    public:
        virtual void invalidate() = 0;
        virtual ~SafePointerBase() = default;
    };

    template <typename T>
    class SafePointer;

    class SafeObject
    {
    private:
        std::vector<SafePointerBase*> observers;

    public:
        void addObserver(SafePointerBase* ptr)
        {
            observers.push_back(ptr);
        }

        void removeObserver(SafePointerBase* ptr)
        {
            // Force-remove all instances of ptr from observers
            observers.erase(std::remove(observers.begin(), observers.end(), ptr), observers.end());
        }

        bool hasObserver(SafePointerBase* ptr) const
        {
            return std::find(observers.begin(), observers.end(), ptr) != observers.end();
        }

        virtual ~SafeObject()
        {
            while (!observers.empty())
            {
                SafePointerBase* ptr = observers.back();
                observers.pop_back();
                if (ptr)
                {
                    ptr->invalidate();
                }
            }

            observers.clear(); // Ensure the list is empty
        }
    };

    /**
     * @class SafePointer
     * @brief A smart pointer for safely observing objects without owning them.
     *
     * SafePointer<T> is a lightweight observer pointer that automatically
     * invalidates itself when the observed object is deleted. It prevents
     * dangling pointer issues by unregistering itself from the tracked object.
     *
     * Unlike std::shared_ptr or std::unique_ptr, SafePointer does not manage
     * the object's lifetime—it simply observes it safely.
     *
     * @tparam T The type of object to observe (must inherit from SafeObject).
     *
     * Features:
     * - Observer Pattern: Tracks an object without preventing its deletion.
     * - Automatic Invalidation: Becomes null when the observed object is deleted.
     * - Safe Assignments: Ensures old references are properly removed.
     * - Move Semantics: Efficiently transfers tracking without duplicate observers.
     *
     * @note SafePointer should only be used with classes derived from SafeObject.
     */

    template <typename T>
    class SafePointer : public SafePointerBase
    {
    private:
        T* ptr = nullptr;

    public:
        SafePointer()
        {
        }

        SafePointer(T* p)
        {
            assign(p);
        }

        SafePointer(const SafePointer& other) = delete;
        SafePointer& operator=(const SafePointer& other) = delete;

        SafePointer(SafePointer&& other) noexcept
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }

        SafePointer& operator=(SafePointer&& other) noexcept
        {
            if (this != &other)
            {
                assign(other.ptr);
                other.reset();
            }
            return *this;
        }

        ~SafePointer()
        {
            reset();
        }

        SafePointer& operator=(T* newPtr)
        {
            assign(newPtr);
            return *this;
        }

        void assign(T* newPtr)
        {
            if (ptr == newPtr) return; // No-op if same pointer

            reset();

            ptr = newPtr;
            if (ptr)
            {
                if (!ptr->hasObserver(this))
                {
                    ptr->addObserver(this);
                }
                else
                {
                    //std::cout << "[SafePointer] Already an observer, skipping add.\n";
                }
            }
        }

        void reset()
        {
            if (ptr)
            {
                ptr->removeObserver(this);
                ptr = nullptr;
            }
        }

        void invalidate() override
        {
            ptr = nullptr;
        }

        T* get() const { return ptr; }
        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }

        explicit operator bool() const { return ptr != nullptr; }
    };

    // Helper function for type deduction
    template <typename T>
    SafePointer<T> makeSafePointer(T* ptr)
    {
        return SafePointer<T>(ptr);
    }
} // namespace pptk
