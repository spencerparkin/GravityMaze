#include "AudioSubSystem.h"
#include "AndroidOut.h"

//------------------------------ AudioSubSystem ------------------------------

AudioSubSystem::AudioSubSystem()
{
    this->systemSetup = false;
    this->audioStream = nullptr;
}

/*virtual*/ AudioSubSystem::~AudioSubSystem()
{
}

bool AudioSubSystem::Setup(AAssetManager* assetManager)
{
    bool success = false;
    AAssetDir* audioDir = nullptr;

    aout << "Initializing audio sub-system..." << std::endl;

    do
    {
        if (this->systemSetup)
        {
            aout << "Audio sub-system already initialized!" << std::endl;
            break;
        }

        oboe::AudioStreamBuilder builder;

        builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        builder.setSharingMode(oboe::SharingMode::Exclusive);
        builder.setCallback(&this->audioFeeder);
        builder.setChannelCount(2);
        builder.setFormat(oboe::AudioFormat::Float);
        builder.setContentType(oboe::ContentType::Sonification);
        builder.setDirection(oboe::Direction::Output);

        oboe::Result result = builder.openStream(&this->audioStream);
        if (result != oboe::Result::OK)
        {
            aout << "Failed to create audio stream." << std::endl;
            aout << "Reason: " << oboe::convertToText(result) << std::endl;
            break;
        }

        int bufferSizeFrames = this->audioStream->getFramesPerBurst() * 2;
        this->audioStream->setBufferSizeInFrames(bufferSizeFrames);

        result = this->audioStream->requestStart();
        if(result != oboe::Result::OK)
        {
            aout << "Failed to start audio stream." << std::endl;
            aout << "Reason: " << oboe::convertToText(result) << std::endl;
            break;
        }

        audioDir = AAssetManager_openDir(assetManager, "audio");
        if(!audioDir)
        {
            aout << "Failed to open audio directory." << std::endl;
            break;
        }

#if 0           // TODO: Re-enable this once I have a .WAV file reader that works.
        bool loadFailureOccurred = false;
        while(true)
        {
            const char* audioFile = AAssetDir_getNextFileName(audioDir);
            if(!audioFile)
                break;

            char audioFilePath[128];
            sprintf(audioFilePath, "audio/%s", audioFile);

            auto audioClip = new AudioClip();
            this->audioClipArray.push_back(audioClip);
            if(!audioClip->Load(audioFilePath, assetManager))
            {
                loadFailureOccurred = true;
                break;
            }
        }

        if(loadFailureOccurred)
        {
            aout << "Failed to load all audio clips." << std::endl;
            break;
        }
#endif

        this->systemSetup = true;
        success = true;
        aout << "Audio sub-system successfully initialized!" << std::endl;
    }
    while(false);

    if(audioDir)
        AAssetDir_close(audioDir);

    if(!success)
        this->Shutdown();

    return success;
}

bool AudioSubSystem::Shutdown()
{
    aout << "Shutting down audio sub-system..." << std::endl;

    for(auto audioClip : this->audioClipArray)
        delete audioClip;

    this->audioClipArray.clear();

    if(this->audioStream)
    {
        this->audioStream->close();
        this->audioStream = nullptr;
    }

    this->systemSetup = false;
    aout << "Audio sub-system shutdown!" << std::endl;
    return true;
}

void AudioSubSystem::PlayFX(SoundFXType soundFXType)
{
}

//------------------------------ AudioSubSystem::AudioClip ------------------------------

AudioSubSystem::AudioClip::AudioClip()
{
    this->bitsPerSample = 0;
    this->numChannels = 0;
    this->numFrames = 0;
    this->sampleRate = 0;
    this->waveBuf = nullptr;
}

/*virtual*/ AudioSubSystem::AudioClip::~AudioClip()
{
    this->Unload();
}

void AudioSubSystem::AudioClip::Unload()
{
    if(this->waveBuf)
    {
        delete[] this->waveBuf;
        this->waveBuf = nullptr;
    }

    this->bitsPerSample = 0;
    this->numChannels = 0;
    this->numFrames = 0;
    this->sampleRate = 0;
}

bool AudioSubSystem::AudioClip::Load(const char* audioFilePath, AAssetManager* assetManager)
{
    bool success = false;
    AAsset* audioAsset = nullptr;

    do
    {
        this->Unload();

        audioAsset = AAssetManager_open(assetManager, audioFilePath, AASSET_MODE_BUFFER);
        if (!audioAsset)
            break;

        int audioAssetSize = AAsset_getLength(audioAsset);
        if (audioAssetSize == 0)
            break;

        const unsigned char* audioAssetBuf = static_cast<const unsigned char*>(AAsset_getBuffer(audioAsset));
        break;  // TODO: Write remainder of loader here.

        success = true;
    }
    while(false);

    if(audioAsset)
        AAsset_close(audioAsset);

    if(!success)
        this->Unload();

    return success;
}

//------------------------------ AudioSubSystem::AudioFeeder ------------------------------

AudioSubSystem::AudioFeeder::AudioFeeder()
{
}

/*virtual*/ AudioSubSystem::AudioFeeder::~AudioFeeder()
{
}

// Note: This is probably be called on a thread other than the main thread!
/*virtual*/ oboe::DataCallbackResult AudioSubSystem::AudioFeeder::onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numAudioFrames)
{
    int bytesPerFrame = audioStream->getBytesPerFrame();
    int bytesPerSample = audioStream->getBytesPerSample();
    int samplesPerFrameOrNumChannels = bytesPerFrame / bytesPerSample;
    int numAudioSamples = numAudioFrames / samplesPerFrameOrNumChannels;

    if(bytesPerSample == sizeof(float))
    {
        float* sampleBuffer = static_cast<float*>(audioData);
        for (int i = 0; i < numAudioSamples; i++)
            sampleBuffer[i] = 0.0;      // Just write silence for now.
    }

    return oboe::DataCallbackResult::Continue;
}