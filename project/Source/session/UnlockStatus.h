
#pragma once

#include "ElementApp.h"

namespace Element {
    class Globals;
    class Settings;
    
    class UnlockStatus :  public kv::EDDOnlineUnlockStatus
    {
    public:
        UnlockStatus (Globals&);
        ~UnlockStatus() { }
        String getProductID() override;
        bool doesProductIDMatch (const String& returnedIDFromServer) override;
        RSAKey getPublicKey() override;
        void saveState (const String&) override;
        String getState() override;
        String getWebsiteName() override;
        URL getServerAuthenticationURL() override;
        URL getApiEndPoint() override;
        StringArray getLocalMachineIDs() override;
    
        inline bool isFullVersion() const
        {
            return false;
        }
        
        inline void dump()
        {
           #if JUCE_DEBUG
            DBG("UNLOCKED: " << ((bool) isUnlocked() ? "yes" : "no"));
            DBG("LICENSE:  " << getLicenseKey());
           #endif
        }
        
    protected:
        inline StringPairArray getQueryParams() override
        {
            StringPairArray params;
            params.set ("mach", getLocalMachineIDs()[0]);
            return params;
        }
        
    private:
        Settings& settings;
    };
}
