#pragma once

#include <algorithm>
#include <map>
#include <vector>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
public:

    struct Mutex_Map
    {
        std::mutex defender_;
        std::map<Key, Value> sub_map_;
    };

    struct Access {

        Access(const Key& key, Mutex_Map& mutex_n_dict) : value_guard_(mutex_n_dict.defender_), ref_to_value(mutex_n_dict.sub_map_[key])
        {
        }

        void operator +=(const Value& value)
        {
            ref_to_value += value;
        }

        std::lock_guard<std::mutex> value_guard_;
        Value& ref_to_value;

    };

    ConcurrentMap() : map_storage_(10)
    {
    }

    explicit ConcurrentMap(size_t bucket_count) : map_storage_(bucket_count)
    {
    }

    Access operator[](const Key& key)
    {
        Mutex_Map& place = map_storage_[uint64_t(key) % map_storage_.size()];
        return { key, place };
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> ordinary_map;
        for (size_t i = 0; i < map_storage_.size(); ++i)
        {
            std::lock_guard tmp(map_storage_[i].defender_);
            auto& ref = map_storage_[i].sub_map_;
            ordinary_map.insert(ref.begin(), ref.end());
        }
        return ordinary_map;
    }

    auto erase(const Key& key)
    {
        Mutex_Map& place = map_storage_[uint64_t(key) % map_storage_.size()];
        std::lock_guard def(place.defender_);
        return place.sub_map_.erase(key);
    }

private:

    std::vector<Mutex_Map> map_storage_;
};