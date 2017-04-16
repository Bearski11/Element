/*
    Main.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"

#include "controllers/AppController.h"
#include "engine/InternalFormat.h"
#include "session/DeviceManager.h"
#include "PluginManager.h"
#include "Commands.h"

#include "Globals.h"
#include "Settings.h"

namespace Element {

class Startup : private Thread
{
public:
    Startup (Globals& w, const bool useThread = false, const bool splash = false)
        : Thread ("ElementStartup"),
          world (w), usingThread (useThread),
          showSplash (splash)
    { }

    ~Startup() { }

    void launchApplication()
    {
        if (usingThread)
        {
            startThread();
            while (isThreadRunning())
                MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
        else
        {
            if (showSplash)
                (new StartupScreen())->deleteAfterDelay (RelativeTime::seconds(5), true);
            this->run();
        }
    }

    ScopedPointer<AppController> controller;
    
    const bool isUsingThread() const { return usingThread; }

private:
    Globals& world;
    const bool usingThread;
    const bool showSplash;
    
    class StartupScreen :  public SplashScreen
    {
    public:
        StartupScreen()
            : SplashScreen ("Element", 600, 400, true)
        {
            addAndMakeVisible (text);
            text.setText ("Loading Application", dontSendNotification);
            text.setSize (600, 400);
            text.setFont (Font (24.0f));
            text.setJustificationType (Justification::centred);
            text.setColour (Label::textColourId, Colours::white);
        }

        void resized() override {
            SplashScreen::resized();
            text.setBounds (getLocalBounds());
        }

        void paint (Graphics& g) override {
            SplashScreen::paint (g);
            g.fillAll (Colours::aliceblue);
        }

    private:
        Label text;
    };

    void run() override
    {
        Settings& settings (world.settings());
        DeviceManager& devices (world.devices());
        PluginManager& plugins (world.plugins());
        
        if (ScopedXml dxml = settings.getUserSettings()->getXmlValue ("devices"))
            devices.initialise (16, 16, dxml.get(), true, "default", nullptr);
            
        AudioEnginePtr engine = new AudioEngine (world);
        world.setEngine (engine); // this will also instantiate the session
        
        // global data is ready, so now we can start using it;
        
        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine));
        plugins.restoreUserPlugins (settings);
        
        world.loadModule ("test");
        controller = new AppController (world);
        
        if (usingThread)
        {
            // post a message to finish launching
        }
    }
};

class Application : public JUCEApplication
{
public:
    Application() { }
    virtual ~Application() { }

    const String getApplicationName()    override      { return ProjectInfo::projectName; }
    const String getApplicationVersion() override      { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed()    override      { return true; }

    void initialise (const String&  commandLine ) override
    {
        initializeModulePath();
        world = new Globals (commandLine);
        launchApplication();
    }

    void shutdown() override
    {
#if 0
        if (gui != nullptr) {
            PluginWindow::closeAllCurrentlyOpenWindows();
            gui = nullptr;
        }

        PluginManager& plugins (world->plugins());
        Settings& settings (world->settings());

        if (ScopedXml el = world->devices().createStateXml())
            settings.getUserSettings()->setValue ("devices", el);

        world->setEngine (nullptr);
        engine = nullptr;
#endif
        controller = nullptr;
        world->setEngine (nullptr);
        world->unloadModules();
        world = nullptr;
    }

    void systemRequestedQuit() override
    {
        Application::quit();
    }

    void anotherInstanceStarted (const String& /*commandLine*/) override
    {
    
    }

    bool perform (const InvocationInfo& info) override
    {
        switch (info.commandID) {
            case Commands::quit: {
                this->systemRequestedQuit();
            } break;
        }

        return true;
    }

    void finishLaunching()
    {
        if (nullptr != controller || nullptr == startup)
            return;
        
        controller = startup->controller.release();
        startup = nullptr;
        controller->run();
    }
    
private:
    ScopedPointer<Globals>       world;
    ScopedPointer<AppController> controller;
    ScopedPointer<Startup> startup;
    
    void launchApplication()
    {
        if (nullptr != controller)
            return;
        
        startup = new Startup (*world, false, false);
        startup->launchApplication();
        if (! startup->isUsingThread())
            finishLaunching();
    }
    
    void initializeModulePath()
    {
        const File path (File::getSpecialLocation (File::invokedExecutableFile));
        File modDir = path.getParentDirectory().getParentDirectory()
                          .getChildFile("lib/element").getFullPathName();
       #if JUCE_DEBUG
        if (! modDir.exists()) {
            modDir = path.getParentDirectory().getParentDirectory()
            .getChildFile ("modules");
        }
       #endif
        
       #if JUCE_WINDOWS
        String putEnv = "ELEMENT_MODULE_PATH="; putEnv << modDir.getFullPathName();
        putenv (putEnv.toRawUTF8());
       #else
        setenv ("ELEMENT_MODULE_PATH", modDir.getFullPathName().toRawUTF8(), 1);
        Logger::writeToLog (String("[element] module path: ") + String(getenv ("ELEMENT_MODULE_PATH")));
       #endif
    }
};

}

START_JUCE_APPLICATION (Element::Application)
