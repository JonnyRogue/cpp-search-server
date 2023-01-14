#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <functional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5; //ограничение на вывод кол-ва запросов.

constexpr auto ten_min_six_degree = 10e-6;


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

vector<string> SplitIntoWords(const string& text) {  //вектор дробления на слова.
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
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key) //поиск отсортированных документов с шаблоном.
                                      const  {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < ten_min_six_degree) {
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
    
        vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus stat) const {
        return FindTopDocuments(raw_query, [stat](int, DocumentStatus status, int) { return status == stat; });
    }   //если статус  указан.
    
        vector<Document> FindTopDocuments(const string& raw_query) const {
         return FindTopDocuments(raw_query, DocumentStatus status) { return status == DocumentStatus::ACTUAL; });
    } 

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
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
        int rating_sum = accumulate(ratings.begin(),ratings.end(),0);
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

    
    double IDF(const string& word) const {  // ф-ция IDF
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper key) const { // поиск документов с шаблонном
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double idf = IDF(word);
            for (const auto [document_id, tf] : word_to_document_freqs_.at(word)) { // итерирование по плюс словам с шаблоной функцией и вычисление TFIDF
                if (key(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += tf * idf;
                }
            }
        }
 
        for (const string& word : query.minus_words) { //итерирование по минус словам
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
 
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

/*  Подставьте сюда вашу реализацию макросов 
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
/*
template <typename T>
void AssertImpl(const T& expr, const string& expr_str, const string& file, const string& func, const int& line, const string& hint){
    if(!expr){
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()){
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();        
    } 
}
 
#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
 
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
 
template <typename T>
void RunTestImpl(const T& t, const string& t_str) {
 
    t();
    cerr << t_str << " OK"s << endl;
} 
 
#define RUN_TEST(func) RunTestImpl((func), #func) 
 
*/




// -------- Начало модульных тестов поисковой системы ----------


void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
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
// Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddedFoundDocument() {
    SearchServer server;
    server.AddDocument(1, "on the street is rainy day"s, DocumentStatus::ACTUAL, { 8, -3, 2 });
    server.AddDocument(2, "guys play on street"s, DocumentStatus::ACTUAL, { 7, 2, 12 });
    server.AddDocument(3, "black and white dog plays with ball"s, DocumentStatus::ACTUAL, { 5, 12, -4 });
    server.AddDocument(4, "dog and cat hates each other"s, DocumentStatus::ACTUAL, { 9, 2 });
    const auto found_doc = server.FindTopDocuments("black cat play");
    ASSERT_EQUAL_HINT(found_doc.size(), 3, "The number of matching documents is incorrect");
}

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void ExcludedMinudsWords() {
    SearchServer server;
    server.AddDocument(1, "on the street is rainy day"s, DocumentStatus::ACTUAL, { 8, -3, 2 });
    server.AddDocument(2, "guys play on street"s, DocumentStatus::ACTUAL, { 7, 2, 12 });
    const auto found_doc = server.FindTopDocuments("-guys -play day");
    ASSERT_EQUAL_HINT(found_doc.size(), 1, "One document must be found");
    ASSERT_EQUAL_HINT(found_doc[0].id, 1, "Document id does not match");
}

// Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatched() {
    SearchServer server;
    server.AddDocument(1, "on the street is rainy day"s, DocumentStatus::ACTUAL, { 8, -3, 2 });
    server.AddDocument(2, "guys play on street"s, DocumentStatus::ACTUAL, { 7, 2, 12 });
    server.AddDocument(3, "black and white dog plays with ball"s, DocumentStatus::ACTUAL, { 5, 12, -4 });
    const auto found_doc = server.FindTopDocuments("guys rainy street -dog"s);
    ASSERT_EQUAL_HINT(found_doc.size(), 2, "The number of matching documents is incorrect");
    ASSERT_EQUAL(found_doc[0].id, 2);
}

// Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestRelevance() {
    SearchServer server;
    server.AddDocument(1, "dog play on the street with ball in rainy day"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(2, "dog play on street"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(3, "black and white dog play with ball"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(4, "dog and cat hates each other"s, DocumentStatus::ACTUAL, { 2,3,4 });
    const auto found_doc = server.FindTopDocuments("dog play street rainy");
    ASSERT_EQUAL(found_doc.size(), 4);
    ASSERT_EQUAL(found_doc[0].id, 2);
}

// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestRatings() {
    SearchServer server;
    server.AddDocument(1, "dog play on the street in rainy day"s, DocumentStatus::ACTUAL, { 1, 1, 1,1 });
    server.AddDocument(4, "dog and cat hates each other"s, DocumentStatus::ACTUAL, { 4,4,4,4,4 });
    vector<Document> found_doc = server.FindTopDocuments("dog"s);
    ASSERT_EQUAL_HINT(found_doc[0].rating, 4, "Incorrect rating");
    ASSERT_EQUAL_HINT(found_doc[1].rating, 1, "Incorrect rating");
}



// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredicate() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    {
        const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 1);
    }
    {
        const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 3);
    }
    {
        const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus /*status*/, int /*rating*/) { return document_id % 2 == 0; });
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 0);
    }
}

// Поиск документов, имеющих заданный статус.
void TestStatus() {
    SearchServer server;
    server.AddDocument(0, "black dog play on the street in rainy day"s, DocumentStatus::ACTUAL, { 8, -3, 2 });
    server.AddDocument(1, "black guys play on street"s, DocumentStatus::IRRELEVANT, { 7, 2, -12 });
    server.AddDocument(2, "black and white dog plays with ball"s, DocumentStatus::BANNED, { 5, 12, -4 });
    server.AddDocument(3, "black dog and cat hates each other"s, DocumentStatus::REMOVED, { 9, 2 });
    {
        const auto found_doc = server.FindTopDocuments("black"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(found_doc[0].id, 2, "Document id has incorrect status");
    }
    {
        const auto found_doc = server.FindTopDocuments("black"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL_HINT(found_doc[0].id, 1, "Document id has incorrect status");
    }
    {
        const auto found_doc = server.FindTopDocuments("black"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL_HINT(found_doc[0].id, 3, "Document id has incorrect status");
    }
    {
        const auto found_doc = server.FindTopDocuments("black"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL_HINT(found_doc[0].id, 0, "Document id has incorrect status");
    }
}

// Корректное вычисление релевантности найденных документов
void TestCorrectRelevance() {
    SearchServer server;
    server.AddDocument(0, "black dog play on the street in rainy day"s, DocumentStatus::ACTUAL, { 8, 8, 8 });
    server.AddDocument(1, "black guys play on street"s, DocumentStatus::ACTUAL, { 8, 8, 8 });
    const auto found_doc = server.FindTopDocuments("black guys"s);
    int i = found_doc[0].relevance * 10000000;
    ASSERT_EQUAL_HINT(i, 1386294, "Relevance does not match"");
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddedFoundDocument);
    RUN_TEST(ExcludedMinudsWords);
    RUN_TEST(TestMatched);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestRatings);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCorrectRelevance);
    // Не забудьте вызывать остальные тесты здесь
}
// --------- Окончание модульных тестов поисковой системы -----------



int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
} 
