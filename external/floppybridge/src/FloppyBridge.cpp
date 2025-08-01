/* FloppyBridge DLL for *UAE
*
* Copyright (C) 2021-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file is multi-licensed under the terms of the Mozilla Public
* License Version 2.0 as published by Mozilla Corporation and the
* GNU General Public License, version 2 or later, as published by the
* Free Software Foundation.
*
* MPL2: https://www.mozilla.org/en-US/MPL/2.0/
* GPL2: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

/*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

#include <cstring>
#include "FloppyBridge.h"
#include "resource.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

// For compatibility with older methods
#define ROMTYPE_ARDUINOREADER_WRITER
#define ROMTYPE_GREASEWEAZLEREADER_WRITER
#define ROMTYPE_SUPERCARDPRO_WRITER

#include "ArduinoFloppyBridge.h"
#include "GreaseWeazleBridge.h"
#include "SuperCardProBridge.h"

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include "bridgeProfileListEditor.h"
#include "bridgeProfileEditor.h"
#include <WinDNS.h>
#pragma comment(lib, "Dnsapi.lib")
HINSTANCE hInstance;

#else
#include <netdb.h>
#endif 
 
#include <unordered_map>



static FloppyBridge::BridgeAbout BridgeInformation = { "FloppyBridge, Copyright(C) 2021-2024 RobSmithDev", "https://amiga.robsmithdev.co.uk/winuae", 1, 6, 0, 0, 0};
static FloppyBridge::BridgeAbout BridgeInformationUpdate = { "FloppyBridge UPDATE AVAILABLE, Copyright(C) 2021-2024 RobSmithDev", "https://amiga.robsmithdev.co.uk/winuae", 1, 6, 0, 0, 0 };
static bool hasUpdateChecked = false;
static bool enableUsageNotifications = true;

std::vector<SerialIO::SerialPortInformation> serialports;
#ifdef _WIN32
std::vector<HBITMAP> bridgeLogos;
#endif
std::unordered_map<unsigned int, BridgeConfig*> profileList;
FloppyBridge::FloppyBridgeProfileInformationDLL* profileCache = nullptr;
std::string profileStringExported;


bool BridgeConfig::fromString(char* serialisedOptions) {
    // Validate
    if (strlen(serialisedOptions) < 7) return false;
    std::string tmp = serialisedOptions;

    size_t pos = tmp.find('[');
    if (pos == std::string::npos) return false;
    if (pos) {
#ifdef _WIN32
        strcpy_s(profileName, 128, (char*)tmp.substr(0, pos).c_str());
#else
        strcpy(profileName, (char*)tmp.substr(0, pos).c_str());
#endif
        tmp = tmp.substr(pos);
    }
    else {
        profileName[0] = '\0';
    }

    if ((tmp[0] != '[') || (tmp[tmp.length() - 1] != ']')) return false;
    tmp = tmp.substr(1, tmp.length() - 2);

    // Split out
    std::vector<std::string> params;
    pos = tmp.find('|');
    while (pos != std::string::npos) {
        params.push_back(tmp.substr(0, pos));
        tmp = tmp.substr(pos + 1);
        pos = tmp.find('|');
    }
    params.push_back(tmp);

    // Parse
    if (params.size() < 5) return false;

    // Validate it's for this bridge
    bridgeIndex = atoi(params[0].c_str());

    unsigned int i = atoi(params[1].c_str());
    autoCache = (i & 1) != 0;
    driveCable = (FloppyBridge::DriveSelection)(((i & 2)>>1) | ((i & 48) >> 3));
    autoDetectComPort = (i & 4) != 0;
    smartSpeed = (i & 8) != 0;
#ifdef _WIN32
    strcpy_s(comPortToUse, sizeof(comPortToUse) - 1, params[2].c_str());
#else
    strcpy(comPortToUse, params[2].c_str());
#endif

    i = atoi(params[3].c_str());
    if (i > (unsigned int)FloppyBridge::BridgeMode::bmMax) return false;
    bridgeMode = (FloppyBridge::BridgeMode)i;

    i = atoi(params[4].c_str());
    if (i > (unsigned int)FloppyBridge::BridgeDensityMode::bdmMax) return false;
    bridgeDensity = (FloppyBridge::BridgeDensityMode)i;

    return true;
}

void BridgeConfig::toString(char** serialisedOptions) {
    unsigned int i = (unsigned int)driveCable;

    unsigned int flags = (autoCache ? 1 : 0) | ((i & 1) << 1) | ((i & 6) << 3) | (autoDetectComPort ? 4 : 0) | (smartSpeed ? 8 : 0);

    std::string tmp;
    tmp = std::string(profileName) + "[" + std::to_string(bridgeIndex) + "|" + std::to_string(flags) + "|" + std::string(comPortToUse) + "|" + std::to_string((unsigned int)bridgeMode) + "|" + std::to_string((unsigned int)bridgeDensity) + "]";
#ifdef _WIN32
    strcpy_s(lastSerialise, sizeof(lastSerialise) - 1, tmp.c_str());
#else
    strcpy(lastSerialise, tmp.c_str());
#endif

    (*serialisedOptions) = lastSerialise;
};

// Returns a pointer to information about the project
void handleAbout(bool checkForUpdates, FloppyBridge::BridgeAbout** output) {
#ifdef _WIN32
    checkForUpdates |= BridgeProfileListEditor::shouldAutoCheckForUpdates();
#endif
    if ((checkForUpdates) && (!hasUpdateChecked)) {
        hasUpdateChecked = true;

#ifdef _WIN32
        // Start winsock
        WSADATA data;
        WSAStartup(MAKEWORD(2, 0), &data);

        DNS_RECORD* dnsRecord;
        std::wstring versionString;

        // Look up TXT record 
        DNS_STATUS error = DnsQuery(L"floppybridge-amiga.robsmithdev.co.uk", DNS_TYPE_TEXT, DNS_QUERY_BYPASS_CACHE, NULL, &dnsRecord, NULL);
        if (!error) {
            const DNS_RECORD* record = dnsRecord;
            while (record) {
                if (record->wType == DNS_TYPE_TEXT) {
                    const DNS_TXT_DATAW* pData = &record->Data.TXT;
                    for (DWORD string = 0; string < pData->dwStringCount; string++) {
                        const std::wstring text = pData->pStringArray[string];
                        if ((text.length() >= 9) && (text.substr(0, 2) == L"v=")) versionString = text.substr(2);
                    }
                }
                record = record->pNext;
            }
            DnsRecordListFree(dnsRecord, DnsFreeRecordList);  //DnsFreeRecordListDeep
        }

        if (!versionString.empty()) {
            // A little hacky to convert a.b.c.d into an array
            sockaddr_in tmp;
            INT len = sizeof(tmp);
            if (WSAStringToAddress((wchar_t*)versionString.c_str(), AF_INET, NULL, (sockaddr*)&tmp, &len) == 0) {
                const in_addr add = tmp.sin_addr;
                BridgeInformationUpdate.updateMajorVersion = add.S_un.S_un_b.s_b1;
                BridgeInformationUpdate.updateMinorVersion = add.S_un.S_un_b.s_b2;
                BridgeInformation.isUpdateAvailable = ((BridgeInformation.majorVersion < BridgeInformation.updateMajorVersion) ||
                    ((BridgeInformation.majorVersion == BridgeInformation.updateMajorVersion) && (BridgeInformation.minorVersion < BridgeInformation.updateMinorVersion))) ? 1 : 0;
                BridgeInformationUpdate.isUpdateAvailable = BridgeInformation.isUpdateAvailable;;
            }
        }
#else
        // Fetch version from 'A' record in the DNS record - this is probably not used by anything anyway
        hostent* address = gethostbyname("floppybridge-amiga.robsmithdev.co.uk");
        if ((address) && (address->h_addrtype == AF_INET)) {
            if (address->h_addr_list[0] != 0) {
                in_addr add = *((in_addr*)address->h_addr_list[0]);
                uint32_t bytes = htonl(add.s_addr);
                BridgeInformationUpdate.updateMajorVersion = bytes >> 24;
                BridgeInformationUpdate.updateMinorVersion = (bytes >> 16) & 0xFF;

                BridgeInformation.isUpdateAvailable = ((BridgeInformation.majorVersion < BridgeInformation.updateMajorVersion) ||
                    ((BridgeInformation.majorVersion == BridgeInformation.updateMajorVersion) && (BridgeInformation.minorVersion < BridgeInformation.updateMinorVersion))) ? 1 : 0;
                BridgeInformationUpdate.isUpdateAvailable = BridgeInformation.isUpdateAvailable;;
            }
        }
#endif
    }
#ifdef _WIN32
    if (output) 
        if (BridgeInformationUpdate.isUpdateAvailable) *output = (FloppyBridge::BridgeAbout*)&BridgeInformationUpdate; else *output = (FloppyBridge::BridgeAbout*)&BridgeInformation;

#else
    if (output) *output = (FloppyBridge::BridgeAbout*)&BridgeInformation;
#endif 
}

// Get driver information
bool handleGetDriverInfo(unsigned int driverIndex, FloppyDiskBridge::BridgeDriver** driverInformation) {
    if (driverIndex >= MAX_NUM_DRIVERS) return false;
    if (!driverInformation) return false;

    switch (driverIndex) {
    case 0: *driverInformation = (FloppyDiskBridge::BridgeDriver*)ArduinoFloppyDiskBridge::staticBridgeInformation(); break;
    case 1: *driverInformation = (FloppyDiskBridge::BridgeDriver*)GreaseWeazleDiskBridge::staticBridgeInformation();  break;
    case 2: *driverInformation = (FloppyDiskBridge::BridgeDriver*)SupercardProDiskBridge::staticBridgeInformation(); break;
    default: return false;
    }
    return true;
}

extern "C" {

    // Returns a pointer to information about the project
    FLOPPYBRIDGE_API void CALLING_CONVENSION BRIDGE_About(bool checkForUpdates, FloppyBridge::BridgeAbout** output) {
        handleAbout(checkForUpdates, output);
    }

    // Returns the number of 'Bridge' Drivers available 
    FLOPPYBRIDGE_API unsigned int CALLING_CONVENSION BRIDGE_NumDrivers(void) {
        return MAX_NUM_DRIVERS;
    }

    // Used to turn on or off the windows messages used to disable the virtual drive system while this is in use.
    // Defaults to TRUE which is what it should do!    
    FLOPPYBRIDGE_API void CALLING_CONVENSION BRIDGE_EnableUsageNotifications(bool enable) {
        enableUsageNotifications = enable;
    }

    // Enumerates the com ports and returns how many of them were found.  
    // You must call this twice.  the first call output=null (triggers enumeration, and buffers size inc null terminator is copied to outputLength)
    // Second call you supply suitable buffer (and populate outputLength) and result is copied. String is a null terminated list of strings
    // Returns if successfully copied or not
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_EnumComports(char* output, unsigned int* outputLength) {
        if (!output) SerialIO::enumSerialPorts(serialports);

        if (!outputLength) return false;

        size_t lengthRequired = 1; // final null terminator
        std::string tmp;
        for (const SerialIO::SerialPortInformation& port : serialports) {
            quickw2a(port.portName, tmp);
            lengthRequired += tmp.length() + 1;
        }

        if (output) {
            // Enough space now?
            if (lengthRequired > *outputLength) return false;

            for (const SerialIO::SerialPortInformation& port : serialports) {
                quickw2a(port.portName, tmp);

                // Copy name
#ifdef _WIN32
                memcpy_s(output, lengthRequired, tmp.c_str(), tmp.length());
#else
                memcpy(output, tmp.c_str(), tmp.length());
#endif
                lengthRequired -= tmp.length();

                // Add separator
                output += tmp.length();
                *output = '\0'; output++;
                lengthRequired--;
            }
            // Add terminator
            *output = '\0'; output++;

            return true;
        }
        else {
            *outputLength = (unsigned int)lengthRequired;
            return false;
        }
    }

    // Returns a pointer to a string which is a serialised result of all of the config options chosen
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetConfigString(BridgeOpened* bridgeDriverHandle, char** serialisedOptions) {
        if (!bridgeDriverHandle) return false;
        if (!serialisedOptions) return false;

        bridgeDriverHandle->config.toString(serialisedOptions);
        return true;
    }
    // Applies the settings in a serialised string of settings obtained from the above function
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetConfigFromString(BridgeOpened* bridgeDriverHandle, char* serialisedOptions) {
        if (!bridgeDriverHandle) return false;
        if (!serialisedOptions) return false;

        return bridgeDriverHandle->config.fromString(serialisedOptions);
    }

    // Used to signal to the virtual drive system that we're going to be using it so let go.
    void releaseVirtualDrives(bool release, int controllerType) {
        if (!enableUsageNotifications) return;
#ifdef _WIN32
        HWND remoteWindow = FindWindow(L"VIRTUALDRIVE_CONTROLLER_CLASS", L"DiskFlashback Tray Control");
        if (remoteWindow) {
            // Signal to release all interfaces of this type
            SendMessage(remoteWindow, WM_USER + 2, (controllerType & 0x7FFF) | (release ? 0 : 0x8000), (LPARAM)GetCurrentProcessId());
        }
#endif
    }
     

#ifdef _WIN32
    // Displays the config dialog (modal) for Floppy Bridge profiles.  
    // *If* you pass a profile ID, the dialog will jump to editing that profile, or return FALSE if it was not found.
    // Returns FALSE if cancel was pressed
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ShowConfigDialog(HWND hwndParent, unsigned int* profileID) {
        if (profileID) {
            auto f = profileList.find(*profileID);
            if (f == profileList.end()) return false;

            // This doesn't modify what's passed unless it returns TRUE
            BridgeProfileEditor editor(hInstance, hwndParent, bridgeLogos, f->second);
            return editor.doModal();
        }

        // Make a backup of the profiles first
        std::unordered_map<unsigned int, BridgeConfig*> backupProfiles;
        for (auto f : profileList) {
            char* tmpStr;
            BridgeConfig* tmp = new BridgeConfig();

            f.second->toString(&tmpStr);
            tmp->fromString(tmpStr);

            backupProfiles.insert(std::make_pair(f.first, tmp));
        }

        BridgeProfileListEditor editor(hInstance, hwndParent, bridgeLogos, profileList);
        if (editor.doModal()) {
            // Success. Delete the backup
            for (auto f : backupProfiles) delete f.second;

            return true;
        } 
        else {
            // Restore the list from the backup.  Step 1: Erase the current list
            for (auto f : profileList) delete f.second;
            // And overwrite it from the backup
            profileList = backupProfiles;

            return false;
        }
    }
#endif

    // Retrieve a list of all of the profiles currently loaded that can be used.
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetAllProfiles(FloppyBridge::FloppyBridgeProfileInformationDLL** profiles, unsigned int* numProfiles) {
        if (profileCache) free(profileCache);
        if (!numProfiles) return false;
        if (!profiles) return false;
        profileCache = (FloppyBridge::FloppyBridgeProfileInformationDLL*)malloc(sizeof(FloppyBridge::FloppyBridgeProfileInformationDLL) * profileList.size());
        if (!profileCache) return false;

        FloppyBridge::FloppyBridgeProfileInformationDLL* output = profileCache;
        for (auto& profile : profileList) {
            output->name = profile.second->profileName;
            output->bridgeMode = profile.second->bridgeMode;
            output->bridgeDensityMode = profile.second->bridgeDensity;
            output->profileID = profile.first;
            output->driverIndex = profile.second->bridgeIndex;
            profile.second->toString(&output->profileConfig);
            
            output++;
        }

        *profiles = profileCache;
        *numProfiles = (unsigned int)profileList.size();
        return true;
    }

    // Imports all profiles into memory from the supplied string.  This will erase any currently in memory
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ImportProfilesFromString(char* profilesConfigString) {
        if (!profilesConfigString) return false;

        // Free memory
        for (auto p : profileList) delete p.second;
        profileList.clear();

        std::string tmp = profilesConfigString;

        // Do until we run out of string
        while (tmp.length()) {
            // Find the BAR
            size_t pos = tmp.find('|');
            if (pos == std::string::npos) break;

            // ProfileID must be >0
            unsigned int profileID = atoi(tmp.substr(0, pos).c_str());
            if (profileID == 0) break;

            tmp = tmp.substr(pos + 1);

            // Extract just this profile
            pos = tmp.find(']');
            if (pos == std::string::npos) break;
            
            BridgeConfig* config = new BridgeConfig();
            if (config->fromString((char*)tmp.substr(0, pos+1).c_str())) {
                profileList.insert(std::make_pair(profileID, config));
            }
            else delete config;
            tmp = tmp.substr(pos + 1);
        }

        return true;
    }

    // Exports all profiles and returns a pointer to the string.  This pointer is only valid while the driver is loaded and until this is called again
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ExportProfilesToString(char** profilesConfigString) {
        if (!profilesConfigString) return false;
        profileStringExported = "";

        for (auto& profile : profileList) {
            profileStringExported += std::to_string(profile.first) + "|";
            char* tmp;
            profile.second->toString(&tmp);
            profileStringExported += tmp;
        }

        *profilesConfigString = (char*)profileStringExported.c_str();
        return true;
    }

    // Returns a pointer to a string containing the details for a profile
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetProfileConfigFromString(unsigned int profileID, char** profilesConfigString) {
        if (!profilesConfigString) return false;
        const auto f = profileList.find(profileID);
        if (f == profileList.end()) return false;

        f->second->toString(profilesConfigString);
        return true;
    }

    // Updates a profile from the supplied string
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetProfileConfigFromString(unsigned int profileID, char* profilesConfigString) {
        if (!profilesConfigString) return false;
        auto f = profileList.find(profileID);
        if (f == profileList.end()) return false;

        return f->second->fromString(profilesConfigString);
    }

    // Updates a profile name from the supplied string
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetProfileName(unsigned int profileID, char* profileName) {
        if (!profileName) return false;
        auto f = profileList.find(profileID);
        if (f == profileList.end()) return false;

#ifdef _WIN32
        strcpy_s(f->second->profileName, 128, profileName);
#else
        strncpy(f->second->profileName, profileName, 128);
#endif
        return true;
    }

    // Creates a new profile and returns its unique ID
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateNewProfile(unsigned int driverIndex, unsigned int* profileID) {
        if (!profileID) return false;
        if (driverIndex >= BRIDGE_NumDrivers()) return false;

        *profileID = 1;
        while (profileList.find(*profileID) != profileList.end()) (*profileID)++;

        BridgeConfig* config = new BridgeConfig();
        config->bridgeIndex = driverIndex;

        profileList.insert(std::make_pair(*profileID, config));

        return true;
    }

    // Deletes a profile by ID.
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DeleteProfile(unsigned int profileID) {
        auto f = profileList.find(profileID);
        
        if (f == profileList.end()) return false;

        delete f->second;
        profileList.erase(f);

        return true;
    }
    
    // Return information about a driver.  Returns FALSE if not available
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetDriverInfo(unsigned int driverIndex, FloppyDiskBridge::BridgeDriver** driverInformation) {
        return handleGetDriverInfo(driverIndex, driverInformation);
    }

    // Creates an instance of a driver.  At this point it is not configured, just initialized
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_Close(BridgeOpened* bridgeDriverHandle) {
        if (bridgeDriverHandle) {
            if (bridgeDriverHandle->bridge) {
                bridgeDriverHandle->bridge->shutdown();

                // Restore virtual drives
                releaseVirtualDrives(false, bridgeDriverHandle->config.bridgeIndex);

                delete bridgeDriverHandle->bridge;
                bridgeDriverHandle->bridge = nullptr;

                return true;
            }
        }
        return false;
    }

    // Creates an instance of a driver.  At this point it is not configured, just initialized
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriver(unsigned int driverIndex, BridgeOpened** bridgeDriverHandle) {
        if (driverIndex >= BRIDGE_NumDrivers()) return false;
        if (!bridgeDriverHandle) return false;
        *bridgeDriverHandle = new BridgeOpened;

        // Reset to defaults
        (*bridgeDriverHandle)->config.autoDetectComPort = true;
        memset((*bridgeDriverHandle)->config.comPortToUse, 0, sizeof((*bridgeDriverHandle)->config.comPortToUse));
        (*bridgeDriverHandle)->config.bridgeIndex = driverIndex;

        BRIDGE_GetDriverInfo(driverIndex, &(*bridgeDriverHandle)->driverDetails);

        return true;
    }

    // Creates an instance of a driver from a config string.  This will automatically choose the correct driver index
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriverFromConfigString(char* configString, BridgeOpened** bridgeDriverHandle) {
        if (!bridgeDriverHandle) return false;
        if (!configString) return false;
        if (strlen(configString) < 7) return false;

        std::string tmp = configString;
        if ((tmp[0] != '[') || (tmp[tmp.length() - 1] != ']')) return false;
        tmp = tmp.substr(1, tmp.length() - 2);

        size_t pos = tmp.find('|');
        if (pos == std::string::npos) return false;
        tmp = tmp.substr(0, pos);

        if (tmp.length() < 1) return false;
        unsigned int i = atoi(tmp.c_str());
        if (i >= BRIDGE_NumDrivers()) return false;

        if (!BRIDGE_CreateDriver(i, bridgeDriverHandle)) return false;
        if (!BRIDGE_SetConfigFromString(*bridgeDriverHandle, configString)) {
            BRIDGE_Close(*bridgeDriverHandle);
            *bridgeDriverHandle = nullptr;
            return false;
        }

        return true;
    }

    // Creates an instance of a driver from a config string.  This will automatically choose the correct driver index
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriverFromProfileID(unsigned int profileID, BridgeOpened** bridgeDriverHandle) {
        if (!bridgeDriverHandle) return false;
        auto f = profileList.find(profileID);
        if (f == profileList.end()) return false;

        if (!BRIDGE_CreateDriver(f->second->bridgeIndex, bridgeDriverHandle)) return false;

        char* config;
        f->second->toString(&config);

        if (!(*bridgeDriverHandle)->config.fromString(config)) {
            BRIDGE_Close(*bridgeDriverHandle);
            *bridgeDriverHandle = nullptr;
            return false;
        }

        return true;
    }

    // Opens the driver for this bridge selection.  If it returns false, errorMessage is a pointer to the error, if TRUE, then errorMessage may be set to a warning message for compatability issues
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_Open(BridgeOpened* bridgeDriverHandle, char** errorMessage) {
        if (bridgeDriverHandle->config.bridgeIndex >= BRIDGE_NumDrivers()) return false;

        // Make sure it's not already open
        BRIDGE_Close(bridgeDriverHandle);

        releaseVirtualDrives(true, bridgeDriverHandle->config.bridgeIndex);

#ifdef _WIN32
        HCURSOR lastCursor = SetCursor(LoadCursor(0, IDC_WAIT));
#endif
        memset(bridgeDriverHandle->lastMessage, 0, sizeof(bridgeDriverHandle->lastMessage));

        switch (bridgeDriverHandle->config.bridgeIndex) {
        case 0: bridgeDriverHandle->bridge = new ArduinoFloppyDiskBridge(bridgeDriverHandle->config.bridgeMode, bridgeDriverHandle->config.bridgeDensity, bridgeDriverHandle->config.autoCache, bridgeDriverHandle->config.smartSpeed, bridgeDriverHandle->config.autoDetectComPort, bridgeDriverHandle->config.comPortToUse); break;
        case 1: bridgeDriverHandle->bridge = new GreaseWeazleDiskBridge(bridgeDriverHandle->config.bridgeMode, bridgeDriverHandle->config.bridgeDensity, bridgeDriverHandle->config.autoCache, bridgeDriverHandle->config.smartSpeed, bridgeDriverHandle->config.autoDetectComPort, bridgeDriverHandle->config.comPortToUse, bridgeDriverHandle->config.driveCable); break;
        case 2: bridgeDriverHandle->bridge = new SupercardProDiskBridge(bridgeDriverHandle->config.bridgeMode, bridgeDriverHandle->config.bridgeDensity, bridgeDriverHandle->config.autoCache, bridgeDriverHandle->config.smartSpeed, bridgeDriverHandle->config.autoDetectComPort, bridgeDriverHandle->config.comPortToUse, bridgeDriverHandle->config.driveCable == FloppyBridge::DriveSelection::dsDriveB); break;
        default: return false;
        }

        // Try to open the device.  Returns FALSE if it fails and sets the error message
        bool result = bridgeDriverHandle->bridge->initialise();

#ifdef _WIN32
        SetCursor(lastCursor);
#endif

        // If the device opens successfully, the error message is actually a warning message instead
        const char* error = bridgeDriverHandle->bridge->getLastErrorMessage();
#ifdef _WIN32	
        strcpy_s(bridgeDriverHandle->lastMessage, sizeof(bridgeDriverHandle->lastMessage) - 1, error);
#else
        strcpy(bridgeDriverHandle->lastMessage, error);
#endif

        if (errorMessage)
        {
            if (strlen(bridgeDriverHandle->lastMessage))
            {
                *errorMessage = bridgeDriverHandle->lastMessage;
            }
            else
            {
                *errorMessage = NULL;
            }
        }

        if (!result) {
            // Restore virtual drive
            releaseVirtualDrives(false, bridgeDriverHandle->config.bridgeIndex);

            delete bridgeDriverHandle->bridge;
            bridgeDriverHandle->bridge = nullptr;
        }

        return result;
    }

    // Returns the driverIndex used to open the driver
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetDriverIndex(BridgeOpened* bridgeDriverHandle, unsigned int* driverIndex) {
        if (!bridgeDriverHandle) return false;
        if (!driverIndex) return false;
        *driverIndex = bridgeDriverHandle->config.bridgeIndex;
        return true;
    }

    // Not normally used, and doesnt do anythin if the driver is connected either
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetDriverIndex(BridgeOpened* bridgeDriverHandle, unsigned int driverIndex) {
        if (!bridgeDriverHandle) return false;
        if ((driverIndex<0) || (driverIndex >= MAX_NUM_DRIVERS)) return false;
        if (bridgeDriverHandle->config.bridgeIndex != driverIndex) {
            bridgeDriverHandle->config.bridgeIndex = driverIndex;
            BRIDGE_GetDriverInfo(driverIndex, &bridgeDriverHandle->driverDetails);
        }
        return true;
    }

    // Creates an instance of a driver.  At this point it is not configured, just initialized
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_FreeDriver(BridgeOpened* bridgeDriverHandle) {
        if (!bridgeDriverHandle) return false;
        if (bridgeDriverHandle->bridge) {
            releaseVirtualDrives(false, bridgeDriverHandle->config.bridgeIndex);
            bridgeDriverHandle->bridge->shutdown();
            delete bridgeDriverHandle->bridge;
        }
        delete bridgeDriverHandle;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////// These must be called before opening the driver, but can also be called after ////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Get the currently selected mode
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetMode(BridgeOpened* bridgeDriverHandle, FloppyBridge::BridgeMode* bridgeMode) {
        if (!bridgeDriverHandle) return false;
        if (!bridgeMode) return false;
        *bridgeMode = bridgeDriverHandle->config.bridgeMode;
        return true;
    }
    // Set the currently selected mode.  This can also be changed *while* the driver is running
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetMode(BridgeOpened* bridgeDriverHandle, FloppyBridge::BridgeMode bridgeMode) {
        if (!bridgeDriverHandle) return false;
        if (bridgeMode > FloppyBridge::BridgeMode::bmMax) return false;
        bridgeDriverHandle->config.bridgeMode = bridgeMode;
        if (bridgeDriverHandle->bridge) bridgeDriverHandle->bridge->changeBridgeMode(bridgeMode);
        return true;
    }
    // Get the current DD/HD mode
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetDensityMode(BridgeOpened* bridgeDriverHandle, FloppyBridge::BridgeDensityMode* densityMode) {
        if (!bridgeDriverHandle) return false;
        if (!densityMode) return false;
        *densityMode = bridgeDriverHandle->config.bridgeDensity;
        return true;
    }
    // Get the current DD/HD mode.  This can also be changed *while* the driver is running
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetDensityMode(BridgeOpened* bridgeDriverHandle, FloppyBridge::BridgeDensityMode densityMode) {
        if (!bridgeDriverHandle) return false;
        if (densityMode > FloppyBridge::BridgeDensityMode::bdmMax) return false;
        bridgeDriverHandle->config.bridgeDensity = densityMode;
        if (bridgeDriverHandle->bridge) bridgeDriverHandle->bridge->changeBridgeDensity(densityMode);
        return true;
    }
    // Returns if the driver currently has Smart Speed enabled which can dynamically switch between normal and turbo disk speed without breaking copy protection
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetSmartSpeedEnabled(BridgeOpened* bridgeDriverHandle, bool* enabled) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_SMARTSPEED)) return false;
        if (!enabled) return false;
        (*enabled) = bridgeDriverHandle->config.smartSpeed;
        return true;
    }
    //  Sets if the driver can dynamically switch between normal and turbo disk speed without breaking copy protectionThis can be set while the bridge is in use
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetSmartSpeedEnabled(BridgeOpened* bridgeDriverHandle, bool enabled) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_SMARTSPEED)) return false;
        bridgeDriverHandle->config.smartSpeed = enabled;
        return true;
    }
    // Gets if the driver should continue to cache other cylinders while the drive isn't being used
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetAutoCache(BridgeOpened* bridgeDriverHandle, bool* isAutoCache) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_AUTOCACHE)) return false;
        if (!isAutoCache) return false;
        (*isAutoCache) = bridgeDriverHandle->config.autoCache;
        return true;
    }
    // Sets if the driver should continue to cache other cylinders while the drive isn't being used
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetAutoCache(BridgeOpened* bridgeDriverHandle, bool isAutoCache) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_AUTOCACHE)) return false;
        bridgeDriverHandle->config.autoCache = isAutoCache;
        return true;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////// These must be called before opening the driver ////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get the currently selected COM device for the driver.  You must *not* modify this string
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetCurrentComPort(BridgeOpened* bridgeDriverHandle, char** comPort) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_COMPORT)) return false;
        if (!comPort) return false;
        *comPort = bridgeDriverHandle->config.comPortToUse;
        return true;
    }
    // Set the currently selected COM device for the driver
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetCurrentComPort(BridgeOpened* bridgeDriverHandle, char* comPort) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_COMPORT)) return false;
        if (!comPort) return false;
        if (strlen(comPort) > 120) return false;  // don't overflow
#ifdef _WIN32	
        strcpy_s(bridgeDriverHandle->config.comPortToUse, sizeof(bridgeDriverHandle->config.comPortToUse) - 1, comPort);
#else
        strcpy(bridgeDriverHandle->config.comPortToUse, comPort);
#endif	
        return true;
    }
    // Get the "auto-detect" com port option for the driver
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetAutoDetectComPort(BridgeOpened* bridgeDriverHandle, bool* autoDetectComPort) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_COMPORT_AUTODETECT)) return false;
        if (!autoDetectComPort) return false;
        (*autoDetectComPort) = bridgeDriverHandle->config.autoDetectComPort;
        return true;
    }
    // Set the "auto-detect" com port option for the driver
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetAutoDetectComPort(BridgeOpened* bridgeDriverHandle, bool autoDetectComPort) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_COMPORT_AUTODETECT)) return false;
        bridgeDriverHandle->config.autoDetectComPort = autoDetectComPort;
        return true;
    }
    // Get the cable on the drive where the floppy drive is (A or B) - Obsolete
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetCable(BridgeOpened* bridgeDriverHandle, bool* isOnB) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_DRIVE_AB)) return false;
        if (!isOnB) return false;
        (*isOnB) = bridgeDriverHandle->config.driveCable == FloppyBridge::DriveSelection::dsDriveB;
        return true;
    }
    // Set the cable on the drive where the floppy drive is (A or B) - Obsolete
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetCable(BridgeOpened* bridgeDriverHandle, bool isOnB) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_DRIVE_AB)) return false;
        bridgeDriverHandle->config.driveCable = isOnB ? FloppyBridge::DriveSelection::dsDriveB : FloppyBridge::DriveSelection::dsDriveA;
        return true;
    }
    // Get the cable on the drive where the floppy drive is (A, B, 0, 1 or 2)
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetCable2(BridgeOpened* bridgeDriverHandle, FloppyBridge::DriveSelection* driveSelection) {
        if (!bridgeDriverHandle) return false;
        if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_DRIVE_AB)) return false;
        if (!driveSelection) return false;
        *driveSelection = bridgeDriverHandle->config.driveCable;
        return true;
    }
    // Set the cable on the drive where the floppy drive is (A, B, 0, 1 or 2)
    FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetCable2(BridgeOpened* bridgeDriverHandle, FloppyBridge::DriveSelection driveSelection) {
        if (!bridgeDriverHandle) return false;
        if ((driveSelection == FloppyBridge::DriveSelection::dsDriveA) || (driveSelection == FloppyBridge::DriveSelection::dsDriveB)) {
            if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_DRIVE_AB)) return false;
        }
        else {
            if (!(bridgeDriverHandle->driverDetails->configOptions & CONFIG_OPTIONS_DRIVE_123)) return false;
        }
        bridgeDriverHandle->config.driveCable = driveSelection;
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////// Actual Bridge I/O ///////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Return the 'bit cell' time in uSec.  Standard DD Amiga disks this would be 2uS, HD disks would be 1us I guess, but mainly used for =4 for SD I think
    FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getBitSpeed(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getBitSpeed();
        }
        return 2;
    }
    FLOPPYBRIDGE_API FloppyDiskBridge::DriveTypeID CALLING_CONVENSION DRIVER_getDriveTypeID(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getDriveTypeID();
        }
        return FloppyDiskBridge::DriveTypeID::dti35DD;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_resetDrive(BridgeOpened* bridgeDriverHandle, int trackNumber) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->resetDrive(trackNumber);
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isAtCylinder0(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isAtCylinder0();
        }
        return true;
    }
    FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getMaxCylinder(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getMaxCylinder();
        }
        return 80;
    }
    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_gotoCylinder(BridgeOpened* bridgeDriverHandle, int cylinderNumber, bool side) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            bridgeDriverHandle->bridge->gotoCylinder(cylinderNumber, side);
        }
    }
    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_handleNoClickStep(BridgeOpened* bridgeDriverHandle, bool side) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            bridgeDriverHandle->bridge->handleNoClickStep(side);
        } 
    }
    FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getCurrentCylinderNumber(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getCurrentCylinderNumber();
        }
        return 0;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_getCurrentSide(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getCurrentSide();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isStillWorking(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isStillWorking();
        }
        return false;
    }

    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_setMotorStatus(BridgeOpened* bridgeDriverHandle, bool side, bool turnOn) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            bridgeDriverHandle->bridge->setMotorStatus(side, turnOn);
        }
    }

    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMotorRunning(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isMotorRunning();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isReady(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isReady();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isDiskInDrive(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isDiskInDrive();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_hasDiskChanged(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->hasDiskChanged();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMFMPositionAtIndex(BridgeOpened* bridgeDriverHandle, int mfmPositionBits) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isMFMPositionAtIndex(mfmPositionBits);
        }
        return mfmPositionBits == 0;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMFMDataAvailable(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isMFMDataAvailable();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_getMFMBit(BridgeOpened* bridgeDriverHandle, int mfmPositionBits) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getMFMBit(mfmPositionBits);
        }
        return 1;
    }
    FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_setDirectMode(BridgeOpened* bridgeDriverHandle, bool directMode) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->setDirectMode(directMode);
        }
        return 1;
    }
    FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_getTrack(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track, bool resyncIndex, int bufferSizeInBytes, void* data) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getMFMTrack(side, track, resyncIndex, bufferSizeInBytes, data);
        }
        return 1;
    }
    FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_putTrack(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track, bool writeFromIndex, int bufferSizeInBytes, void* data) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->writeMFMTrackToBuffer(side, track, writeFromIndex, bufferSizeInBytes, data);
        }
        return 1;
    }
    FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_getMFMSpeed(BridgeOpened* bridgeDriverHandle, int mfmPositionBits) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->getMFMSpeed(mfmPositionBits);
        }
        return 1000;
    }
    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_mfmSwitchBuffer(BridgeOpened* bridgeDriverHandle, bool side) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            bridgeDriverHandle->bridge->mfmSwitchBuffer(side);
        }
    }
    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_setSurface(BridgeOpened* bridgeDriverHandle, bool side) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            bridgeDriverHandle->bridge->setSurface(side);
        }
    }
    FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_maxMFMBitPosition(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->maxMFMBitPosition();
        }
        return 6000;
    }
    FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_writeShortToBuffer(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track, unsigned short mfmData, int mfmPosition) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->writeShortToBuffer(side, track, mfmData, mfmPosition);
        }
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWriteProtected(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isWriteProtected();
        }
        return true;
    }
    FLOPPYBRIDGE_API unsigned int CALLING_CONVENSION DRIVER_commitWriteBuffer(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->commitWriteBuffer(side, track);
        }
        return 6000;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWritePending(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isWritePending();
        }
        return false;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWriteComplete(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isWriteComplete();
        }
        return true;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_canTurboWrite(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->canTurboWrite();
        }
        return true;
    }
    FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isReadyToWrite(BridgeOpened* bridgeDriverHandle) {
        if ((bridgeDriverHandle) && (bridgeDriverHandle->bridge)) {
            return bridgeDriverHandle->bridge->isReadyToWrite();
        }
        return false;
    }
}

#ifdef _WIN32

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        hInstance = hinstDLL;
        if (bridgeLogos.size() < 1) {
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BRIDGELOGO0), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BRIDGELOGO1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BRIDGELOGO2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SMALLLOGO0), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SMALLLOGO1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            bridgeLogos.push_back((HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SMALLLOGO2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));           
        }
        break;

    case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#endif
