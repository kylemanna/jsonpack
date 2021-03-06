/**
 *  Jsonpack - JSON object representation
 *
 *  Copyright (c) 2015 Yadiel Martinez Gonzalez <ymglez2015@gmail.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef JSONPACK_ARRAY_OBJECT_HPP
#define JSONPACK_ARRAY_OBJECT_HPP

#include <vector>
#include <unordered_map>
#include <string.h>

#ifndef _MSC_VER
#include <bits/hash_bytes.h>
#else
#include <xhash>
#endif

#include "jsonpack/namespace.hpp"

JSONPACK_API_BEGIN_NAMESPACE

/**
 * Represent a keys in the json string
 */
struct key
{

    const char * _ptr;
    std::size_t _bytes;

    bool operator== (const key &k1) const
    {
        std::size_t min =  std::min( k1._bytes, _bytes );
		bool ret = ( memcmp(k1._ptr, _ptr, min) == 0 );
        return ret;
    }

    key():_ptr(nullptr), _bytes(0){}

    key(const key& k):_ptr(k._ptr),_bytes(k._bytes)  {}

};


/**
 * functor to hash keys in unordered_map
 */
struct key_hash
{
    std::size_t operator()( key const &__val) const
    {
#ifndef _MSC_VER
        return std::_Hash_bytes(__val._ptr, __val._bytes, static_cast<std::size_t>(0xc70f6907UL));
#else
		return std::_Hash_seq( (const unsigned char*)__val._ptr, __val._bytes);
#endif

    }

};

/**
 * JSON tokens
 */
enum jsonpack_token_type
{
    JTK_OPEN_KEY = 0,
    JTK_CLOSE_KEY = 1,
    JTK_COLON = 2,
    JTK_COMMA = 3,
    JTK_OPEN_BRACKET = 4,
    JTK_CLOSE_BRACKET = 5,

    JTK_STRING_LITERAL = 6,

    JTK_INTEGER = 7,
    JTK_REAL = 8,

    JTK_TRUE = 9,
    JTK_FALSE = 10,
    JTK_NULL = 11,

    JTK_INVALID = 12
};


/**
 * Position of a value in string json
 */
struct position
{
    jsonpack_token_type _type;
    unsigned long _pos;
    unsigned long _count;


#ifndef _MSC_VER
    position& operator =(const position &rvalue)
    {
        _type = rvalue._type;
        _pos = rvalue._pos;
        _count = rvalue._count;
        return *this;
    }

#endif

};


struct value;

/**
 * Collection of key/value pairs (javascript object)
 */
typedef std::unordered_map<key, value, key_hash >  object_t;

/**
 * Sequence of values
 */
typedef std::vector<value> array_t;

/**
 * For union active field control
 */
enum fields
{
    _POS,
    _OBJ,
    _ARR
};


/**
 * Represent a JSON value. The union wrapper can represents a:
 * - position of: integer, real, UTF-8 string, boolean or null
 * - an array_t
 * - an object_t
 */
struct value
{
    fields _field;

    union
    {
        position   _pos;
        object_t*  _obj;
        array_t*   _arr;
    };

    value& operator =(const value &rv)
    {
        _field = rv._field;
        switch (_field)
        {
        case _POS:
            _pos = rv._pos;
            break;
        case _ARR:
            _arr = rv._arr;
            break;
        default:
            _obj = rv._obj;
            break;
        }

        return *this;
    }
};

//forward
static inline void delete_object(object_t *obj);

/**
 * Function to free array_t
 */
static inline void delete_array(array_t *arr)
{
    for(array_t::iterator elem = arr->begin(); elem != arr->end(); elem++)
    {
        if((*elem)._field == _OBJ)
        {
            delete_object((*elem)._obj);
        }
        else if((*elem)._field == _ARR)
        {
            delete_array((*elem)._arr);
        }
    }

    arr->clear();
    delete arr;
    arr = nullptr;
}

/**
 * Function to free object_t
 */
static inline void delete_object(object_t *obj)
{
    for(object_t::iterator it = obj->begin(); it != obj->end(); it++)
    {
        if(it->second._field == _OBJ)
        {
            delete_object(it->second._obj);
        }
        else if(it->second._field == _ARR)
        {
            delete_array(it->second._arr);
        }
    }

    obj->clear();
    delete obj;
    obj = nullptr;
}

/**
 * Function to free internal elements
 */
static inline void clean_object(object_t & obj)
{
    for(object_t::iterator it = obj.begin(); it != obj.end(); it++)
    {
        if(it->second._field == _OBJ)
        {
            delete_object(it->second._obj);
        }
        else if(it->second._field == _ARR)
        {
            delete_array(it->second._arr);
        }
    }
}

/*
enum value_field
{
    _NULL,
    _STR,
    _BOOL,
    _INTEGER,
    _REAL,
    _ARRAY,
    _OBJECT

};

}*/




JSONPACK_API_END_NAMESPACE

#endif // JSONPACK_ARRAY_OBJECT_HPP

