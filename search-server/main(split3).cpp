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
    int result;
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
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& stop_word : stop_words_) { 
            if (IsValidWord(stop_word) == false) {  
                throw invalid_argument("Stop words contain invalid characters"s);
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id<0) {
            throw invalid_argument("Document with negative id");
        }
        if (documents_.count(document_id)) {
            throw invalid_argument("Document with this id already exists");
        }
        if (IsValidWord(document) == false) {
            throw invalid_argument("Document contains invalid characters");
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        doc_id.push_back(document_id);
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const { 
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

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
        return  matched_documents;
        
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;});
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        
        if(index < 0 || index > documents_.size()){throw out_of_range("Out of range"s);}
        
        return doc_id.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
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
        auto result = tuple{matched_words, documents_.at(document_id).status};
        return result;
        
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> doc_id;

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
        if(text.empty() == true) {
            throw invalid_argument("Search query is empty"s);
        }
        if (text.back() == '-') {
            throw invalid_argument("Missing text after minus"s);
        }
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if(IsValidWord(text)==false) {
            throw invalid_argument("Invalid characters in the search query"s);
        }
        if (text[0] == '-') {
            throw invalid_argument("More than one minus before words"s); 
        }
        return  {text, is_minus, IsStopWord(text)};
         
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

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double idf_ = IDF(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * idf_;
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

    static bool IsValidWord(const string& word) {
        bool i = false;
        // A valid word must not contain special characters
        i = none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
        return i;
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
void TestExcludedMinudsWords() {
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
    const auto match_word = server.MatchDocument("-on the -street is rainy day"s, 1);
    const auto [str, doc] = match_word;
    ASSERT_HINT(str.empty(), "Matching words not be found");
    const auto match_word2 = server.MatchDocument("-guys -play on street"s, 2);
    const auto [str2, doc2] = match_word;
    ASSERT_HINT(str2.empty(), "Matching words not be found");
}

// Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortByRelevance() {
    SearchServer server;
    server.AddDocument(1, "dog play on the street with ball in rainy day"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(2, "dog play on street"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(3, "black and white dog play with ball"s, DocumentStatus::ACTUAL, { 2,3,4 });
    server.AddDocument(4, "dog and cat hates each other"s, DocumentStatus::ACTUAL, { 2,3,4 });
    const auto found_doc = server.FindTopDocuments("dog play street rainy");
    ASSERT_EQUAL(found_doc.size(), 4);
    ASSERT_EQUAL(found_doc[0].id, 2);
    ASSERT_EQUAL(found_doc[1].id, 1);
    ASSERT_EQUAL(found_doc[2].id, 3);
    ASSERT_EQUAL(found_doc[3].id, 4);
   }

// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestCalculationRatings() {
    SearchServer server;
    server.AddDocument(1, "dog play on the street in rainy day"s, DocumentStatus::ACTUAL, { 1, 1, 1, 1 });
    server.AddDocument(4, "dog and cat hates each other"s, DocumentStatus::ACTUAL, { 4, 4, 4, 4, 4 });
    vector<Document> found_doc = server.FindTopDocuments("dog"s);
    ASSERT_EQUAL_HINT(found_doc[0].rating, (4*5)/5, "Incorrect rating");
    ASSERT_EQUAL_HINT(found_doc[1].rating, (1*4)/4, "Incorrect rating");
    }



// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFindDocumentByPredicate() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    {
        const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus /*status*/, int /*rating*/) { return document_id % 2 == 0; });
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, 0);
    }
}

// Поиск документов, имеющих заданный статус.
void TestFindDocumentByStatus() {
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
    int count_id = 0;
    vector<double> rel = {0.138629};
    for(const auto found_doc : server.FindTopDocuments("black guys"s)){
    ASSERT_HINT(abs((found_doc.relevance) - rel[count_id])<0.001,"Done");
           count_id=count_id +1;
    }
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddedFoundDocument);
    RUN_TEST(TestExcludedMinudsWords);
    RUN_TEST(TestMatched);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestCalculationRatings);
    RUN_TEST(TestFindDocumentByPredicate);
    RUN_TEST(TestFindDocumentByStatus);
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
