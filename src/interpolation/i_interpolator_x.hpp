#pragma once

#include <memory>

#include <geometry.hpp>

class IInterpolatorX
{
public:
    virtual ~IInterpolatorX() = default;

    virtual void operator()(DSpanX const inout_data, DViewX const coordinates) const = 0;
};

class InterpolatorXProxy : public IInterpolatorX
{
    std::unique_ptr<IInterpolatorX> m_impl;

public:
    InterpolatorXProxy(std::unique_ptr<IInterpolatorX>&& impl) : m_impl(std::move(impl)) {}

    virtual void operator()(DSpanX const inout_data, DViewX const coordinates) const override
    {
        return (*m_impl)(inout_data, coordinates);
    }
};

class IPreallocatableInterpolatorX : public IInterpolatorX
{
public:
    virtual InterpolatorXProxy preallocate() const = 0;
};