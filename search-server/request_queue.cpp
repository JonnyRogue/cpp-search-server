#include "request_queue.h"


std::vector<Document> RequestQueue::AddFindRequest(std::string_view raw_query, DocumentStatus status) {
        // напишите реализацию
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}
std::vector<Document> RequestQueue::AddFindRequest(std::string_view raw_query) {
        // напишите реализацию
    return AddFindRequest (raw_query, DocumentStatus::ACTUAL);
}
int RequestQueue:: GetNoResultRequests() const {
        // напишите реализацию
    return count_if(requests_.begin(),requests_.end(), [](QueryResult query){return query.query_==0;});
}
