#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <mutex>
#include <vector>

#include "log_duration.h"
#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Mutexx {
        std::mutex mutex_;
        std::map<Key, Value> map_;
    };
    std::vector<Mutexx> mutexx;
    
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guardd;
        Value& ref_to_value;
        
        
        Access(const Key& key, Mutexx& mutexx) : guardd(mutexx.mutex_), ref_to_value(mutexx.map_[key]) {}

        // ...
    };

    explicit ConcurrentMap(size_t bucket_count) : mutexx(bucket_count) {}

    Access operator[](const Key& key) {
        auto& mut_ = mutexx[uint64_t(key)%mutexx.size()];
        return {key, mut_};
    }
    
    void erase(const Key& key){    
      auto& mut_ = mutexx[static_cast<uint64_t>(key) % mutexx.size()];
        std::lock_guard guard(mut_.mutex_);
        mut_.map_.erase(key);
    }
    
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex_, map_] : mutexx) {
            std::lock_guard guard(mutex_);
            result.insert(map_.begin(), map_.end());
        }
        return result;
    }
};
