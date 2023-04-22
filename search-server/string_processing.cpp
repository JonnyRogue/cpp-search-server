#include "string_processing.h"
/*std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
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
}*/
 
std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    std::string_view space = " ";
    int64_t start = text.find_first_not_of(space);
    const int64_t end = text.npos;
    while (start != end) {
        int64_t space_ = text.find(' ', start);
        words.push_back(space_ == end ? text.substr(start) : text.substr(start, space_ - start));
        start = text.find_first_not_of(space, space_);
    }
    return words;
}
