#include "AndroidOut.h"

AndroidOut androidOut("AO");
std::ostream aout(&androidOut);

AndroidOut::AndroidOut(const char* logTag)
{
    this->logTag = logTag;
}

/*virtual*/ int AndroidOut::sync()
{
    __android_log_print(ANDROID_LOG_DEBUG, this->logTag, "%s", this->str().c_str());
    this->str("");
    return 0;
}