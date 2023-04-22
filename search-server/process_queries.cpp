#include <execution>
#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server] (const std::string & query) {return search_server.FindTopDocuments(query);});
    return result;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> documents = ProcessQueries(search_server, queries);
    std::list<Document> result;
    for (const std::vector<Document>& document : documents) {
        result.insert(result.end(), document.begin(), document.end());
    }
    
    return result;
}

/*std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> results(queries.size());
    std::transform(std::execution::par, queries.cbegin(), queries.cend(), results.begin(), [&search_server](std::string const query) {
        return search_server.FindTopDocuments(query);
        });
    return results;
}
 
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::list<Document> result;
    for (auto& docs : ProcessQueries(search_server, queries)) {
        for (auto& doc : docs) {
            result.push_back(doc);
        }
    }
    return result;
} */
