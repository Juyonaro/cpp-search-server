#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;                                        // Общее кол-во возвращаемых результатов

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }

    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words_doc = SplitIntoWordsNoStop(document);        // Убираем стоп-слова из док-та
        const double word_TF = 1. / words_doc.size();                           // Ценность слова документа
        for (const string& word : words_doc) {
            word_in_document_freqs_[word][document_id] += word_TF;              // Если слово раньше не встречалось, то задаем его ценность
        }                                                                       // Если слово снова есть в док-те, увеличиваем его ценность
        ++document_count_;                                                      // Увеличиваем общее кол-во док-тов
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        auto matched_documents = FindAllDocuments(ParseQuery(raw_query));       // Метод поиска релевантных док-тов по 

        sort(matched_documents.begin(), matched_documents.end(),                // Сортировка найденных документов
            [](const Document& lhs, const Document& rhs) {                      // Лямбда-функция сортирующая
                return lhs.relevance > rhs.relevance;                           // по релевантности документов
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {             // При большое кол-ве найденных док-тов
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);                // Отображаем только 5 самых релевантных
        }
        return matched_documents;
    }

private:
    struct Query {                                                              // Используется для обработки запроса пользователя
        set<string> minus_words;                                                // Слова, исключенные из запроса
        vector<string> plus_words;                                              // Слова, необходимые в запросе
    };

    int document_count_ = 0;                                                    // Храним кол-во документов во всей системе
    map<string, map<int, double>> word_in_document_freqs_;                      // Храним слова док-та, id док-та и TF-IDF
    set<string> stop_words_;                                                    // Храним стоп-слова, исключаемые из запроса

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {             // Метод разбивает поисковую строку на отдельные слова без стоп слов
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {                                            // Признак стоп-слова
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(string text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {                 // Разбиваем поисковой запрос на отдельные слова без стоп-слов
            if (word[0] == '-') {                                               // Признак слова исключенного из запроса
                query_words.minus_words.insert(word.substr(1));                 // Добавление в коллекцию минус-слов без минуса перед словом
            }
            else {
                query_words.plus_words.push_back(word);                         // Добавление в коллекцию плюс-слов
            }
        }
        return query_words;                                                     // Возвращаем обработанный поисковой запрос
    }

    double FindWordIDF(const int& count_word) const {                           // Функция вычисления IDF для слов запроса
        return log(static_cast<double>(document_count_) / count_word);
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {         // Метод ищет документы по текстовому запросу
        map<int, double> document_to_relevance;                                 // Храним id и релевантности TF-IDF найденных док-тов
        for (const string& p_word : query_words.plus_words) {                   // Проходим по всем плюс-словам из запроса
            if (word_in_document_freqs_.count(p_word) == 0) {                   // В документе может не быть слова из запроса
                continue;
            }
            const double word_IDF = FindWordIDF(word_in_document_freqs_.at(p_word).size()); // Получаем значение IDF слова из запроса 
            for (const auto& [doc_id, word_TF] : word_in_document_freqs_.at(p_word)) {
                document_to_relevance[doc_id] += word_TF * word_IDF;            // Накапливаем значение релевантности по TF-IDF для док-та
            }
        }
        for (const string& word : query_words.minus_words) {                    // Проходим по исключенным словам из запроса
            if (word_in_document_freqs_.count(word) == 0) {                     // В документе может не быть слова из запроса
                continue;
            }
            for (const auto& [doc_id, content] : word_in_document_freqs_.at(word)) { // Ищем исключенные слова в документах
                document_to_relevance.erase(doc_id);                            // Убираем док-т из выдачи результатов
            }                                                                   // если в нем есть исключенное слово
        }
        vector<Document> matched_documents;
        for (const auto& [doc_id, relevance] : document_to_relevance) {         // Получаем значения найденных документов
            matched_documents.push_back({ doc_id, relevance });                 // Добавляем значения в результирующий набор
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}