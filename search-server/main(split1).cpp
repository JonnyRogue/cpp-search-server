#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5; //ограничение на вывод кол-ва запросов.


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
};

class SearchServer {
    
   
    
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        if(document.empty()) //исключаем пустые строки.
        
            return;
        
        const vector<string> words = SplitIntoWordsNoStop(document);// разбиваем на слова без  стоп слов.
        double tf;
      
      for(auto word:words) //итерация уже без стоп стоп слов.
        {
            tf = static_cast<double>(1/static_cast<double>(words.size())); // вычисляем TF.
        word_to_document_freqs_[word][document_id]=word_to_document_freqs_[word][document_id]+tf; // Добавляем TF для каждого слова.
    }
        ++document_count_; 
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    

   map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;
    
     int document_count_ = 0; //общее кол-во документов для подсчета IDF.
    
    struct Query {
    set <string> plus_w; //плюс слова Query query_words.plus_w
    set <string> minus_w; //минус слова Query query_words.minus_w
};


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {  // исключаем стоп слова.
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if ( word[0]=='-')
            {
                string minus_minus = word.substr(1);
                if(!IsStopWord(minus_minus)) //условие при отсутствии минуса...
                {
                    query_words.minus_w.insert(minus_minus); //добавляем минус слово
                }
            }
            else query_words.plus_w.insert(word); //добавляем плюс слово
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map <int,double> document_to_relevance;
       
        for(auto pl_w:query_words.plus_w) //итерирование по плюс словам и вычисление TF-IDF.
        {
            if (!word_to_document_freqs_.count(pl_w)) continue;
            double idf = log(static_cast <double>(document_count_/static_cast<double>(word_to_document_freqs_.at(pl_w).size())));
             
                             if (word_to_document_freqs_.count(pl_w))
            {
                for(auto [id,tf]:word_to_document_freqs_.at(pl_w))
                {
                    double idf_tf= idf*tf;
                    document_to_relevance[id] += idf_tf;
                }
            }
        }
        
        for(auto min_w:query_words.minus_w) //итерирование по минус словам.
        {
            if (word_to_document_freqs_.count(min_w))
            {
                for(auto  [id,relevance] :word_to_document_freqs_.at(min_w))
                {
                    document_to_relevance.erase(id); // удаляем id документы с минус словами.
                }
            }
        }
        
        for (const auto& [id,relevance] : document_to_relevance) {
            
                matched_documents.push_back({id, relevance});
            }
        
        return matched_documents; // возврат вектора документов
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
