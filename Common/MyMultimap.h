
#ifndef __HASH__MAP__
#define __HASH__MAP__

#include "MemTLS.h"
#include "../NetServer/NetEnums.h"
#include <functional>

using namespace std;

namespace server_baby
{

    constexpr int DATA_MAX_SIZE = 65536;
    constexpr int DATA_MAX_SIZE_MASK = 65535;

    template <typename Key, typename Value>
    class HashNode 
    {
    public:
        HashNode(const Key& key, const Value& value) :
            key_(key), value_(value), next_(nullptr) {}
    
    public:
        HashNode* next_;
        Value value_;
        Key key_;

    };

    template <typename Key>
    struct KeyHash 
    {
        unsigned long operator()(const Key& key) const
        {
            unsigned long keyCopy = static_cast<unsigned long>(key);
            return keyCopy & DATA_MAX_SIZE_MASK;
        }
    };

    template <typename Key, typename Value>
    class MyMultimap 
    {
    public:
        MyMultimap() 
        {}

        virtual ~MyMultimap() 
        {
            for (int i = 0; i < DATA_MAX_SIZE; ++i) 
            {
                HashNode<Key, Value>* entry = table_[i];
                while (entry != nullptr) 
                {
                    HashNode<Key, Value>* prev = entry;
                    entry = entry->next_;
                    
                    nodePool_->Free(prev);
                }
                table_[i] = nullptr;
            }
        }

        bool Get(const Key& key, Value& value) 
        {
            HashNode<Key, Value>* entry = table_[key];

            if (!entry)
                return false;

            value = entry->value_;
            return true;
        }

        void ForeachForSameKey(std::function<void(Key, Value)> func, const Key& key)
        {
            HashNode<Key, Value>* entry = table_[key];

            if (!entry)
                return;

            HashNode<Key, Value>* cursor = entry;
            while (cursor != NULL)
            {
                func(cursor->key_, cursor->value_);
                cursor = cursor->next_;
            }

        }

        void Foreach(std::function<void(Key, Value)> func)
        {
            for (int i = 0; i < DATA_MAX_SIZE; i++)
            {
                ForeachForSameKey(func, i);
            }

        }

        int SizeForSameKey(const Key& key)
        {

            int size = NULL;
            HashNode<Key, Value>* entry = table_[key];

            if (!entry)
                return size;

            HashNode<Key, Value>* cursor = entry;
            while (cursor != nullptr)
            {
                cursor = cursor->next_;
                size++;
            }

            return size;
        }

        void Put(const Key& key, const Value& value) 
        {

            if (key > DATA_MAX_SIZE_MASK)
                CrashDump::Crash();

            HashNode<Key, Value>* entry = table_[static_cast<int>(key)];
            HashNode<Key, Value>* newEntry = nodePool_->Alloc();
            newEntry->key_ = key;
            newEntry->value_ = value;
            newEntry->next_ = nullptr;

            if (!entry)
            {
                table_[key] = newEntry;
                return;
            }

            while (entry->next_ != nullptr) 
            {
                entry = entry->next_;
            }

            entry->next_ = newEntry; 
        }

        bool Remove(const Key& key, const Value& value)
        {
            HashNode<Key, Value>* prev = nullptr;
            HashNode<Key, Value>* entry = table_[key];

            if (!entry)
                return false;

            while (entry->next_ != nullptr && entry->value_ != value)
            {
                prev = entry;
                entry = entry->next_;
            }

            if (entry->value_ != value)
                return false; 
            
            if (table_[key] == entry)
            {
                if (entry->next_)
                    table_[key] = entry->next_;
                else
                    table_[key] = nullptr;
            }

            if (prev)
                prev->next_ = entry->next_;

            nodePool_->Free(entry);
            return true;

        }

        bool RemoveAllForSameKey(std::function<void(Key, Value)> func, const Key& key)
        {

            HashNode<Key, Value>* prev = nullptr;
            HashNode<Key, Value>* entry = table_[key];

            if (!entry)
                return false;

            while (entry != nullptr)
            {
                prev = entry;
                entry = entry->next_;
                func(prev->key_, prev->value_);
                nodePool_->Free(prev);
            }

            table_[key] = nullptr;

            return true;
        }

        void Clear()
        {
            for (int i = 0; i < DATA_MAX_SIZE; ++i)
            {
                table_[i] = nullptr;
            }
        }

    private:
        HashNode<Key, Value>* table_[DATA_MAX_SIZE] = { 0 };
        static MemTLS<HashNode<Key, Value>>* nodePool_; 
    };

    template<typename Key, typename Value>
    MemTLS<HashNode<Key, Value>>* MyMultimap<Key, Value>::nodePool_ = new MemTLS<HashNode<Key, Value>>(100, 1, eHASH_MAP_CODE);

}
#endif