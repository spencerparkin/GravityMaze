#pragma once

struct android_app;

class Progress
{
public:
    Progress();
    virtual ~Progress();

    bool Load(android_app* app);
    bool Save(android_app* app);

    void Reset();

    int GetLevel() const;
    void SetLevel(int level);

private:

    void GetProgressFilepath(android_app* app, char* progressFilepath, int progressFilepathSize);

    int level;
};