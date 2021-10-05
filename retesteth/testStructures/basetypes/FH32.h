#pragma once
#include "FH.h"
#include <retesteth/dataObject/DataObject.h>
#include <retesteth/dataObject/SPointer.h>
using namespace dataobject;

namespace test
{
namespace teststruct
{
// Validate and manage the type of FixedHash32
// Deserialized from string of "0x1122...32" exact length
struct FH32 : FH
{
    FH32(dev::RLP const& _rlp) : FH(_rlp, 32) {}
    FH32(DataObject const& _data) : FH(_data, 32) {}
    FH32(string const& _data) : FH(_data, 32) {}
    FH32(dev::bigint const& _data) : FH(_data, 32) {}
    FH32* copy() const { return new FH32(m_data->asString()); }

    bool isZero() const { return m_data.getCContent() == 0; }
    static FH32 const& zero()
    {
        static FH32 zero("0x0000000000000000000000000000000000000000000000000000000000000000");
        return zero;
    }
};

typedef GCP_SPointer<FH32> spFH32;

}  // namespace teststruct
}  // namespace test
