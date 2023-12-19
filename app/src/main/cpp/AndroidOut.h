#pragma once

#include <android/log.h>
#include <sstream>

extern std::ostream aout;

class AndroidOut : public std::stringbuf
{
public:
    AndroidOut(const char* logTag);

protected:
    virtual int sync() override;

private:
    const char* logTag;
};