#include "remove_duplicates.h"

using namespace std::string_literals;
 
void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> id_del;
    std::set<std::set<std::string>> unique_words_;
    for (const int doc_id : search_server) {
        std::map<std::string, double> all_words;
        
        all_words = search_server.GetWordFrequencies(doc_id);
        std::set<std::string> unique_words;
        
        for (auto [word, _] : all_words) {
            unique_words.insert(word);
        }
       
        //Не могу додумать, какую операцию проделать... help)
        //std::transform(all_words.begin(),all_words.end(),unique_words.begin(), [] 
        
        if (unique_words_.count(unique_words)) {
            id_del.insert(doc_id);
        } else {
            unique_words_.insert(unique_words);
        }
    }
    for (auto id: id_del) {
        std::cout<<"Found duplicate document id "<< id <<std::endl;
        search_server.RemoveDocument(id);
    }
}
