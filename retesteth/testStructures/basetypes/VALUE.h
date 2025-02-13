#pragma once
#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/RLP.h>
#include <retesteth/dataObject/DataObject.h>
#include <retesteth/dataObject/SPointer.h>

using namespace dataobject;

namespace test
{
namespace teststruct
{
// Validate and manage the type of VALUE (bigInt)
// Deserialized from string of "0x1122...32", "123343"
// Can be limited by _limit max value

struct VALUE : GCP_SPointerBase
{
    VALUE(dev::RLP const& _rlp);
    VALUE(dev::bigint const&);
    VALUE(int);
    VALUE(DataObject const&);  // Does not require to move smart pointer here as this structure changes a lot
    VALUE* copy() const { return new VALUE(m_data); }

    bool operator<(long long _rhs) const { return m_data < _rhs; }
    bool operator>(VALUE const& _rhs) const { return m_data > _rhs.asBigInt(); }
    bool operator>=(VALUE const& _rhs) const { return m_data >= _rhs.asBigInt(); }
    bool operator<(VALUE const& _rhs) const { return m_data < _rhs.asBigInt(); }
    bool operator<=(VALUE const& _rhs) const { return m_data <= _rhs.asBigInt(); }
    bool operator!=(VALUE const& _rhs) const { return m_data != _rhs.asBigInt(); }
    bool operator==(VALUE const& _rhs) const { return m_data == _rhs.asBigInt(); }

    VALUE operator-(VALUE const& _rhs) const { return VALUE(m_data - _rhs.asBigInt()); }
    VALUE operator-(long long  _rhs) const { return VALUE(m_data - _rhs); }
    VALUE operator/(VALUE const& _rhs) const { return VALUE(m_data / _rhs.asBigInt()); }
    VALUE operator/(long long  _rhs) const { return VALUE(m_data / _rhs); }
    VALUE operator*(VALUE const& _rhs) const { return VALUE(m_data * _rhs.asBigInt()); }
    VALUE operator*(long long  _rhs) const { return VALUE(m_data * _rhs); }
    VALUE operator+(VALUE const& _rhs) const { return VALUE(m_data + _rhs.asBigInt()); }
    VALUE operator+(long long  _rhs) const { return VALUE(m_data + _rhs); }

    VALUE& operator+=(VALUE const& _rhs) { m_data += _rhs.asBigInt(); m_dirty = true; return *this; }
    VALUE& operator+=(long long  _rhs) { m_data += _rhs; m_dirty = true; return *this; }
    VALUE& operator-=(VALUE const& _rhs) { m_data -= _rhs.asBigInt(); m_dirty = true; return *this; }
    VALUE& operator-=(long long  _rhs) { m_data -= _rhs; m_dirty = true; return *this; }
    VALUE& operator/=(VALUE const& _rhs) { m_data /= _rhs.asBigInt(); m_dirty = true; return *this; }
    VALUE& operator/=(long long  _rhs) { m_data /= _rhs; m_dirty = true; return *this; }
    VALUE& operator*=(VALUE const& _rhs) { m_data *= _rhs.asBigInt(); m_dirty = true; return *this; }
    VALUE& operator*=(long long  _rhs) { m_data *= _rhs; m_dirty = true; return *this; }

    VALUE operator++(int) { m_data++; return *this; }

    string const& asString(size_t _roundBytes = 1) const;
    string asDecString() const;
    dev::bigint const& asBigInt() const { return m_data; }
    bool isBigInt() const { return m_bigint; }

private:
    VALUE() {}
    string verifyHexString(std::string const& _s, std::string const& _k = string()) const;
    dev::bigint m_data;
    bool m_bigint = false;

    // Optimizations
    mutable bool m_dirty = true;
    mutable string m_dataStrZeroXCache;
    mutable string m_dataStrBigIntCache;
};

typedef GCP_SPointer<VALUE> spVALUE;


}  // namespace teststruct
}  // namespace test
