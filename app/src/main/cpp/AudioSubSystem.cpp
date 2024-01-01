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

bool AudioSubSystem::Setup()
{
    bool success = false;
    aout << "Initializing audio sub-system..." << std::endl;

    do
    {
        // TODO: We don't want to be loading our audio clips here and then destroying
        //       them when we shutdown, because we're going to be setup and shutdown
        //       with the window.  Rather, we could just expect a pointer to an audio
        //       clip cache owned by the main program.

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

        this->systemSetup = true;
        success = true;
        aout << "Audio sub-system successfully initialized!" << std::endl;
    }
    while(false);

    if(!success)
        this->Shutdown();

    return success;
}

bool AudioSubSystem::Shutdown()
{
    aout << "Shutting down audio sub-system..." << std::endl;

    if(this->audioStream)
    {
        this->audioStream->close();
        this->audioStream = nullptr;
    }

    this->systemSetup = false;
    aout << "Audio sub-system shutdown!" << std::endl;
    return true;
}

void AudioSubSystem::PlayFX(SoundFX soundFX)
{
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