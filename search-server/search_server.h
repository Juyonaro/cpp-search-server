#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <execution>

using std::literals::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double accuracy = 1e-6;

class SearchServer {
public:
    /*
    Конструкторы
    */
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}
    
    explicit SearchServer(std::string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}
    
    /*
    Метод AddDocument
    */
    
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
    
    /*
    Метод RemoveDocument
    */
    
    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
    
    /*
    Метод FindTopDocuments
    */
    
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,
                                           std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,
                                           std::string_view raw_query,
                                           DocumentStatus status) const;
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,
                                           std::string_view raw_query) const;
    
    /* original */
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    
    /*
    Метод MatchDocument
    */
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;
    
    // Метод получения частот слов по ID документа
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    size_t GetDocumentCount() const;
    
    std::set<int>::const_iterator begin() const {
        return document_ids_.begin();
    }

    std::set<int>::const_iterator end() const {
        return document_ids_.end();
    }
    
private:
    struct DocumentData {
        std::string content;
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;
    // Слово, Документ - TF
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    // Частота слов по ID док-та
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    // Документ, Рейтинг - Статус
    std::map<int, DocumentData> documents_;
    // Порядковый номер ~ ID док-та
    std::set<int> document_ids_;
    
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);
    
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    
    static int ComputeAverageRating(const std::vector<int>& ratings);

    double ComputeWordInverseDocumentFreq(std::string_view word) const;
    
    QueryWord ParseQueryWord(std::string_view text) const;
    
    Query ParseQuery(std::string_view raw_query, bool policy_flag = true) const;
    
    /*
    Приватный метод FindAllDocuments
    */
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&,
                                           const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&,
                                           const Query& query, DocumentPredicate document_predicate) const;
    
};

/*
Реализация шаблонного метода FindTopDocuments
*/

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            return std::abs(lhs.relevance - rhs.relevance) < accuracy ?
                lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
        }
    );
    
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query,
                                                     DocumentStatus status) const {
    return SearchServer::FindTopDocuments(policy, raw_query,
                                         [&status](int document_id, DocumentStatus doc_status, int rating) {
                                             return status == doc_status;
                                         });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
                                                     DocumentPredicate document_predicate) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

/*
Реализация шаблонного метода FindAllDocuments
*/

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
        
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    
    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }
    
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating }
        );
    }
    
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query,
                                                     DocumentPredicate document_predicate) const {
    return SearchServer::FindAllDocuments(query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query,
                                                     DocumentPredicate document_predicate) const {
    std::vector<Document> matched_documents;
    // ConcurrentMap принимает количество подсловарей, на которые надо разбить всё пространство ключей
    ConcurrentMap<int, double> document_to_relevance(100);
    std::map<int, double> document_result;
        
    auto find_condition = [&](std::string_view word) {
        if (word_to_document_freqs_.count(word)) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    };
    
    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), find_condition);
    
    auto erase_condition = [&](std::string_view word) {
        if (word_to_document_freqs_.count(word)) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
    };
    
    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), erase_condition);
    
    // Метод BuildOrdinaryMap объединяет части словаря и возвращет весь словарь целиком
    document_result = document_to_relevance.BuildOrdinaryMap();
    matched_documents.reserve(document_result.size());
    
    for (const auto [document_id, relevance] : document_result) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    
    return matched_documents;
}