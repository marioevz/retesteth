#include "TransactionLegacy.h"
#include <libdevcore/Address.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Common.h>
#include <retesteth/EthChecks.h>
#include <retesteth/TestHelper.h>
#include <retesteth/testStructures/Common.h>

using namespace dev;

namespace test
{
namespace teststruct
{
TransactionLegacy::TransactionLegacy(spDataObjectMove _data)
{
    m_secretKey = spVALUE(new VALUE(0));
    m_rawData = _data.getPointer();
    fromDataObject(m_rawData.getCContent());
    (*m_rawData).removeKey("secretKey");
}

void TransactionLegacy::fromDataObject(DataObject const& _data)
{
    try
    {
        REQUIRE_JSONFIELDS(_data, "TransactionLegacy " + _data.getKey(),
            {
                {"data", {{DataType::String}, jsonField::Required}},
                {"gasLimit", {{DataType::String}, jsonField::Required}},
                {"gasPrice", {{DataType::String}, jsonField::Required}},
                {"nonce", {{DataType::String}, jsonField::Required}},
                {"value", {{DataType::String}, jsonField::Required}},
                {"to", {{DataType::String, DataType::Null}, jsonField::Required}},
                {"secretKey", {{DataType::String}, jsonField::Optional}},
                {"v", {{DataType::String}, jsonField::Optional}},
                {"r", {{DataType::String}, jsonField::Optional}},
                {"s", {{DataType::String}, jsonField::Optional}},

                {"publicKey", {{DataType::String}, jsonField::Optional}},  // Besu EthGetBlockBy transaction
                {"raw", {{DataType::String}, jsonField::Optional}},        // Besu EthGetBlockBy transaction
                {"chainId", {{DataType::String}, jsonField::Optional}},    // Besu EthGetBlockBy transaction

                {"blockHash", {{DataType::String}, jsonField::Optional}},         // EthGetBlockBy transaction
                {"blockNumber", {{DataType::String}, jsonField::Optional}},       // EthGetBlockBy transaction
                {"from", {{DataType::String}, jsonField::Optional}},              // EthGetBlockBy transaction
                {"hash", {{DataType::String}, jsonField::Optional}},              // EthGetBlockBy transaction
                {"transactionIndex", {{DataType::String}, jsonField::Optional}},  // EthGetBlockBy transaction
                {"expectException", {{DataType::Object}, jsonField::Optional}}    // BlockchainTest filling
            });

        m_data = spBYTES(new BYTES(_data.atKey("data")));
        m_gasLimit = spVALUE(new VALUE(_data.atKey("gasLimit")));
        m_gasPrice = spVALUE(new VALUE(_data.atKey("gasPrice")));
        m_nonce = spVALUE(new VALUE(_data.atKey("nonce")));
        m_value = spVALUE(new VALUE(_data.atKey("value")));

        if (_data.atKey("to").type() == DataType::Null || _data.atKey("to").asString().empty())
            m_creation = true;
        else
        {
            m_creation = false;
            m_to = spFH20(new FH20(_data.atKey("to")));
        }

        if (_data.count("secretKey"))
            buildVRS(_data.atKey("secretKey"));
        else
        {
            m_v = spVALUE(new VALUE(_data.atKey("v")));
            m_r = spVALUE(new VALUE(_data.atKey("r")));
            m_s = spVALUE(new VALUE(_data.atKey("s")));
            rebuildRLP();
        }
    }
    catch (std::exception const& _ex)
    {
        throw UpwardsException(string("TransactionLegacy convertion error: ") + _ex.what() + _data.asJson());
    }
}

void TransactionLegacy::fromRLP(dev::RLP const& _rlp)
{
    // 0 - nonce        3 - to      6 - v
    // 1 - gasPrice     4 - value   7 - r
    // 2 - gasLimit     5 - data    8 - s

    size_t i = 0;
    m_nonce = spVALUE(new VALUE(_rlp[i++]));
    m_gasPrice = spVALUE(new VALUE(_rlp[i++]));
    m_gasLimit = spVALUE(new VALUE(_rlp[i++]));

    auto const r = _rlp[i++];
    std::ostringstream stream;
    stream << r.toBytes();
    m_creation = false;
    if (stream.str() == "0x")
        m_creation = true;
    else
        m_to = spFH20(new FH20(r));

    m_value = spVALUE(new VALUE(_rlp[i++]));
    m_data = spBYTES(new BYTES(_rlp[i++]));
    m_v = spVALUE(new VALUE(_rlp[i++]));
    m_r = spVALUE(new VALUE(_rlp[i++]));
    m_s = spVALUE(new VALUE(_rlp[i++]));
    rebuildRLP();
}

TransactionLegacy::TransactionLegacy(dev::RLP const& _rlp)
{
    m_secretKey = spVALUE(new VALUE(0));
    fromRLP(_rlp);
}

TransactionLegacy::TransactionLegacy(BYTES const& _rlp)
{
    m_secretKey = spVALUE(new VALUE(0));
    dev::bytes decodeRLP = sfromHex(_rlp.asString());
    dev::RLP rlp(decodeRLP, dev::RLP::VeryStrict);
    fromRLP(rlp);
}

void TransactionLegacy::streamHeader(dev::RLPStream& _s) const
{
    _s << nonce().serializeRLP();
    _s << gasPrice().serializeRLP();
    _s << gasLimit().serializeRLP();
    if (m_creation)
        _s << "";
    else
        _s << to().serializeRLP();
    _s << value().serializeRLP();
    _s << test::sfromHex(data().asString());
}

void TransactionLegacy::buildVRS(VALUE const& _secret)
{
    m_secretKey = spVALUE(new VALUE(_secret));
    dev::RLPStream stream;
    stream.appendList(6);
    streamHeader(stream);
    dev::h256 hash(dev::sha3(stream.out()));
    dev::Signature sig = dev::sign(dev::Secret(_secret.asString()), hash);
    dev::SignatureStruct sigStruct = *(dev::SignatureStruct const*)&sig;
    ETH_FAIL_REQUIRE_MESSAGE(
        sigStruct.isValid(), TestOutputHelper::get().testName() + " Could not construct transaction signature!");

    // 27 because devcrypto signing donesn't count chain id
    bigint v (dev::toCompactHexPrefixed(dev::u256(sigStruct.v + 27)));
    bigint r (dev::toCompactHexPrefixed(dev::u256(sigStruct.r)));
    bigint s (dev::toCompactHexPrefixed(dev::u256(sigStruct.s)));
    m_v = spVALUE(new VALUE(v));
    m_r = spVALUE(new VALUE(r));
    m_s = spVALUE(new VALUE(s));

    (*m_rawData)["v"] = m_v->asString();
    (*m_rawData)["r"] = m_r->asString();
    (*m_rawData)["s"] = m_s->asString();

    rebuildRLP();
}


const spDataObject TransactionLegacy::asDataObject(ExportOrder _order) const
{
    // Optimize out preparation here
    // Cache output data so not to construct it multiple times
    // Useful when filling the tests as this function is called 2-3 times for each tr
    // Promise that after constructor the data does not change
    if (m_rawData.isEmpty())
    {
        spDataObject out(new DataObject());
        (*out)["data"] = m_data->asString();
        (*out)["gasLimit"] = m_gasLimit->asString();
        (*out)["gasPrice"] = m_gasPrice->asString();
        (*out)["nonce"] = m_nonce->asString();
        if (m_creation)
            (*out)["to"] = "";
        else
            (*out)["to"] = m_to->asString();
        (*out)["value"] = m_value->asString();
        (*out)["v"] = m_v->asString();
        (*out)["r"] = m_r->asString();
        (*out)["s"] = m_s->asString();
        m_rawData = out;
    }


    if (_order == ExportOrder::ToolStyle && m_rawDataTool.isEmpty())
    {
        m_rawDataTool = spDataObject(new DataObject());
        (*m_rawDataTool).copyFrom(m_rawData);
        (*m_rawDataTool).performModifier(mod_removeLeadingZerosFromHexValues, DataObject::ModifierOption::RECURSIVE, {"data", "to"});
        (*m_rawDataTool).renameKey("gasLimit", "gas");
        (*m_rawDataTool).renameKey("data", "input");
        if (!m_secretKey.isEmpty() && m_secretKey.getCContent() != 0)
            (*m_rawDataTool)["secretKey"] = m_secretKey->asString();
    }

    return (_order == ExportOrder::ToolStyle) ? m_rawDataTool : m_rawData;
}

void TransactionLegacy::rebuildRLP()
{
    dev::RLPStream out;
    out.appendList(9);
    streamHeader(out);
    if (v().isBigInt())
        out << v().serializeRLP();
    else
        out << v().asBigInt().convert_to<dev::byte>();
    out << r().serializeRLP();
    out << s().serializeRLP();
    m_outRlpStream = out;
    m_rawRLPdata = spBYTES(new BYTES(dev::toHexPrefixed(out.out())));
    m_hash = spFH32(new FH32("0x" + dev::toString(dev::sha3(out.out()))));
}

}  // namespace teststruct
}  // namespace test
