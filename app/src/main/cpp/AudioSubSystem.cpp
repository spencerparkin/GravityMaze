#include "AudioSubSystem.h"
#include "AndroidOut.h"
#include "Math/Utilities/Random.h"

using namespace AudioDataLib;

//------------------------------ AudioSubSystem ------------------------------

AudioSubSystem::AudioSubSystem()
{
    this->systemSetup = false;
    this->audioStream = nullptr;
    this->audioFeeder = nullptr;
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

        this->audioFeeder = new AudioFeeder();

        oboe::AudioStreamBuilder builder;

        builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        builder.setSharingMode(oboe::SharingMode::Shared);
        builder.setCallback(this->audioFeeder);
        builder.setContentType(oboe::ContentType::Sonification);
        builder.setDirection(oboe::Direction::Output);
        builder.setFormatConversionAllowed(true);
        builder.setErrorCallback(&this->errorCallback);
#if 0
        builder.setChannelMask(oboe::ChannelMask::Mono);
        builder.setChannelCount(oboe::ChannelCount::Mono);
        builder.setSampleRate(48000);
        builder.setFormat(oboe::AudioFormat::I16);
#endif

        oboe::Result result = builder.openStream(&this->audioStream);
        if (result != oboe::Result::OK)
        {
            aout << "Failed to create audio stream." << std::endl;
            aout << "Reason: " << oboe::convertToText(result) << std::endl;
            break;
        }

        if(!this->audioFeeder->Configure(this->audioStream))
        {
            aout << "Could not configure our audio sink based on the given audio stream." << std::endl;
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

            if(strstr(audioFile, "GO_") == audioFile)
                audioClip->type = SoundFXType::GOOD_OUTCOME;
            else if(strstr(audioFile, "BO_") == audioFile)
                audioClip->type = SoundFXType::BAD_OUTCOME;
            else
                audioClip->type = SoundFXType::UNKNOWN;
        }

        if(loadFailureOccurred)
        {
            aout << "Failed to load all audio clips." << std::endl;
            break;
        }

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

    if(this->audioFeeder)
    {
        delete this->audioFeeder;
        this->audioFeeder = nullptr;
    }

    this->systemSetup = false;
    aout << "Audio sub-system shutdown!" << std::endl;
    return true;
}

void AudioSubSystem::PlayFX(SoundFXType soundFXType)
{
    std::vector<AudioClip*> possibleClipsArray;
    for(AudioClip* audioClip : this->audioClipArray)
        if(audioClip->type == soundFXType)
            possibleClipsArray.push_back(audioClip);

    int i = PlanarPhysics::Random::Integer(0, possibleClipsArray.size() - 1);
    AudioClip* chosenClip = possibleClipsArray[i];
    this->audioFeeder->audioSink.AddAudioInput(new AudioStream(chosenClip->audioData));
}

bool AudioSubSystem::PumpAudio()
{
    this->audioFeeder->audioSink.GenerateAudio(0.05, 0.05);
    return true;
}

//------------------------------ AudioSubSystem::ErrorCallback ------------------------------

AudioSubSystem::ErrorCallback::ErrorCallback()
{
}

/*virtual*/ AudioSubSystem::ErrorCallback::~ErrorCallback()
{
}

/*virtual*/ bool AudioSubSystem::ErrorCallback::onError(oboe::AudioStream* audioStream, oboe::Result result)
{
    aout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    aout << "Audio error: " << oboe::convertToText(result) << std::endl;
    aout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;

    return false;
}

//------------------------------ AudioSubSystem::AudioClip ------------------------------

AudioSubSystem::AudioClip::AudioClip()
{
    this->audioData = nullptr;
    this->type = SoundFXType::UNKNOWN;
}

/*virtual*/ AudioSubSystem::AudioClip::~AudioClip()
{
    this->Unload();
}

void AudioSubSystem::AudioClip::Unload()
{
    if(this->audioData)
    {
        AudioData::Destroy(this->audioData);
        this->audioData = nullptr;
    }
}

bool AudioSubSystem::AudioClip::Load(const char* audioFilePath, AAssetManager* assetManager)
{
    bool success = false;
    AAsset* audioAsset = nullptr;
    WaveFileFormat waveFileFormat;

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
        BufferStream inputStream(audioAssetBuf, audioAssetSize);

        std::string error;
        if(!waveFileFormat.ReadFromStream(inputStream, this->audioData, error))
            break;

        // Make sure the format is what we expect to process on the stream.
        const AudioData::Format& format = this->audioData->GetFormat();
        assert(format.numChannels == 1);
        assert(format.bitsPerSample == 16);
        assert(format.framesPerSecond == 48000);
        assert(format.sampleType == AudioData::Format::SIGNED_INTEGER);

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

AudioSubSystem::AudioFeeder::AudioFeeder() : audioSink(true)
{
}

/*virtual*/ AudioSubSystem::AudioFeeder::~AudioFeeder()
{
}

bool AudioSubSystem::AudioFeeder::Configure(oboe::AudioStream* audioStream)
{
    int sampleRate = audioStream->getSampleRate();
    int numChannels = audioStream->getChannelCount();
    int bitDepth = audioStream->getBytesPerSample() * 8;
    oboe::AudioFormat sampleFormat = audioStream->getFormat();

    aout << "Audio stream sample rate: " << sampleRate << std::endl;
    aout << "Audio stream bit depth: " << bitDepth << std::endl;
    aout << "Audio stream channels: " << numChannels << std::endl;
    aout << "Audio stream format: " << oboe::convertToText(sampleFormat) << std::endl;

    AudioData::Format audioDataFormat;
    audioDataFormat.bitsPerSample = bitDepth;
    audioDataFormat.framesPerSecond = sampleRate;
    audioDataFormat.numChannels = numChannels;

    switch(sampleFormat)
    {
        case oboe::AudioFormat::Float:
        {
            audioDataFormat.sampleType = AudioData::Format::FLOAT;
            break;
        }
        case oboe::AudioFormat::I16:
        case oboe::AudioFormat::I32:
        {
            audioDataFormat.sampleType = AudioData::Format::SIGNED_INTEGER;
            break;
        }
        default:
        {
            return false;
        }
    }

    this->audioSink.SetAudioOutput(new ThreadSafeAudioStream(audioDataFormat, new AudioMutex(), true));
    return true;
}

// Note: This is called on a thread other than the main thread!
/*virtual*/ oboe::DataCallbackResult AudioSubSystem::AudioFeeder::onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numAudioFrames)
{
    const AudioData::Format& format = this->audioSink.GetAudioOutput()->GetFormat();
    uint64_t numBytesNeeded = format.BytesPerFrame() * uint64_t(numAudioFrames);
    uint64_t numBytesRead = this->audioSink.GetAudioOutput()->ReadBytesFromStream((uint8_t*)audioData, numBytesNeeded);
    for(uint64_t i = numBytesRead; i < (uint64_t)numBytesNeeded; i++)
        ((uint8_t*)audioData)[i] = 0;

    return oboe::DataCallbackResult::Continue;
}

//------------------------------ AudioSubSystem::AudioMutex ------------------------------

AudioSubSystem::AudioMutex::AudioMutex()
{
    pthread_mutex_init(&this->mutex, nullptr);
}

/*virtual*/ AudioSubSystem::AudioMutex::~AudioMutex()
{
    pthread_mutex_destroy(&this->mutex);
}

/*virtual*/ void AudioSubSystem::AudioMutex::Lock()
{
    // The spin-wait here is being used to avoid preemption and may
    // be justified since we should *never* have to wait very long
    // for the lock to become available.
    while(true)
    {
        int result = pthread_mutex_trylock(&this->mutex);
        if(result == 0)
            break;
    }
}

/*virtual*/ void AudioSubSystem::AudioMutex::Unlock()
{
    pthread_mutex_unlock(&this->mutex);
}