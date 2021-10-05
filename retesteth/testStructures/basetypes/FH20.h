#pragma once
#include "FH.h"
#include <retesteth/dataObject/DataObject.h>
#include <retesteth/dataObject/SPointer.h>
#include <algorithm>
#include <random>
using namespace dataobject;

namespace test
{
namespace teststruct
{
// Validate and manage the type of FixedHash20
// Deserialized from string of "0x1122...20" exact length
struct FH20 : FH
{
    FH20(dev::RLP const& _rlp) : FH(_rlp, 20) {}
    FH20(DataObject const& _data) : FH(_data, 20) {}
    FH20(string const& _data) : FH(_data, 20) {}
    FH20(dev::bigint const& _data) : FH(_data, 20) {}
    FH20* copy() const { return new FH20(m_data->asString()); }

    static FH20 random()
    {
        string initStr = "0x";
        std::vector<string> hex{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f"};
        auto rng = std::default_random_engine{};
        for (size_t i = 0; i < 40; i++)
        {
            std::shuffle(hex.begin(), hex.end(), rng);
            initStr += hex.at(5);
        }
        return FH20(DataObject(initStr));
    }
    static FH20 const& zero()
    {
        static FH20 zero("0x0000000000000000000000000000000000000000");
        return zero;
    }
};

typedef GCP_SPointer<FH20> spFH20;

}  // namespace teststruct
}  // namespace test
