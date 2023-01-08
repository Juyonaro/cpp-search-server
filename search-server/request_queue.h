#pragma once

#include "document.h"
#include "search_server.h" // vector, string

#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(
        const std::string& raw_query, DocumentPredicate document_predicate)
    {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        // Увеличиваем счетчик необработанных запросов, если результат поиска пустой
        if (result.empty()) {
            ++unprocessed_requests_;
        }
        // Первый запрос прошлых суток нам больше не интересен и может быть удалён
        if (requests_.size() >= min_in_day_) {
            // Последний элемент проверяем на пустоту (т.е. запрос не обработан)
            if (requests_.back().result.empty()) {
                // Если результат был пустым, то перед удалением
                // уменьшаем количество необработанных запросов
                --unprocessed_requests_;
            }
            requests_.pop_back();
        }
        
        requests_.push_front({ result });
        
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(
            raw_query, [status](int document_id, DocumentStatus doc_status, int rating)
            { return status == doc_status; }
        );
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        std::vector<Document> result;
    };
    std::deque<QueryResult> requests_;
    
    const SearchServer& search_server_;
    
    const static int min_in_day_ = 1440;
    // Необработанные запросы
    int unprocessed_requests_ = 0;
};