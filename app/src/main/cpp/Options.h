#pragma once

struct android_app;

class Options
{
public:
    Options();
    virtual ~Options();

    bool Load(android_app* app);

private:
    bool LoadFromString(const char* optionsJsonBuf, int optionsJsonBufSize);

    double gravity;
    double bounce;
};