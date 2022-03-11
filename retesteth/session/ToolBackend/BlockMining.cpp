#include "BlockMining.h"
#include "Options.h"
#include "ToolChainHelper.h"
#include "dataObject/ConvertFile.h"
#include "dataObject/DataObject.h"
#include <libdevcore/CommonIO.h>
#include <testStructures/types/BlockchainTests/BlockchainTestFiller.h>

using namespace test;
using namespace dataobject;
using namespace test::teststruct;

namespace
{
BlockchainTestFillerEnv* readBlockchainFillerTestEnv(spDataObjectMove _data, SealEngine _sEngine)
{
    auto const& data = _data.getPointer();
    if (data->count("baseFeePerGas"))
        return new BlockchainTestFillerEnv1559(_data, _sEngine);
    return new BlockchainTestFillerEnvLegacy(_data, _sEngine);
}
}  // namespace

namespace toolimpl
{
void BlockMining::prepareEnvFile()
{
    m_envPath = m_chainRef.tmpDir() / "env.json";
    auto spHeader = m_currentBlockRef.header()->asDataObject();
    spBlockchainTestFillerEnv env(readBlockchainFillerTestEnv(dataobject::move(spHeader), m_chainRef.engine()));
    spDataObject& envData = const_cast<spDataObject&>(env->asDataObject());
    if (m_parentBlockRef.header()->number() != m_currentBlockRef.header()->number())
    {
        if (m_parentBlockRef.header()->hash() != m_currentBlockRef.header()->parentHash())
            ETH_ERROR_MESSAGE("ToolChain::mineBlockOnTool: provided parent block != pending parent block hash!");
        (*envData).removeKey("currentDifficulty");
        (*envData)["parentTimestamp"] = m_parentBlockRef.header()->timestamp().asString();
        (*envData)["parentDifficulty"] = m_parentBlockRef.header()->difficulty().asString();
        (*envData)["parentUncleHash"] = m_parentBlockRef.header()->uncleHash().asString();
    }

    // BlockHeader hash information for tool mining
    size_t k = 0;
    for (auto const& bl : m_chainRef.blocks())
        (*envData)["blockHashes"][fto_string(k++)] = bl.header()->hash().asString();
    for (auto const& un : m_currentBlockRef.uncles())
    {
        spDataObject uncle;
        int delta = (int)(m_currentBlockRef.header()->number() - un->number()).asBigInt();
        if (delta < 1)
            throw test::UpwardsException("Uncle header delta is < 1");
        (*uncle)["delta"] = delta;
        (*uncle)["address"] = un->author().asString();
        (*envData)["ommers"].addArrayObject(uncle);
    }

    // Options Hook
    Options::getCurrentConfig().performFieldReplace(envData.getContent(), FieldReplaceDir::RetestethToClient);

    m_envPathContent = envData->asJson();
    writeFile(m_envPath.string(), m_envPathContent);
}

void BlockMining::prepareAllocFile()
{
    m_allocPath = m_chainRef.tmpDir() / "alloc.json";
    m_allocPathContent = m_currentBlockRef.state()->asDataObject()->asJsonNoFirstKey();
    writeFile(m_allocPath.string(), m_allocPathContent);
}

void BlockMining::prepareTxnFile()
{
    bool exportRLP = true;
    string const txsfile = m_currentBlockRef.transactions().size() && exportRLP ? "txs.rlp" : "txs.json";
    m_txsPath = m_chainRef.tmpDir() / txsfile;

    string txsPathContent;
    if (exportRLP)
    {
        dev::RLPStream txsout(m_currentBlockRef.transactions().size());
        for (auto const& tr : m_currentBlockRef.transactions())
            txsout.appendRaw(tr->asRLPStream().out());
        m_txsPathContent = m_currentBlockRef.transactions().size() ? "\"" + dev::toString(txsout.out()) + "\"" : "[]";
        writeFile(m_txsPath.string(), m_txsPathContent);
    }
    else
    {
        DataObject txs(DataType::Array);
        static u256 c_maxGasLimit = u256("0xffffffffffffffff");
        for (auto const& tr : m_currentBlockRef.transactions())
        {
            if (tr->gasLimit().asBigInt() <= c_maxGasLimit)  // tool fails on limits here.
                txs.addArrayObject(tr->asDataObject(ExportOrder::ToolStyle));
            else
                ETH_WARNING("Retesteth rejecting tx with gasLimit > 64 bits for tool" +
                            TestOutputHelper::get().testInfo().errorDebug());
        }
        Options::getCurrentConfig().performFieldReplace(txs, FieldReplaceDir::RetestethToClient);
        m_txsPathContent = txs.asJson();
        writeFile(m_txsPath.string(), m_txsPathContent);
    }
}

void BlockMining::executeTransition()
{
    m_outPath = m_chainRef.tmpDir() / "out.json";
    m_outAllocPath = m_chainRef.tmpDir() / "outAlloc.json";

    string cmd = m_chainRef.toolPath().string();
    if (m_engine != SealEngine::NoReward)
    {
        // Convert FrontierToHomesteadAt5 -> Homestead if block > 5, and get reward
        auto tupleRewardFork = prepareReward(m_engine, m_chainRef.fork(), m_currentBlockRef.header()->number());
        cmd += " --state.fork " + std::get<1>(tupleRewardFork).asString();
        cmd += " --state.reward " + std::get<0>(tupleRewardFork).asDecString();
    }
    else
        cmd += " --state.fork " + m_chainRef.fork().asString();

    cmd += " --input.alloc " + m_allocPath.string();
    cmd += " --input.txs " + m_txsPath.string();
    cmd += " --input.env " + m_envPath.string();
    cmd += " --output.basedir " + m_chainRef.tmpDir().string();
    cmd += " --output.result " + m_outPath.filename().string();
    cmd += " --output.alloc " + m_outAllocPath.filename().string();

    bool traceCondition = Options::get().vmtrace && m_currentBlockRef.header()->number() != 0;
    if (traceCondition)
    {
        cmd += " --trace ";
        if (!Options::get().vmtrace_nomemory)
            cmd += "--trace.memory ";
        if (!Options::get().vmtrace_noreturndata)
            cmd += "--trace.returndata ";
        if (Options::get().vmtrace_nostack)
            cmd += "--trace.nostack ";
    }

    ETH_TEST_MESSAGE("Alloc:\n" + m_allocPathContent);
    if (m_currentBlockRef.transactions().size())
    {
        ETH_TEST_MESSAGE("Txs:\n" + m_txsPathContent);
        for (auto const& tr : m_currentBlockRef.transactions())
            ETH_TEST_MESSAGE(tr->asDataObject()->asJson());
    }
    ETH_TEST_MESSAGE("Env:\n" + m_envPathContent);

    string out = test::executeCmd(cmd, ExecCMDWarning::NoWarning);
    ETH_TEST_MESSAGE(cmd);
    ETH_TEST_MESSAGE(out);
}

ToolResponse BlockMining::readResult()
{
    string const outPathContent = dev::contentsString(m_outPath.string());
    string const outAllocPathContent = dev::contentsString(m_outAllocPath.string());
    ETH_TEST_MESSAGE("Res:\n" + outPathContent);
    ETH_TEST_MESSAGE("RAlloc:\n" + outAllocPathContent);

    if (outPathContent.empty())
        ETH_ERROR_MESSAGE("Tool returned empty file: " + m_outPath.string());
    if (outAllocPathContent.empty())
        ETH_ERROR_MESSAGE("Tool returned empty file: " + m_outAllocPath.string());

    // Construct block rpc response
    ToolResponse toolResponse(ConvertJsoncppStringToData(outPathContent));
    spDataObject returnState = ConvertJsoncppStringToData(outAllocPathContent);
    toolResponse.attachState(restoreFullState(returnState.getContent()));

    bool traceCondition = Options::get().vmtrace && m_currentBlockRef.header()->number() != 0;
    if (traceCondition)
        traceTransactions(toolResponse);

    return toolResponse;
}

void BlockMining::traceTransactions(ToolResponse& _toolResponse)
{
    size_t i = 0;
    for (auto const& tr : m_currentBlockRef.transactions())
    {
        fs::path txTraceFile;
        string const trNumber = test::fto_string(i++);
        txTraceFile = m_chainRef.tmpDir() / string("trace-" + trNumber + "-" + tr->hash().asString() + ".jsonl");
        if (fs::exists(txTraceFile))
        {
            string const preinfo = "\nTransaction number: " + trNumber + ", hash: " + tr->hash().asString() + "\n";
            string const info = TestOutputHelper::get().testInfo().errorDebug();
            string const traceinfo = "\nVMTrace:" + info + cDefault + preinfo;
            _toolResponse.attachDebugTrace(
                tr->hash(), DebugVMTrace(traceinfo, trNumber, tr->hash(), dev::contentsString(txTraceFile)));
        }
        else
            ETH_LOG("Trace file `" + txTraceFile.string() + "` not found!", 1);
    }
}

BlockMining::~BlockMining()
{
    fs::remove(m_envPath);
    fs::remove(m_allocPath);
    fs::remove(m_txsPath);
    fs::remove(m_outPath);
    fs::remove(m_outAllocPath);
}

}  // namespace toolimpl