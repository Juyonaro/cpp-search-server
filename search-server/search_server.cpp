#include "search_server.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <numeric>

using std::literals::string_literals::operator""s;

/*
Реализация AddDocument
*/

void SearchServer::AddDocument(int document_id, std::string_view document,
                               DocumentStatus status, const std::vector<int>& ratings) {
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

    // Сохраняем ID док-та
    document_ids_.emplace(document_id);
    
    auto words = SplitIntoWordsNoStop(document);
    
    // Создаем новую строку и инициализируем содержимым document
    const std::string doc_content { document };
    // Сохраняем док-т в системе
    documents_.emplace(document_id, DocumentData{ doc_content, ComputeAverageRating(ratings), status });
    
    words = SplitIntoWordsNoStop(documents_.at(document_id).content);
    
    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
}

/*
Реализация RemoveDocument
*/

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id) == 0) {
        throw std::invalid_argument("Document ID does not exist"s);
    }
    
    for (auto [word, _] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (documents_.count(document_id) == 0) {
        throw std::invalid_argument("Document ID does not exist"s);
    }
    
    // Создаем вектор указателей на слова в док-те
    const auto& words_freqs = document_to_word_freqs_.at(document_id);
    std::vector<const std::string*> words(words_freqs.size());
    // Используем transform для извлечения указателей из словаря
    std::transform(std::execution::par,
                  words_freqs.begin(), words_freqs.end(),
                  words.begin(),
                  [](const auto& elem) {
                      return new std::string(elem.first);;
                  });
    
    // Удаляем док-т из словаря для каждого слова в док-те
    std::for_each(std::execution::par,
                  words.begin(), words.end(),
                 [this, document_id](const std::string* elem) {
                     word_to_document_freqs_.at(*elem).erase(document_id);
                 });
    
    // Удаляем док-т из остальных словарей
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

/*
Реализация FindTopDocuments
*/

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

/*
Реализация MatchDocument
*/

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    std::string_view raw_query, int document_id) const
{
    if (document_id < 0) {
        throw std::invalid_argument("Incorrect document ID"s);
    }
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("Document ID does not exist"s);
    }
    
    const Query query = ParseQuery(raw_query);
    const auto& word_freqs = document_to_word_freqs_.at(document_id);

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    
    // Объявляем лямбда-функцию для проверки наличия слова в минус-словаре
    auto minus_condition = [&word_freqs](const std::string_view word) {
        return word_freqs.count(word) > 0;
    };
    
    // Проверяем есть ли исключенные слова в док-те
    if (std::none_of(query.minus_words.begin(), query.minus_words.end(), minus_condition)) {
        // Если исключенных слов в док-те нет, перебираем слова поискового запроса
        std::for_each(query.plus_words.begin(), query.plus_words.end(),
                      // Захватываем все переменные по ссылке
                      [&](const std::string_view word) {
                          if (word_to_document_freqs_.count(word) != 0 && word_to_document_freqs_.at(word).count(document_id)) {
                              matched_words.emplace_back(word);
                          }
                      });
    }
    
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const
{
    if (document_id < 0) {
        throw std::invalid_argument("Incorrect document ID"s);
    }
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("Document ID does not exist"s);
    }

    const Query& query = ParseQuery(raw_query, false);
    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    
    auto minus_condition = [&word_freqs](const std::string_view word) {
        return word_freqs.count(word) > 0;
    };
    
    if (std::none_of(query.minus_words.begin(), query.minus_words.end(), minus_condition)) {
        std::copy_if(query.plus_words.begin(), query.plus_words.end(),
                     std::back_inserter(matched_words), [&word_freqs](const std::string_view word) {
                         return word_freqs.count(word) > 0;
                     });
    }
    
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

// Метод получения частот слов по ID документа
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    } else { // Если док-та не существует, возвращаем ссылку на пустой контейнер
        static const std::map<std::string_view, double> tmp;
        return tmp;
    }
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    // Дополнительная проверка на пустое исключенное слово / двойное тире / спец.символы
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid"s);
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view raw_query, bool policy_flag) const {
    Query query;
    for (auto word : SplitIntoWords(raw_query)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.emplace_back(query_word.data);
            } else {
                query.plus_words.emplace_back(query_word.data);
            }
        }
    }
    
    // Сортировка и удаление дубликатов будет работать только для однопоточных версий
    if (policy_flag) {
        sort(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
        
        sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
    }
    
    return query;
}