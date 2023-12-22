#include "Options.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Options::Options()
{
    this->gravity = 0.0;
    this->bounce = 0.0;
}

/*virtual*/ Options::~Options()
{
}

bool Options::Load(android_app* app)
{
    const char* dataFolder = app->activity->internalDataPath;
    char optionsFile[256];
    sprintf(optionsFile, "%s/options.json", dataFolder);
    FILE* fp = fopen(optionsFile, "r");
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        size_t optionsJsonBufSize = ftell(fp);
        char* optionsJsonBuf = new char[optionsJsonBufSize + 1];
        fseek(fp, 0, SEEK_SET);
        fread(optionsJsonBuf, optionsJsonBufSize, 1, fp);
        optionsJsonBuf[optionsJsonBufSize] = '\0';
        bool optionsLoaded = this->LoadFromString(optionsJsonBuf, optionsJsonBufSize);
        delete[] optionsJsonBuf;
        fclose(fp);
        if(optionsLoaded)
            return true;
    }

    strcpy(optionsFile, "default_options.json");
    AAsset* optionsAsset = AAssetManager_open(app->activity->assetManager, optionsFile, AASSET_MODE_STREAMING);
    if(!optionsAsset)
        return false;

    const char* optionsJsonBuf = (const char*)AAsset_getBuffer(optionsAsset);
    int optionsJsonBufSize = AAsset_getLength(optionsAsset);
    return this->LoadFromString(optionsJsonBuf, optionsJsonBufSize);
}

bool Options::LoadFromString(const char* optionsJsonBuf, int optionsJsonBufSize)
{
    // TODO: Load from string here.
    return true;
}