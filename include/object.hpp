#pragma once

#include "common.hpp"
#include "value.hpp"

enum class ObjType
{
    STRING;
}

struct Obj
{
    ObjType type;
};


struct ObjString final : public Obj
{
    std::string value;

};
