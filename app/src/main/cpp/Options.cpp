#include "Options.h"
#include "JsonValue.h"
#include "AndroidOut.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace ParseParty;

Options::Options()
{
    this->gravity = 980.0;
    this->bounce = 0.5;
    this->audio = true;
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
        char* optionsJsonBuf = new char[optionsJsonBufSize];
        fseek(fp, 0, SEEK_SET);
        fread(optionsJsonBuf, optionsJsonBufSize, 1, fp);
        bool optionsLoaded = this->LoadFromString(optionsJsonBuf, optionsJsonBufSize);
        delete[] optionsJsonBuf;
        fclose(fp);
        if(optionsLoaded)
            return true;
    }

    aout << "Options file didn't exist or didn't open, so falling back on default options." << std::endl;

    strcpy(optionsFile, "default_options.json");
    AAsset* optionsAsset = AAssetManager_open(app->activity->assetManager, optionsFile, AASSET_MODE_STREAMING);
    if(!optionsAsset)
    {
        aout << "Failed to open default options file." << std::endl;
        return false;
    }

    const char* optionsJsonBuf = (const char*)AAsset_getBuffer(optionsAsset);
    int optionsJsonBufSize = AAsset_getLength(optionsAsset);
    bool optionsLoaded = this->LoadFromString(optionsJsonBuf, optionsJsonBufSize);
    AAsset_close(optionsAsset);
    return optionsLoaded;
}

bool Options::LoadFromString(const char* optionsJsonBuf, int optionsJsonBufSize)
{
    char* safeBuffer = new char[optionsJsonBufSize + 1];
    ::memcpy(safeBuffer, optionsJsonBuf, optionsJsonBufSize);
    safeBuffer[optionsJsonBufSize] = '\0';
    std::string optionsJsonStr(safeBuffer);
    delete[] safeBuffer;

    std::string parseError;
    std::unique_ptr<JsonValue> jsonData(JsonValue::ParseJson(optionsJsonStr, parseError));
    if(!jsonData)
    {
        aout << "Failed to parse options file!" << std::endl;
        aout << "Parse error: " + parseError << std::endl;
        return false;
    }

    auto jsonOptions = dynamic_cast<const JsonObject*>(jsonData.get());
    if(!jsonOptions)
    {
        aout << "Parsed JSON was not an object at its root." << std::endl;
        return false;
    }

    auto jsonGravity = dynamic_cast<const JsonFloat*>(jsonOptions->GetValue("gravity"));
    if(jsonGravity)
        this->gravity = jsonGravity->GetValue();

    auto jsonBounce = dynamic_cast<const JsonFloat*>(jsonOptions->GetValue("bounce"));
    if(jsonBounce)
        this->bounce = jsonBounce->GetValue();

    auto jsonAudio = dynamic_cast<const JsonBool*>(jsonOptions->GetValue("audio"));
    if(jsonAudio)
        this->audio = jsonAudio->GetValue();

    return true;
}