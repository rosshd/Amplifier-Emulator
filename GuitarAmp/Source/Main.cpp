#include <JuceHeader.h>

class MainComponent : public juce::AudioAppComponent,
                     public juce::Timer,
                     public juce::ChangeListener
{
public:
    MainComponent()
    {
        setSize(600, 400);
        currentInputLevel = 0.0f;
        
        // Add debug label first
        addAndMakeVisible(debugLabel);
        debugLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(settingsButton);
        settingsButton.setButtonText("Audio Settings");
        settingsButton.onClick = [this] { showAudioSettings(); };
        
        // IMPORTANT: Set up audio BEFORE initializing the device manager
        setAudioChannels(1, 2); // This must come before showing settings
        
        deviceManager.addChangeListener(this);
        startTimerHz(30);
        
        // Initial debug update
        updateDebugInfo();
    }

    ~MainComponent() override
    {
        deviceManager.removeChangeListener(this);
        shutdownAudio();
    }

    void updateDebugInfo()
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        juce::String debugText;
        
        if (device == nullptr)
        {
            debugText = "No audio device selected";
        }
        else
        {
            auto activeInputs = device->getActiveInputChannels();
            auto inputBits = activeInputs.toString(2); // Get binary representation
            
            debugText = "Device: " + device->getName() + "\n"
                     + "Active Channels Bits: " + inputBits + "\n"
                     + "Input Channel Count: " + juce::String(activeInputs.countNumberOfSetBits()) + "\n"
                     + "Sample Rate: " + juce::String(device->getCurrentSampleRate()) + "\n"
                     + "Buffer Size: " + juce::String(device->getCurrentBufferSizeSamples()) + "\n"
                     + "Input Names:";
            
            // List all available input channels
            auto numInputs = device->getInputChannelNames();
            for (const auto& name : numInputs)
            {
                debugText += "\n - " + name;
            }
            
            debugText += "\nInput Level: " + juce::String(currentInputLevel);
        }
        
        debugLabel.setText(debugText, juce::dontSendNotification);
    }

    void showAudioSettings()
    {
        juce::DialogWindow::LaunchOptions options;
        
        auto* audioSettings = new juce::AudioDeviceSelectorComponent(
            deviceManager,    // device manager
            1,               // minAudioInputChannels
            2,               // maxAudioInputChannels
            2,               // minAudioOutputChannels
            2,               // maxAudioOutputChannels
            true,           // showMidiInputOptions
            false,          // showMidiOutputSelector
            true,           // showChannelsAsStereoPairs
            false           // hideAdvancedOptionsWithButton
        );
        
        audioSettings->setSize(450, 350);
        
        options.content.setOwned(audioSettings);
        options.content->setSize(450, 350);
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        if (device != nullptr)
        {
            updateDebugInfo();
            repaint();
        }
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        
        if (device == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Get the input buffer
        const float* const inputBuffer = bufferToFill.buffer->getReadPointer(0);
        
        // Calculate both peak and RMS levels
        float sumSquared = 0.0f;
        float peakLevel = 0.0f;
        
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            const float inputSample = inputBuffer[sample];
            sumSquared += inputSample * inputSample;
            peakLevel = std::max(peakLevel, std::abs(inputSample));
        }
        
        // Update the level (using RMS)
        currentInputLevel = std::sqrt(sumSquared / bufferToFill.numSamples) * 10.0f; // Increased sensitivity
        
        // Copy to output with gain
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            float* outputBuffer = bufferToFill.buffer->getWritePointer(channel);
            
            if (channel == 0) // First channel: copy input
            {
                for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
                    outputBuffer[sample] = inputBuffer[sample] * 2.0f; // Added gain
            }
            else // Other channels: copy from first channel
            {
                bufferToFill.buffer->copyFrom(channel, 0, bufferToFill.buffer->getReadPointer(0),
                                            bufferToFill.numSamples);
            }
        }
    }

    void releaseResources() override {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        
        // Level meter
        g.setColour(juce::Colours::green);
        auto meterWidth = getWidth() * 0.8f;
        auto meterHeight = 30.0f;
        auto meterX = (getWidth() - meterWidth) * 0.5f;
        auto meterY = getHeight() * 0.5f;
        
        g.drawRect(meterX, meterY, meterWidth, meterHeight);
        
        // Draw level (with increased visibility for quiet signals)
        float scaledLevel = std::min(currentInputLevel, 1.0f);
        g.fillRect(meterX, meterY, meterWidth * scaledLevel, meterHeight);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        settingsButton.setBounds(bounds.removeFromTop(30).reduced(5));
        debugLabel.setBounds(bounds.removeFromTop(200)); // Increased height for debug info
    }

    void timerCallback() override
    {
        updateDebugInfo();
        repaint();
    }

private:
    float currentInputLevel;
    juce::TextButton settingsButton;
    juce::Label debugLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

// Window class to hold our component
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name)
        : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

// Application class
class GuitarAmpApplication : public juce::JUCEApplication
{
public:
    GuitarAmpApplication() {}

    const juce::String getApplicationName() override { return "Guitar Amp"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpApplication)
};

// This macro creates the application's main() function
START_JUCE_APPLICATION(GuitarAmpApplication)