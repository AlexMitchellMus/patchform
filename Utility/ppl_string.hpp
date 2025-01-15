/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ostream>

// This file is part of the Plug Patch Library
// Simple std::string wrapper with helpers

namespace ppl {

class string {
public:
    // Constructors
    string() = default; // Default constructor
    explicit string(const char* str) : data_(str) {} // From C-string
    explicit string(const std::string& str) : data_(str) {} // From std::string
    explicit string(std::string&& str) noexcept : data_(std::move(str)) {} // Move constructor

    // Assignment operators
    string& operator=(const char* str) {
        data_ = str;
        return *this;
    }
    string& operator=(const std::string& str) {
        data_ = str;
        return *this;
    }
    string& operator=(std::string&& str) noexcept {
        data_ = std::move(str);
        return *this;
    }

    // Conversion to std::string
    explicit operator const std::string&() const noexcept { return data_; }
    explicit operator std::string&() noexcept { return data_; }

    // Utility functions
    [[nodiscard]] string toLower() const {
        string result = *this;
        std::ranges::transform(result.data_,
                               result.data_.begin(),
                               [](const unsigned char c) { return std::tolower(c); });
        return result;
    }

    [[nodiscard]] string toUpper() const {
        string result = *this;
        std::ranges::transform(result.data_,
                               result.data_.begin(),
                               [](const unsigned char c) { return std::toupper(c); });
        return result;
    }

    [[nodiscard]] bool startsWith(const std::string& prefix) const {
        return data_.compare(0, prefix.size(), prefix) == 0;
    }

    [[nodiscard]] bool endsWith(const std::string& suffix) const {
        if (suffix.size() > data_.size()) return false;
        return data_.compare(data_.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    [[nodiscard]] const std::string& str() const noexcept { return data_; } // Access underlying std::string

    [[nodiscard]] std::vector<string> tokenize(const std::string_view delimiter = " ") const {
        std::vector<string> tokens;
        size_t start = 0;
        size_t end;

        while ((end = data_.find_first_of(delimiter, start)) != std::string::npos) {
            if (end != start) { // Skip empty tokens
                tokens.emplace_back(data_.substr(start, end - start));
            }
            start = end + 1;
        }

        // Add the last token, if any
        if (start < data_.size()) {
            tokens.emplace_back(data_.substr(start));
        }

        return tokens;
    }

    // Accessors
    char& operator[](const size_t index) { return data_[index]; }
    const char& operator[](const size_t index) const { return data_[index]; }

    [[nodiscard]] size_t size() const noexcept { return data_.size(); }
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    // Check if the string is empty
    [[nodiscard]] bool isEmpty() const noexcept {
        return data_.empty();
    }

    // c_str() function to retrieve a C-string
    [[nodiscard]] const char* c_str() const noexcept {
        return data_.c_str();
    }

    // Overload + operator for concatenation
    [[nodiscard]] string operator+(const string& other) const {
        return string(data_ + other.data_);
    }

    [[nodiscard]] string operator+(const std::string& other) const {
        return string(data_ + other);
    }

    [[nodiscard]] string operator+(const char* other) const {
        return string(data_ + std::string(other));
    }

    // Equality operators
    [[nodiscard]] bool operator==(const string& other) const noexcept {
        return data_ == other.data_;
    }

    [[nodiscard]] bool operator==(const std::string& other) const noexcept {
        return data_ == other;
    }

    [[nodiscard]] bool operator==(const char* other) const noexcept {
        return data_ == other;
    }

    [[nodiscard]] bool operator!=(const string& other) const noexcept {
        return data_ != other.data_;
    }

    [[nodiscard]] bool operator!=(const std::string& other) const noexcept {
        return data_ != other;
    }

    [[nodiscard]] bool operator!=(const char* other) const noexcept {
        return data_ != other;
    }

private:
    std::string data_;
};

// Non-member equality operators for symmetry
inline bool operator==(const std::string& lhs, const string& rhs) noexcept
{
    return lhs == rhs.str();
}

inline bool operator==(const char* lhs, const string& rhs) noexcept
{
    return lhs == rhs.str();
}

inline bool operator!=(const std::string& lhs, const string& rhs) noexcept
{
    return lhs != rhs.str();
}

inline bool operator!=(const char* lhs, const string& rhs) noexcept
{
    return lhs != rhs.str();
}

// Overload << operator for ppl::string
inline std::ostream& operator<<(std::ostream& os, const string& s)
{
    os << s.str(); // Use the str() method to access the underlying std::string
    return os;
}




} // namespace ppl
