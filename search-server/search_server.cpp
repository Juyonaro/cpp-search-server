#include "search_server.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <numeric>

using std::literals::string_literals::operator""s;

void SearchServer::AddDocument(int document_id, const std::string& document,
    DocumentStatus status, const std::vector<int>& ratings)
{
    // Првоерка на спец.символы в док-те
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Document contains special symbols"s);
    }
    // Проверка на корректный ID док-та
    if (document_id < 0) {
        throw std::invalid_argument("Document ID is invalid"s);
    }
    // Проверка на имеющийся ID док-та
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("Document ID is already exist"s);
    }
    
    const auto words = SplitIntoWordsNoStop(document);
    
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    // Сохраняем док-т в системе
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    // Сохраняем ID док-та
    document_ids_.emplace(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto [word, _] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(
    const std::string& raw_query, int document_id) const
{
    std::vector<std::string> matched_words;
    
    const auto query = ParseQuery(raw_query);
    // Добавляем док-ты со словами из запроса
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    // Убираем из выдачи результатов док-ты с исключенными словами
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    
    return {matched_words, documents_.at(document_id).status};
}

// Метод получения частот слов по ID документа
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    } else { // Если док-та не существует, возвращаем ссылку на пустой контейнер
        //return std::map<std::string, double>();
        static const std::map<std::string, double> tmp;
        return tmp;
    }
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}
bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c)
        { return c >= '\0' && c < ' '; }
    );
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    // Накапливаем сумму рейтингов документа
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    
    return rating_sum / static_cast<int>(ratings.size());
}
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    // Дополнительная проверка на пустое исключенное слово / двойное тире / спец.символы
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}
SearchServer::Query SearchServer::ParseQuery(const std::string& raw_query) const {
    Query query;
    for (const std::string& word : SplitIntoWords(raw_query)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    
    return query;
}


//


void AddDocument(SearchServer& search_server, int document_id,
    const std::string& document, DocumentStatus status, const std::vector<int>& ratings)
{
    search_server.AddDocument(document_id, document, status, ratings);
}