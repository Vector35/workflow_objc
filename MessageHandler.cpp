#include "MessageHandler.h"

using namespace BinaryNinja;

const std::set<std::string> arcFunctionNames = {
    "_objc_retain",
    "_objc_release",
    "_objc_autorelease",
    "_objc_autoreleaseReturnValue",
    "_objc_retainAutoreleasedReturnValue",
    "_objc_unsafeClaimAutoreleasedReturnValue"
};

MessageHandler::MessageHandler(Ref<BinaryView> data)
    : m_data(data)
{
    m_shouldCleanupARCCode = BinaryNinja::Settings::Instance()->Get<bool>("objc.cleanupARCCode");

    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);

    m_authStubsSection = data->GetSectionByName("__auth_stubs");
    m_stubsSection = data->GetSectionByName("__stubs");

    if (!m_authStubsSection && !m_stubsSection)
        m_readyForRealAnalysisPass = true;
    else
        data->RegisterNotification(this);

    m_msgSendFunctions = findMsgSendFunctions(data);
    m_arcFunctions = findARCFunctions(data);
}

void MessageHandler::OnSymbolAdded(BinaryNinja::BinaryView* view, BinaryNinja::Symbol* sym)
{
    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);

    if (m_readyForRealAnalysisPass)
        return;
    if (sym->GetType() != ImportedFunctionSymbol)
        return;
    if (!m_presentRequiredStubs.count(sym->GetShortName()))
        return;

    m_locatedRequiredStubs.insert(sym->GetShortName());
    if (arcFunctionNames.count(sym->GetShortName()))
        m_arcFunctions.insert(sym->GetAddress());
    else if (sym->GetShortName() == "_objc_msgSend") {
        auto f = m_data->GetAnalysisFunction(m_data->GetDefaultPlatform(), sym->GetAddress());
        if (!f)
            return;
        auto retType = BinaryNinja::Confidence<BinaryNinja::Ref<BinaryNinja::Type>>(
            BinaryNinja::Type::PointerType(m_data->GetAddressSize(), BinaryNinja::Type::VoidType(),
                0));

        std::vector<BinaryNinja::FunctionParameter> params;
        auto cc = m_data->GetDefaultPlatform()->GetDefaultCallingConvention();

        params.push_back({ "self",
            BinaryNinja::Type::NamedType(m_data, { "id" }),
            true,
            BinaryNinja::Variable() });
        params.push_back({ "sel",
            BinaryNinja::Type::PointerType(m_data->GetAddressSize(), BinaryNinja::Type::IntegerType(1, false)),
            true,
            BinaryNinja::Variable() });

        auto funcType = BinaryNinja::Type::FunctionType(retType, cc, params, true);
        f->SetUserType(funcType);

        m_msgSendFunctions.insert(f->GetStart());
    }

    if (m_locatedRequiredStubs.size() == m_presentRequiredStubs.size()) {
        m_readyForRealAnalysisPass = true;

        for (auto fAddr : m_reAnalysisRequiredFunctions)
            for (auto f : m_data->GetAnalysisFunctionsForAddress(fAddr))
                f->Reanalyze();

        m_data->UnregisterNotification(this);
    }
}

std::set<uint64_t> MessageHandler::findMsgSendFunctions(BinaryNinja::Ref<BinaryNinja::BinaryView> data)
{
    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);

    std::set<uint64_t> results;
    const auto authGotSection = data->GetSectionByName("__auth_got");
    const auto gotSection = data->GetSectionByName("__got");
    const auto laSymbolPtrSection = data->GetSectionByName("__la_symbol_ptr");

    // Shorthand to check if a symbol lies in a given section.
    auto sectionContains = [](Ref<Section> section, Ref<Symbol> symbol) {
        const auto start = section->GetStart();
        const auto length = section->GetLength();
        const auto address = symbol->GetAddress();

        return (uint64_t)(address - start) <= length;
    };

    const auto candidates = data->GetSymbolsByName("_objc_msgSend");
    for (const auto& c : candidates) {
        if ((authGotSection && sectionContains(authGotSection, c))
            || (gotSection && sectionContains(gotSection, c))
            || (laSymbolPtrSection && sectionContains(laSymbolPtrSection, c))) {
            results.insert(c->GetAddress());
        }
        if ((m_authStubsSection && sectionContains(m_authStubsSection, c))
            || (m_stubsSection && sectionContains(m_stubsSection, c))) {
            results.insert(c->GetAddress());
            m_locatedRequiredStubs.insert(c->GetShortName());
        }
    }
    m_presentRequiredStubs.insert("_objc_msgSend");

    return results;
}

std::set<uint64_t> MessageHandler::findARCFunctions(BinaryNinja::Ref<BinaryNinja::BinaryView> data)
{
    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);

    std::set<uint64_t> results;
    const auto authGotSection = data->GetSectionByName("__auth_got");
    const auto gotSection = data->GetSectionByName("__got");
    const auto laSymbolPtrSection = data->GetSectionByName("__la_symbol_ptr");

    // Shorthand to check if a symbol lies in a given section.
    auto sectionContains = [](Ref<Section> section, Ref<Symbol> symbol) {
        const auto start = section->GetStart();
        const auto length = section->GetLength();
        const auto address = symbol->GetAddress();

        return (uint64_t)(address - start) <= length;
    };

    std::vector<BinaryNinja::Ref<BinaryNinja::Symbol>> candidates;
    std::vector<BinaryNinja::Ref<BinaryNinja::Symbol>> next;
    for (auto name : arcFunctionNames) {
        next = data->GetSymbolsByName(name);
        candidates.insert(candidates.end(), next.begin(), next.end());
    }
    for (auto& c : candidates) {
        if ((authGotSection && sectionContains(authGotSection, c))
            || (gotSection && sectionContains(gotSection, c))
            || (laSymbolPtrSection && sectionContains(laSymbolPtrSection, c))) {
            {
                results.insert(c->GetAddress());
                m_presentRequiredStubs.insert(c->GetShortName());
            }
        }
        if ((m_authStubsSection && sectionContains(m_authStubsSection, c))
            || (m_stubsSection && sectionContains(m_stubsSection, c))) {
            m_locatedRequiredStubs.insert(c->GetShortName());
            results.insert(c->GetAddress());
        }
    }

    return results;
}

void MessageHandler::functionWasAnalyzed(uint64_t addr)
{
    if (!m_readyForRealAnalysisPass) {
        std::unique_lock<std::mutex> lock(m_reAnalysisRequiredFunctionsMutex);
        m_reAnalysisRequiredFunctions.insert(addr);
    }
}

bool MessageHandler::isMessageSend(uint64_t functionAddress)
{
    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);
    return m_msgSendFunctions.count(functionAddress);
}

bool MessageHandler::isARCFunction(uint64_t functionAddress)
{
    std::unique_lock<std::recursive_mutex> lock(m_stubMutex);
    return m_arcFunctions.count(functionAddress);
}

bool MessageHandler::isFunctionLocatedInStubSection(uint64_t addr)
{
    if (m_stubsSection && m_stubsSection->GetStart() < addr
        && m_stubsSection->GetStart() + m_stubsSection->GetLength() > addr)
        return true;
    else if (m_authStubsSection && m_authStubsSection->GetStart() < addr
        && m_authStubsSection->GetStart() + m_authStubsSection->GetLength() > addr)
        return true;

    return false;
}