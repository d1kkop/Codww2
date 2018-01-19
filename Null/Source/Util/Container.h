#pragma once

#include "../Common.h"


namespace Null
{
	template <typename T>
	T sign(const T& t)
	{
		if (t > 0) return T(1);
		else if (t < 0) return T(-1);
		else return T(0);
	}

	// ----- Generic Collection -----------------------------------------------------------------------------------------------------------
	template <class Collection>
	u64 length(const Collection& collection)
	{
		return collection.size();
	}

	template <class Collection>
	u64 size(const Collection& collection)
	{
		return length(collection);
	}

	template <class Collection, class Lamda>
	void for_each(Collection& collection, const Lamda& cb)
	{
		for (auto& v : collection)
		{
			cb(v);
		}
	}

	template <class Collection, class Value>
	void add(Collection& collection, Value& v)
	{
		collection.emplace_back(v);
	}

	template <class Collection, class Value>
	bool add_unique(Collection& collection, Value& v)
	{
		if (!contains(collection, v)) add(collection, v);
	}

	template <class Collection, class Value>
	void remove(Collection& collection, const Value& v)
	{
		for (auto it = collection.begin(); it != collection.end(); it++)
		{
			if (v == *it)
			{
				collection.erase(it);
				break;
			}
		}
	}

	template <class Collection, class Value>
	void remove_all(Collection& collection, const Value& v)
	{
		std::remove(collection.begin(), collection.end(), v);
	}

	template <class Collection, class Lamda>
	void remove_if(Collection& collection, const Lamda& cb)
	{
		for (auto it = collection.begin(); it != collection.end(); )
		{
			if (cb(*it)) 
			{
				it = collection.erase(it);
			} else it++;
		}
	}

	template <class Collection, class T>
	bool contains(const Collection& collection, const T& value)
	{
		return std::find(collection.begin(), collection.end(), value) != collection.end();
	}

	/*	Removes duplicate entires. Note that the type must have an '== operator' defined. */
	template <class Collection>
	void unique(Collection& collection)
	{
		std::unique(collection.begin(), collection.end());
	}

	/*  Note that the type must have an '== operator' defined. */
	template <class Collection>
	void unique_copy(const Collection& collection, Collection& newCollection)
	{
		std::unique_copy(collection.begin(), collection.end(), newCollection);
	}

	template <class CollectionA, class CollectionB>
	void to_array(const CollectionA& from, CollectionB& to)
	{
		for (auto& e : from)
		{
			to.emplace_back(e);
		}
	}


	// ----- List -----------------------------------------------------------------------------------------------------------
	template <class T>
	const T* list_at(const List<T>& list, u32 idx)
	{
		if (idx >= list.size()) return nullptr;
		if (idx < list.size()/2)
		{
			for (auto it = list.begin(); it != list.end(); it++, idx--)
			{
				if (idx==0) return &(*it);
			}
		}
		else 
		{
			idx = (u32)list.size() - idx - 1;
			for (auto it = list.rbegin(); it != list.rend(); it++, idx--)
			{
				if (idx==0) return &(*it);
			}
		}
		return nullptr;
	}


	// ----- Map -----------------------------------------------------------------------------------------------------------

	template <typename Collection, typename Key>
	bool map_contains(Collection& map, const Key& key)
	{
		return collection.count(key) != 0;
	}

	template <typename Collection, typename Key, typename Value>
	bool map_insert(Collection& map, const Key& key, const Value& value)
	{
		auto it = map.find(key);
		if (it == map.end()) 
		{
			map.insert(std::make_pair(key, value));
			return true;
		}
		return false;
	}

	template <typename Collection, typename Key>
	void map_remove(Collection& map, const Key& key)
	{
		map.erase(key);
	}

	template <typename Collection, typename Key>
	auto map_get(Collection& map, const Key& key)
	{
		auto it = map.find(key);
		if (it != map.end()) return it->second;
		return (decltype(it->second))nullptr;
	}

	template <typename Collection, typename Key, typename Value>
	bool map_get_value(Collection& map, const Key& key, Value& vOut)
	{
		auto it = map.find(key);
		if (it != map.end()) 
		{
			vOut = it.second;
			return true;
		}
		return false;
	}
}
