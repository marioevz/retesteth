#include <retesteth/TestHelper.h>
#include <retesteth/TestOutputHelper.h>
#include <retesteth/testStructures/basetypes.h>
#include <boost/test/unit_test.hpp>
#include <functional>

using namespace std;
using namespace dev;
using namespace test;
using namespace test::teststruct;

void checkException(std::function<void()> _job, string const& _exStr)
{
    bool exception = false;
    try
    {
        _job();
    }
    catch (std::exception const& _ex)
    {
        exception = true;
        ETH_ERROR_REQUIRE_MESSAGE(string(_ex.what()).find(_exStr) != string::npos,
            "(Exception != Expected Exception) " + string(_ex.what()) + " != " + _exStr);
    }
    ETH_ERROR_REQUIRE_MESSAGE(exception, "Expected error! `" + _exStr);
}

template <class T>
void checkSerializeBigint(T const& _a, string const& _rlpForm, string const& _expectedAfter = string())
{
    RLPStream sout(1);
    sout << _a.serializeRLP();

    auto out = sout.out();
    ETH_ERROR_REQUIRE_MESSAGE(
        toHexPrefixed(out) == _rlpForm, "RLP Serialize (out != expected) " + toHexPrefixed(out) + " != " + _rlpForm);

    size_t i = 0;
    RLP rlp(out);
    T aa(rlp[i++]);

    if (!_expectedAfter.empty())
    {
        ETH_ERROR_REQUIRE_MESSAGE(aa.asString() == _expectedAfter,
            "Var Serialize (before != after, expectedafter) " + _a.asString() + " != " + aa.asString() + ", " + _expectedAfter);
    }
    else
        ETH_ERROR_REQUIRE_MESSAGE(
            aa.asString() == _a.asString(), "Var Serialize (before != after) " + _a.asString() + " != " + aa.asString());

    // check that 0x:bigint 0x00 encode into rlp 00 and not 80
}

BOOST_FIXTURE_TEST_SUITE(StructTest, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(value_normal)
{
    std::vector<VALUE> tests = {
        VALUE(DataObject("0x1122")),
        VALUE(DataObject("0x01")),
        VALUE(DataObject("0x1")),
        VALUE(DataObject("0x00")),         // bigint serializes 0 value into `80` instead of `00`
        VALUE(DataObject("0x0")),
        VALUE(DataObject("0x22")),
        VALUE(DataObject("0x112233445500"))
    };

    std::vector<VALUE> testsBigint = {
        VALUE(DataObject("0x:bigint 0x1122")),
        VALUE(DataObject("0x:bigint 0x01")),
        VALUE(DataObject("0x:bigint 0x1")),
        VALUE(DataObject("0x:bigint 0x")),  // 0x is empty `80` encoding
        VALUE(DataObject("0x:bigint 0x")),  // use `0x:bigint 0x00` to actually encode `00`
        VALUE(DataObject("0x:bigint 0x22")),
        VALUE(DataObject("0x:bigint 0x112233445500"))
    };

    RLPStream sout(tests.size());
    RLPStream sout2(tests.size());
    RLPStream soutBigint(tests.size());
    size_t i = 0;
    for (auto const& el : tests)
    {
        sout << el.serializeRLP();
        sout2 << el.asBigInt();
        soutBigint << testsBigint.at(i++).serializeRLP();
    }

    auto out = sout.out();
    auto out2 = sout2.out();
    auto outbigint = soutBigint.out();
    // std::cerr << toHexPrefixed(out) << std::endl;
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == "0xcf821122010180802286112233445500", "RLP Serialize different to expected: `" + toHexPrefixed(out));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == toHexPrefixed(out2), "RLP (serializeRLP != asBigInt) " + toHexPrefixed(out) + " != " + toHexPrefixed(out2));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == toHexPrefixed(outbigint), "RLP (serializeRLP != bigint serializeRLP) " + toHexPrefixed(out) + " != " + toHexPrefixed(outbigint));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out2) == toHexPrefixed(outbigint), "RLP (asBigInt != bigint serializeRLP) " + toHexPrefixed(out2) + " != " + toHexPrefixed(outbigint));


    i = 0;
    RLP rlp(out);
    for (auto const& el : tests)
    {
        VALUE deserialized(rlp[i++]);
        ETH_ERROR_REQUIRE_MESSAGE(deserialized.asString() == el.asString(), "Var (Deserialize != Serialize) " + deserialized.asString() + " != " + el.asString());
    }
}

BOOST_AUTO_TEST_CASE(value_notPrefixed)
{
    checkException([]() { VALUE a(DataObject("1122")); }, "is not prefixed hex");
}

BOOST_AUTO_TEST_CASE(value_leadingZero)
{
    checkException([]() { VALUE a(DataObject("0x0002")); }, "has leading 0");
    checkException([]() { VALUE a(DataObject("0x0001000000000000000000000000000000000000000000000000000000000000000001")); },
        "has leading 0");
}

BOOST_AUTO_TEST_CASE(value_wrongChar)
{
    checkException([]() { VALUE a(DataObject("0xh2")); }, "Unexpected content found while parsing character string");
}

BOOST_AUTO_TEST_CASE(value_emptyByte)
{
    checkException([]() { VALUE a(DataObject("0x")); }, "set as empty byte string");
}

BOOST_AUTO_TEST_CASE(value_emptyString)
{
    checkException([]() { VALUE a(DataObject("")); }, "element must be at least 0x prefix");
}

BOOST_AUTO_TEST_CASE(value_oversize)
{
    checkException(
        []() { VALUE a(DataObject("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")); }, ">u256");
}

//--- OVERLOADED VALUE FEAUTURES ---

BOOST_AUTO_TEST_CASE(valueb_emptyString)
{
    checkException([]() { VALUE a(DataObject("0x:bigint ")); }, "element must be at least 0x prefix");
    checkException([]() { VALUE a(DataObject("0x:bigint")); }, "Unexpected content found while parsing character string");
}

BOOST_AUTO_TEST_CASE(valueb_prefixed00)
{
    VALUE a(DataObject("0x:bigint 0x0022"));
    checkSerializeBigint(a, "0xc3820022");

    VALUE b(DataObject(" 0x:bigint 0x0001000000000000000000000000000000000000000000000000000000000000000001"));
    checkSerializeBigint(b, "0xe4a30001000000000000000000000000000000000000000000000000000000000000000001");
}

BOOST_AUTO_TEST_CASE(valueb_normal)
{
    VALUE a(DataObject("0x:bigint 0x22"));
    checkSerializeBigint(a, "0xc122", "0x22");
}

BOOST_AUTO_TEST_CASE(valueb_normal2)
{
    VALUE a(DataObject("0x:bigint 0x01"));
    checkSerializeBigint(a, "0xc101", "0x01");
}

BOOST_AUTO_TEST_CASE(valueb_empty)
{
    VALUE a(DataObject("0x:bigint 0x"));
    checkSerializeBigint(a, "0xc180", "0x00");
}

BOOST_AUTO_TEST_CASE(valueb_zero)
{
    VALUE a(DataObject("0x:bigint 0x00"));
    checkSerializeBigint(a, "0xc100", "0x00");
}

BOOST_AUTO_TEST_CASE(valueb_zeroPrefixed)
{
    VALUE a(DataObject("0x:bigint 0x0000"));
    checkSerializeBigint(a, "0xc3820000");
}

BOOST_AUTO_TEST_CASE(valueb_oversize)
{
    VALUE a(DataObject("0x:bigint 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    checkSerializeBigint(a, "0xe2a1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
}


// HASH FUNCTIONS
BOOST_AUTO_TEST_CASE(hash32)
{
    FH32 a("0x1122334455667788991011121314151617181920212223242526272829303132");
    checkSerializeBigint(a, "0xe1a01122334455667788991011121314151617181920212223242526272829303132");
    FH32 b("0x0000334455667788991011121314151617181920212223242526272829303132");
    checkSerializeBigint(b, "0xe1a00000334455667788991011121314151617181920212223242526272829303132");
    FH32 c("0x0022334455667788991011121314151617181920212223242526272829303132");
    checkSerializeBigint(c, "0xe1a00022334455667788991011121314151617181920212223242526272829303132");
}

BOOST_AUTO_TEST_CASE(hash256)
{
    FH256 a(
        "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000");
    checkSerializeBigint(a,
        "0xf90103b9010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000");
}

BOOST_AUTO_TEST_CASE(hash32_exceptions)
{
    checkException([]() { FH32 a("0x112233"); }, "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a("0x12233"); }, "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a("0x0000112233"); }, "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a("0x112233445566778899101112131415161718192021222324252627282930313233"); },
        "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a("112233"); }, "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a("0x"); }, "Initializing FH32 from string that is not hash32");
    checkException([]() { FH32 a(""); }, "Initializing FH32 from string that is not hash32");
}


BOOST_AUTO_TEST_CASE(hash_normal)
{
    std::vector<FH32> tests = {FH32(DataObject("0x1122334455667788991011121314151617181920212223242526272829303132"))};

    std::vector<FH32> testsBigint = {
        FH32(DataObject("0x:bigint 0x1122334455667788991011121314151617181920212223242526272829303132"))};

    RLPStream sout(tests.size());
    RLPStream sout2(tests.size());
    RLPStream soutBigint(tests.size());
    size_t i = 0;
    for (auto const& el : tests)
    {
        sout << el.serializeRLP();
        sout2 << el.asBigInt();
        soutBigint << testsBigint.at(i++).serializeRLP();
    }

    auto out = sout.out();
    auto out2 = sout2.out();
    auto outbigint = soutBigint.out();
    // std::cerr << toHexPrefixed(out) << std::endl;
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == "0xe1a01122334455667788991011121314151617181920212223242526272829303132",
        "RLP Serialize different to expected: `" + toHexPrefixed(out));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == toHexPrefixed(out2),
        "RLP (serializeRLP != asBigInt) " + toHexPrefixed(out) + " != " + toHexPrefixed(out2));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out) == toHexPrefixed(outbigint),
        "RLP (serializeRLP != bigint serializeRLP) " + toHexPrefixed(out) + " != " + toHexPrefixed(outbigint));
    ETH_ERROR_REQUIRE_MESSAGE(toHexPrefixed(out2) == toHexPrefixed(outbigint),
        "RLP (asBigInt != bigint serializeRLP) " + toHexPrefixed(out2) + " != " + toHexPrefixed(outbigint));


    i = 0;
    RLP rlp(out);
    for (auto const& el : tests)
    {
        FH32 deserialized(rlp[i++]);
        ETH_ERROR_REQUIRE_MESSAGE(deserialized.asString() == el.asString(),
            "Var (Deserialize != Serialize) " + deserialized.asString() + " != " + el.asString());
    }
}

BOOST_AUTO_TEST_SUITE_END()
