#pragma once
#include "search_server.h"
#include <deque>
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) :server(search_server){}
        // напишите реализацию
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
    
    private:
    struct QueryResult {
        // определите, что должно быть в структуре
        bool query_;
    };
    const SearchServer& server;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
   
    
}; 
template <typename DocumentPredicate>
    std:: vector<Document> RequestQueue:: AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        // напишите реализацию
        std::vector<Document>  doc = server.FindTopDocuments(raw_query,document_predicate);
        
        QueryResult query;
        query.query_ = (doc.empty() == false);
        
        if(!(requests_.size()<min_in_day_)) {
        requests_.pop_front();
        } 
        requests_.push_back(query);
        return doc;
    }
