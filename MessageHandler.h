#pragma once

#include <binaryninjaapi.h>

class MessageHandler : public BinaryNinja::BinaryDataNotification {

    BinaryNinja::Ref<BinaryNinja::BinaryView> m_data;
    bool m_shouldCleanupARCCode;

    BinaryNinja::Ref<BinaryNinja::Section> m_authStubsSection;
    BinaryNinja::Ref<BinaryNinja::Section> m_stubsSection;

    bool m_readyForRealAnalysisPass = false;
    std::recursive_mutex m_stubMutex;
    std::set<std::string> m_presentRequiredStubs;
    std::set<std::string> m_locatedRequiredStubs;

    std::mutex m_reAnalysisRequiredFunctionsMutex;
    std::set<uint64_t> m_reAnalysisRequiredFunctions;

    std::set<uint64_t> m_msgSendFunctions;
    std::set<uint64_t> m_checkedNonMsgSendFunctions;
    std::set<uint64_t> m_arcFunctions;
    std::set<uint64_t> m_checkedNonARCFunctions;

    std::set<uint64_t> findMsgSendFunctions(BinaryNinja::Ref<BinaryNinja::BinaryView> data);
    std::set<uint64_t> findARCFunctions(BinaryNinja::Ref<BinaryNinja::BinaryView> data);

    virtual void OnSymbolAdded(BinaryNinja::BinaryView* view, BinaryNinja::Symbol* sym) override;

public:
    MessageHandler(BinaryNinja::Ref<BinaryNinja::BinaryView> data);

    void functionWasAnalyzed(uint64_t addr);

    bool ShouldCleanupARCCode() const { return m_shouldCleanupARCCode; }

    std::set<uint64_t> getMessageSendFunctions() const { return m_msgSendFunctions; }
    bool hasMessageSendFunctions() const { return m_msgSendFunctions.size() != 0; }
    bool isMessageSend(uint64_t);
    bool isARCFunction(uint64_t);

    bool isFunctionLocatedInStubSection(uint64_t);
};
