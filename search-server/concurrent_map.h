#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <string>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex; 
        std::map<Key, Value> data;
    };

    std::vector<Bucket> buckets_;

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard <std::mutex> guard;
        Value& ref_to_value;
        
        Access(const Key& key, Bucket& bucket) : guard(bucket.mutex), ref_to_value(bucket.data[key]) {}
    };

    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {}

    Access operator[](const Key& key) {
        auto& bucket = buckets_[key % buckets_.size()];
        
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        
        for (auto& [mutex, data] : buckets_) {
            std::lock_guard guard(mutex);
            result.insert(data.begin(), data.end());
        }
        
        return result;
    }
    
    void erase(const Key& key) {
		uint64_t index = static_cast<uint64_t>(key) % buckets_.size(); 
        
		std::lock_guard guard(buckets_[index].mutex); 
        
		buckets_[index].data.erase(key); 
	}
};