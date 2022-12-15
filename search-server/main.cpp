/*
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;
*/
/* Подставьте вашу реализацию класса SearchServer сюда */
/*
const int MAX_RESULT_DOCUMENT_COUNT = 5;

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

// Подставьте вашу реализацию класса SearchServer сюда

struct Document {
    int id;
    double relevance;
    int rating;
};
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
class SearchServer {
public:
    void SetStopWords(const string& text) {
        // Ваша реализация данного метода
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        // Ваша реализация данного метода
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        // Ваша реализация данного метода
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        // Ваша реализация данного метода
        return FindTopDocuments(raw_query, [&status](int document_id, DocumentStatus stat, double rating) {
            return status == stat;
        });
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        // Ваша реализация данного метода
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    int GetDocumentCount() const {
        // Ваша реализация данного метода
        return documents_.size();
    }
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        // Ваша реализация данного метода
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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
private:
    // Реализация приватных методов вашей поисковой системы
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate& document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                // Получаем информацию о документе
                const auto& current_document = documents_.at(document_id);
                // Фильтрация документов по заданным критериям
                if (document_predicate(document_id, current_document.status, current_document.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};
*/
// Подставьте сюда вашу реализацию макросов
// ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
/*
template <typename T>
void RunTestImpl(const T& t, const string& func) {
    t();
    cerr << func << " PASSED" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

// Макрос ASSERT_EQUAL & ASSERT_EQUAL_HINT
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

// Макрос ASSERT & ASSERT_HINT
void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
*/
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/

// Тест проверяет, что поисковая система добавляет документ.
// А также находит добавленный документ по поисковому запросу.
void TestAddDocumentAndFindDocumentsBySearchQuery() {
    SearchServer server;
    server.SetStopWords("a in the with"s);
    // Для случайных значений
    srand(time(NULL));
    // Убеждаемся, что добавление документов и поиск работает корректно
    {
        // Диапазон случайных чисел от 1 до 200
        const int random_id = 1 + rand() % 200;
        const string doc_content = "old dog with a gold collar"s;
        // Диапазон случайных чисел для рейтинга от -10 до 10
        const vector<int> doc_ratings = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
        server.AddDocument(random_id, doc_content, DocumentStatus::ACTUAL, doc_ratings);
        const auto found_docs = server.FindTopDocuments("dog"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, random_id);
    }
    // Убеждаемся, что поисковая система находит добавленный документ по поисковому запросу
    {
        const int random_id = 1 + rand() % 200;
        const vector<int> doc_ratings = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
        server.AddDocument(random_id, "red cat with a leash"s, DocumentStatus::ACTUAL, doc_ratings);
        // Проверяем, что поисковая система возвращает пустой результат
        // в случае отсутствия совпадений слов документа по поисковому запросу
        const auto empty_found = server.FindTopDocuments("golden fish"s);
        ASSERT(empty_found.empty());
        // Проверяем, что поисковая система возвращает найденный документ
        const auto found_docs = server.FindTopDocuments("cat with a name leash"s);
        ASSERT_EQUAL(found_docs.size(), 1);
    }
}

// Тест проверяет, что поисковая система исключает документы, содержащие минус-слова поискового запроса.
void TestExcludeDocumentWithMinusWords() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int random_id = 1 + rand() % 200;
    const string doc_content = "red cat with a leash"s;
    const vector<int> doc_ratings = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id, doc_content, DocumentStatus::ACTUAL, doc_ratings);

    const auto found_docs = server.FindTopDocuments("cat -leash"s);
    ASSERT(found_docs.empty());
}

// Тест проверяет матчинг документов в поисковой системе.
// При матчинге должны быть возвращены все слова из поискового запроса, присутствующие в документе.
// Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TesthDocumentsMatchingWithMinusWords() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int random_id = rand() % 1 + 200;
    const string doc_content = "white cat with a fluffy tail"s;
    const vector<int> doc_ratings = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id, doc_content, DocumentStatus::ACTUAL, doc_ratings);

    // Проверяем, что функция возвращает только совпавшие слова в документе
    {
        auto [words, _] = server.MatchDocument("white cat"s, random_id);
        ASSERT_EQUAL(words.size(), 2);
    }
    // Проверяем, что функция возвращает пустой список слов, в случае совпадения минус-слова в документе
    {
        auto [words, _] = server.MatchDocument("white cat -fluffy"s, random_id);
        ASSERT(words.empty());
    }
}

// Тест проверяет сортировку найденных документов по релевантности.
// Возвращаемые документы должны быть отсортированы в порядке убывания релевантности.
void TestSortingFoundDocumentsByRelevance() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int rand_id_1 = 1 + rand() % 10;
    const string doc_content_1 = "white cat with a fluffy tail"s;
    const vector<int> doc_ratings_1 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_1, doc_content_1, DocumentStatus::ACTUAL, doc_ratings_1);

    const int rand_id_2 = 11 + rand() % 20;
    const string doc_content_2 = "red cat with a leash"s;
    const vector<int> doc_ratings_2 = {(rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_2, doc_content_2, DocumentStatus::ACTUAL, doc_ratings_2);

    const int rand_id_3 = 21 + rand() % 30;
    const string doc_content_3 = "old dog with a gold collar"s;
    const vector<int> doc_ratings_3 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_3, doc_content_3, DocumentStatus::ACTUAL, doc_ratings_3);

    // Проверяем, что релевантность отсортирована по убыванию
    const auto found_docs = server.FindTopDocuments("red cat"s);
    ASSERT_EQUAL(found_docs.size(), 2);
    ASSERT_EQUAL(found_docs[0].id, rand_id_2); // Слова - red cat
    ASSERT_EQUAL(found_docs[1].id, rand_id_1); // Слова - cat
}

// Тест проверяет вычисление рейтинга документов.
// Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestDocumentRatingCalculation() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int random_id = 1 + rand() % 200;
    const string doc_content = "goldfish in a carrier"s;
    const vector<int> doc_ratings = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id, doc_content, DocumentStatus::ACTUAL, doc_ratings);

    int sum = 0;
    for (const auto& rt : doc_ratings) {
        sum += rt;
    }
    int avg_rating = sum / static_cast<int>(doc_ratings.size());

    const auto found_docs = server.FindTopDocuments("goldfish"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.rating, avg_rating);
}

// Тест проверяет фильтрацию результатов поиска с использованием предиката, задаваемого пользователем.
void TestFilteringSearchResultsWithPredicate() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int random_id_1 = 1;
    const string doc_content_1 = "goldfish in a carrier"s;
    const vector<int> doc_ratings_1 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id_1, doc_content_1, DocumentStatus::BANNED, doc_ratings_1);

    const int random_id_2 = 2;
    const string doc_content_2 = "dog in the park with a leash"s;
    const vector<int> doc_ratings_2 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id_2, doc_content_2, DocumentStatus::BANNED, doc_ratings_2);

    const int random_id_3 = 3;
    const string doc_content_3 = "bird"s;
    const vector<int> doc_ratings_3 = {7, 9, 9};
    server.AddDocument(random_id_3, doc_content_3, DocumentStatus::ACTUAL, doc_ratings_3);

    // Проверка для идентификатора документа
    const auto found_by_id = server.FindTopDocuments("dog"s, [](int id, DocumentStatus status, int rating) {
        return id % 2 == 0;
        if (status == DocumentStatus::BANNED || rating > 0) {
        }
    });
    ASSERT_EQUAL(found_by_id.size(), 1);
    ASSERT_EQUAL(found_by_id[0].id, random_id_2);

    // Проверка для статуса документа
    const auto banned_status_res = server.FindTopDocuments("goldfish"s, [](int id, DocumentStatus status, int rating) {
        return status == DocumentStatus::BANNED;
        if (id == rating) {
        }
    });
    ASSERT_EQUAL(banned_status_res.size(), 1);
    ASSERT_EQUAL(banned_status_res[0].id, random_id_1);

    // Проверка для рейтинга документа
    const auto found_by_rating = server.FindTopDocuments("bird"s, [](int id, DocumentStatus status, int rating) {
        return rating > 5;
        if (id > 0 || status == DocumentStatus::ACTUAL) {
        }
    });
    ASSERT_EQUAL(found_by_rating.size(), 1);
    ASSERT_EQUAL(found_by_rating[0].id, random_id_3);
}

// Тест проверяет поиск документов, соответствующих заданному статусу.
void TestSearchDocumentsMatchTheSpecifiedStatus() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int random_id_1 = 1 + rand() % 10;
    const string doc_content_1 = "parrot in the park with a leash"s;
    const vector<int> doc_ratings_1 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id_1, doc_content_1, DocumentStatus::BANNED, doc_ratings_1);

    const int random_id_2 = 11 + rand() % 20;
    const string doc_content_2 = "parrot with a fancy bag"s;
    const vector<int> doc_ratings_2 = {(rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(random_id_2, doc_content_2, DocumentStatus::ACTUAL, doc_ratings_2);

    const auto actual_status_res_1 = server.FindTopDocuments("leash"s, DocumentStatus::ACTUAL);
    ASSERT(actual_status_res_1.empty());
    const auto actual_status_res_2 = server.FindTopDocuments("parrot"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(actual_status_res_2.size(), 1);
    ASSERT_EQUAL(actual_status_res_2[0].id, random_id_2);
    const auto actual_status_res_3 = server.FindTopDocuments("parrot"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(actual_status_res_3.size(), 1);
    ASSERT_EQUAL(actual_status_res_3[0].id, random_id_1);
}

// Тест проверяет корректное вычисление релевантности найденных документов.
void TestCorrectCalculationOfTheRelevanceOfFoundDocuments() {
    SearchServer server;
    server.SetStopWords("a in the with"s);

    srand(time(NULL));

    const int rand_id_1 = 1 + rand() % 10;
    const string doc_content_1 = "red cat with a gold leash"s;
    const vector<int> doc_ratings_1 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_1, doc_content_1, DocumentStatus::ACTUAL, doc_ratings_1);

    const int rand_id_2 = 11 + rand() % 20;
    const string doc_content_2 = "white cat with a fluffy tail"s;
    const vector<int> doc_ratings_2 = {(rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_2, doc_content_2, DocumentStatus::ACTUAL, doc_ratings_2);

    const int rand_id_3 = 21 + rand() % 30;
    const string doc_content_3 = "old cat with a cute collar"s;
    const vector<int> doc_ratings_3 = {(rand() % 21 - 10), (rand() % 21 - 10), (rand() % 21 - 10)};
    server.AddDocument(rand_id_3, doc_content_3, DocumentStatus::ACTUAL, doc_ratings_3);

    // Проверяем, что релевантность подсчитывается и отсортирована по убыванию
    const auto found_result = server.FindTopDocuments("old cat with a gold collar"s);
    ASSERT_EQUAL(found_result.size(), 3);
    ASSERT_EQUAL(found_result[0].id, rand_id_3); // old cat collar
    ASSERT_EQUAL(found_result[1].id, rand_id_1); // cat gold
    ASSERT_EQUAL(found_result[2].id, rand_id_2); // cat

    // Проверяем корректное вычисление релевантности
    // Слова встречаются во всех документах: old - 1 / cat - 3 / gold - 1 / collar - 1
    // Для первого результата слова: old / cat / collar
    const double res_1 = (log(3.)*0.25 + log(1.)*0.25 + log(3.)*0.25);
    // Для второго результата слова: cat / gold
    const double res_2 = (log(1.)*0.25 + log(3.)*0.25);
    // Для третьего результата слова: cat
    const double res_3 = (log(1.)*0.25);

    ASSERT(abs(found_result[0].relevance - res_1) < 1e-6);
    ASSERT(abs(found_result[1].relevance - res_2) < 1e-6);
    ASSERT(abs(found_result[2].relevance - res_3) < 1e-6);
}



// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestAddDocumentAndFindDocumentsBySearchQuery);
    RUN_TEST(TestExcludeDocumentWithMinusWords);
    RUN_TEST(TesthDocumentsMatchingWithMinusWords);
    RUN_TEST(TestSortingFoundDocumentsByRelevance);
    RUN_TEST(TestDocumentRatingCalculation);
    RUN_TEST(TestFilteringSearchResultsWithPredicate);
    RUN_TEST(TestSearchDocumentsMatchTheSpecifiedStatus);
    RUN_TEST(TestCorrectCalculationOfTheRelevanceOfFoundDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------
/*
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
*/
