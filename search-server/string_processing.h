#pragma once
#include <set>
#include <vector>
#include <string>

std::vector<std::string_view> SplitIntoWords(const std::string_view text);
 
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    std::string storage;
    for (auto& stor : strings) {
        storage = stor;
        if (!storage.empty()) {
            non_empty_strings.insert(storage);
        }
    }
    return non_empty_strings;
}
