#include "Progress.h"
#include "AndroidOut.h"
#include "JsonValue.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace ParseParty;

Progress::Progress()
{
    this->Reset();
}

/*virtual*/ Progress::~Progress()
{
}

void Progress::Reset()
{
    this->level = 0;
    this->touches = 0;
    this->seedModifier = (int)::time(nullptr);
}

bool Progress::Load(android_app* app)
{
    char progressFile[256];
    this->GetProgressFilepath(app, progressFile, sizeof(progressFile));
    FILE* fp = fopen(progressFile, "r");
    if(!fp)
    {
        this->Reset();
        return this->Save(app);
    }

    fseek(fp, 0, SEEK_END);
    size_t progressJsonBufSize = ftell(fp);
    char* progressJsonBuf = new char[progressJsonBufSize + 1];
    fseek(fp, 0, SEEK_SET);
    fread(progressJsonBuf, progressJsonBufSize, 1, fp);
    progressJsonBuf[progressJsonBufSize] = '\0';
    std::string parseError;
    std::unique_ptr<JsonValue> jsonData(JsonValue::ParseJson(progressJsonBuf, parseError));
    delete[] progressJsonBuf;
    fclose(fp);

    if(!jsonData)
    {
        aout << "Failed to parse progress file!" << std::endl;
        aout << "Parse error: " + parseError << std::endl;
        return false;
    }

    JsonObject* jsonObject = dynamic_cast<JsonObject*>(jsonData.get());
    if(!jsonObject)
    {
        aout << "Expected root of progress JSON to be an object." << std::endl;
        return false;
    }

    JsonInt* jsonLevel = dynamic_cast<JsonInt*>(jsonObject->GetValue("level"));
    if(!jsonLevel)
    {
        aout << "No level found in progress object." << std::endl;
        return false;
    }

    JsonInt* jsonTouches = dynamic_cast<JsonInt*>(jsonObject->GetValue("touches"));
    if(!jsonTouches)
    {
        aout << "No touches found in progress object." << std::endl;
        return false;
    }

    JsonInt* jsonSeedModifier = dynamic_cast<JsonInt*>(jsonObject->GetValue("seed_mod"));
    if(!jsonSeedModifier)
    {
        aout << "No seed modifier found in progress object." << std::endl;
        return false;
    }

    this->level = jsonLevel->GetValue();
    this->touches = jsonTouches->GetValue();
    this->seedModifier = jsonSeedModifier->GetValue();
    return true;
}

bool Progress::Save(android_app* app)
{
    char progressFile[256];
    this->GetProgressFilepath(app, progressFile, sizeof(progressFile));
    FILE* fp = fopen(progressFile, "w");
    if(!fp)
    {
        aout << "Failed to open progress file (" << progressFile << ") for writing." << std::endl;
        return false;
    }

    std::shared_ptr<JsonObject> jsonObject(new JsonObject());
    jsonObject->SetValue("level", new JsonInt(this->level));
    jsonObject->SetValue("touches", new JsonInt(this->touches));
    jsonObject->SetValue("seed_mod", new JsonInt(this->seedModifier));

    std::string jsonString;
    if(!jsonObject->PrintJson(jsonString))
    {
        aout << "Failed to generate JSON string." << std::endl;
        fclose(fp);
        return false;
    }

    fwrite(jsonString.c_str(), jsonString.length(), 1, fp);
    fclose(fp);
    return true;
}

void Progress::GetProgressFilepath(android_app* app, char* progressFilepath, int progressFilepathSize)
{
    ::memset(progressFilepath, 0, progressFilepathSize);
    const char* dataFolder = app->activity->internalDataPath;
    sprintf(progressFilepath, "%s/progress.json", dataFolder);
}

int Progress::GetLevel() const
{
    return this->level;
}

void Progress::SetLevel(int level)
{
    this->level = level;
}

int Progress::GetTouches() const
{
    return this->touches;
}

void Progress::SetTouches(int touches)
{
    this->touches = touches;
}

int Progress::GetSeedModifier() const
{
    return this->seedModifier;
}