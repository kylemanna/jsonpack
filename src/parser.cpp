﻿
/**
 *  Jsonpack - JSON syntax parser
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


#define STRINGIFY(str) #str

#define JSONPACK_ERROR_UNEXPECTED( expect, found, pos ) \
    std::string("Expected ") + STRINGIFY(expect) + std::string(", and found ") \
    + STRINGIFY(found) + std::string(" at ") + STRINGIFY(pos)

#include <string.h>

#include "jsonpack/exceptions.hpp"
#include "jsonpack/parser.hpp"



JSONPACK_API_BEGIN_NAMESPACE


static const std::string token_str[] =
{
    "{",
    "}",
    ":",
    ",",
    "[",
    "]",
    "String Literal",

    "Integer",
    "Real",

    "BOOLEAN",
    "BOOLEAN",
    "NULL",

    "Invalid Token"
};



/** ****************************************************************************
 ******************************** SCANER ***************************************
 *******************************************************************************/

void scanner::init(const char* json, const std::size_t &len)
{
    _size = len;
    if(_size > 0)
    {
        _source = (_size != 0) ? json : nullptr;
        _i = 0;
        _c = _source[0];
    }
    else
    {
        throw invalid_json("Empty json string");
    }
}

void scanner::advance()
{
    _c = _source[++_i];
}

jsonpack_token_type scanner::next()
{
    switch ( _c )
    {
    case '{':
        advance();
        return JTK_OPEN_KEY;
        break;
    case '}':
        advance();
        return JTK_CLOSE_KEY;
        break;
    case '[':
        advance();
        return JTK_OPEN_BRACKET;
        break;
    case ']':
        advance();
        return JTK_CLOSE_BRACKET;
        break;
    case ':':
        advance();
        return JTK_COLON;
        break;
    case ',':
        advance();
        return JTK_COMMA;
        break;
    case '"':
        return string_literal();
        break;

    default:
        if(std::isspace(  _c ) )
        {
            advance();
            return next();
        }
        else
        {
            return other_value();
        }
    }
}

jsonpack_token_type scanner::string_literal()
{
    _start_token_pos = _i;
    advance();

    while( _c != '"' && _i < _size )
    {
        advance();
    }

    if( _i < _size - 1 )//last char must be '}' or ']'
    {
        advance();
        return JTK_STRING_LITERAL;
    }

    return JTK_INVALID;
}

jsonpack_token_type scanner::other_value()
{
    if(std::isdigit( _c ) || _c == '+' || _c == '-' )
        return number();

    if(std::isalpha( _c ) )
    {
        std::string lex = "";
        lex.reserve(8);
        _start_token_pos = _i;
        do
        {
            lex += _c;
            advance();
        }
        while( std::isalpha(_c) && _i < _size );

        if(lex.compare("true") == 0 || lex.compare("TRUE") == 0 )
            return JTK_TRUE;
        if(lex.compare("false") == 0 || lex.compare("FALSE") == 0 )
            return JTK_FALSE;
        if(lex.compare("null") == 0 || lex.compare("NULL") == 0 )
            return JTK_NULL;
    }

    return JTK_INVALID;

}

jsonpack_token_type scanner::number()
{
    register uint_fast8_t state = 1;
    _start_token_pos = _i;

    if(_c == '+' || _c == '-')
        state = 0;

    while (true)
    {
        switch (state)
        {
        case 0:
            advance();
            if( std::isdigit( _c ) ) state = 1;
            else return JTK_INVALID;
            break;
        case 1:
            do
            {
                advance();
            }
            while( std::isdigit( _c ) && _i < _size );

            if(_c == '.') state = 2;
            else return JTK_INTEGER;

            break;
        case 2:
            advance();
            if( std::isdigit(_c) ) state = 3;
            else return JTK_INVALID;
            break;
        case 3:
            do
            {
                advance();
            }
            while( std::isdigit(_c) && _i < _size );

            if(_c == 'e' || _c == 'E') state = 4;
            else return JTK_REAL;

            break;
        case 4:
            advance();
            if(_c == '+' || _c == '-') state = 5;
            else if(std::isdigit(_c)) state = 6;
            else return JTK_INVALID;

            break;
        case 5:
            advance();
            if(std::isdigit(_c)) state = 6;
            else return JTK_INVALID;

            break;
        case 6:
            do
            {
                advance();
            }
            while( std::isdigit(_c) && _i < _size );

            return JTK_REAL;

            break;
        default:
            return JTK_INVALID;
            break;
        }
    }
}


key scanner::get_last_key( bool expect_str_literal = false )
{
    char * lex = const_cast<char*>(_source)  ;
    lex += _start_token_pos + expect_str_literal;

    key k;
    k._ptr = lex;
    k._bytes = _i - _start_token_pos - 2*expect_str_literal;

    return k;

}

value scanner::get_last_value(bool expect_str_literal = false)
{
    value p;
    position vpos;
    vpos._pos = _start_token_pos + expect_str_literal;
    vpos._count = _i - _start_token_pos - 2*expect_str_literal;
    vpos._type = JTK_INVALID; //avoiding initializer warnig

    p._pos = vpos;
    p._field = _POS;

    return p;
}


/** ****************************************************************************
 ******************************** PARSER ***************************************
 *******************************************************************************/

jsonpack_token_type parser::_tk;
scanner parser::_s;
std::string parser::error_;

//---------------------------------------------------------------------------------------------------
bool parser::match(const jsonpack_token_type &token)
{
    register bool ok = (_tk == token);
    advance();
    if(!ok)
	{
        error_ = "Expect \"";
		error_.append( token_str[token] );

		error_.append("\", but found \""); 
		error_.append( token_str[_tk] );
		error_.append("\"") ;
	}
    return ok;
}


//---------------------------------------------------------------------------------------------------
void parser::advance()
{
    _tk = _s.next();
}

//---------------------------------------------------------------------------------------------------
bool parser::json_validate(const char *json,const std::size_t &len, object_t &members )
{

    error_ = "";

    _s.init(json, len);
    advance();

    return match(JTK_OPEN_KEY) && item_list(members) && match(JTK_CLOSE_KEY);
}

//---------------------------------------------------------------------------------------------------
bool parser::json_validate(const char *json,const std::size_t &len, array_t &elemets )
{
    error_ = "";

    _s.init(json, len);
    advance();

    return match(JTK_OPEN_BRACKET) && array_list(elemets) && match(JTK_CLOSE_BRACKET);
}

//---------------------------------------------------------------------------------------------------
bool parser::item_list(object_t &members)
{
    if(_tk == JTK_CLOSE_KEY)
        return true;

    if( item(members) )
    {
        if(_tk == JTK_COMMA )
        {
            advance();
            return item_list(members);
        }
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------
bool parser::item(object_t &members)
{
    if( _tk == JTK_STRING_LITERAL ) //key
    {
        /**
         * Get current object key
         */
        key k = _s.get_last_key(true);

        advance();
        return match(JTK_COLON) && value(k, members); // : value
    }
		
    error_ = "Expect key \"";
	error_.append( token_str[JTK_STRING_LITERAL] );

	error_.append("\", but found \""); 
	error_.append( token_str[_tk] );
	error_.append("\"") ;

    return false;
}

//---------------------------------------------------------------------------------------------------
bool parser::value(key k, object_t &members)
{
    if( _tk == JTK_INTEGER ||
            _tk == JTK_REAL ||
            _tk == JTK_STRING_LITERAL ||
            _tk == JTK_TRUE ||
            _tk == JTK_FALSE ||
            _tk == JTK_NULL ) //literals
    {

        /**
         * Add value into the map with current key
         */
        jsonpack::value val = _s.get_last_value(_tk == JTK_STRING_LITERAL);
        val._pos._type = _tk;
        members[k] = val;
//        members.emplace_hint(members.end(), k, val);

        advance();
        return true;
    }

    if( _tk == JTK_OPEN_KEY )
    {
        advance();

        object_t* new_obj = new object_t();  // create obj

        register bool object_ok = item_list(*new_obj);          //fill obj

        if(object_ok)
        {
            jsonpack::value p;                                  //create value width field _obj
            p._obj = new_obj;
            p._field = _OBJ;

            members[k] = p;                                     // add to the map
//            members.emplace_hint(members.end(), k, p);

            return match(JTK_CLOSE_KEY);
        }

        delete_object(new_obj);
        return false;
    }

    if( _tk == JTK_OPEN_BRACKET )
    {
        advance();

        array_t* new_array = new array_t();   // create arr

        register bool array_ok = array_list(*new_array);         // fill arr
        if(array_ok)
        {
            jsonpack::value p;                                  //create value width field _arr
            p._arr = new_array;
            p._field = _ARR;

            members[k] = p;                                     // add to the map
//            members.emplace_hint(members.end(), k, p);

            return match(JTK_CLOSE_BRACKET);
        }
        delete_array(new_array);
        return false;
    }

	error_ = "Expect valid JSON value , but found \"";
	error_.append( token_str[_tk] );
	error_.append("\"") ;

    return false;
}

//---------------------------------------------------------------------------------------------------
bool parser::array_list(array_t &elemets)
{
    if(_tk == JTK_CLOSE_BRACKET)
        return true;

    if( value(elemets) )
    {
        if(_tk == JTK_COMMA )
        {
            advance();
            return array_list(elemets);
        }
        return true;
    }
    return false;

}

//---------------------------------------------------------------------------------------------------
bool parser::value( array_t &elemets)
{
    if( _tk == JTK_INTEGER ||
            _tk == JTK_REAL ||
            _tk == JTK_STRING_LITERAL ||
            _tk == JTK_TRUE ||
            _tk == JTK_FALSE ||
            _tk == JTK_NULL ) //literals
    {

        /**
         * Add value into the vector
         */
        jsonpack::value vpos = _s.get_last_value(_tk == JTK_STRING_LITERAL);
        vpos._pos._type = _tk;
        elemets.push_back(vpos);
//        elemets.emplace_back(vpos);

        advance();
        return true;
    }

    if( _tk == JTK_OPEN_KEY )
    {
        advance();

        object_t* new_obj = new object_t();  // create obj

        register bool object_ok = item_list(*new_obj);          //fill obj

        if(object_ok)
        {
            jsonpack::value p;                                  //create value width field _obj
            p._obj = new_obj;
            p._field = _OBJ;

            elemets.push_back(p);                               // add to the map
//            elemets.emplace_back(p);

            return match(JTK_CLOSE_KEY);
        }

        delete_object(new_obj);
        return false;
    }

    if( _tk == JTK_OPEN_BRACKET )
    {
        advance();

        array_t* new_array = new array_t();   // create arr

        register bool array_ok = array_list(*new_array);         // fill arr
        if(array_ok)
        {
            jsonpack::value p;                                  //create value width field _arr
            p._arr = new_array;
            p._field = _ARR;

            elemets.push_back(p);                               // add to the vector
//            elemets.emplace_back(p);

            return match(JTK_CLOSE_BRACKET);
        }
        delete_array(new_array);
        return false;
    }

	error_ = "Expect valid JSON value , but found \"";
	error_.append( token_str[_tk] );
	error_.append("\"") ;

    return false;
}


JSONPACK_API_END_NAMESPACE
