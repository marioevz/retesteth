#pragma once
#include "FH.h"
#include <retesteth/dataObject/DataObject.h>
#include <retesteth/dataObject/SPointer.h>
using namespace dataobject;

namespace test
{
namespace teststruct
{
struct FH256 : FH
{
    FH256(dev::RLP const& _rlp) : FH(_rlp, 256) {}
    FH256(DataObject const& _data) : FH(_data, 256) {}
    FH256(string const& _data) : FH(_data, 256) {}
    FH256(dev::bigint const& _data) : FH(_data, 256) {}
    FH256* copy() const { return new FH256(m_data->asString()); }

    static FH256 const& zero()
    {
        static FH256 zero(
            "0x0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000");
        return zero;
    }
};

typedef GCP_SPointer<FH256> spFH256;

}  // namespace teststruct
}  // namespace test
