#include "LevelLoader.h"

#include <set>
#include <iostream>
#include <cassert>

#include "../io/FileSystem.h"
#include "../DFSFile.h"
#include "../resourceManager/ResourceManager.h"
#include "../dataUtil/TextIn.h"

void LevelLoader::mountDefaultFilesystems()
{
    fs->mount("BOOT", 1);
    fs->mount("PRELOAD", 1);
    fs->mount("AUDIO/MUSIC", 10);
    fs->mount("AUDIO/VOICE", 11);
    fs->mount("AUDIO/HOT", 12);
    fs->mount("AUDIO/AMBIENT", 12);
    fs->mount("STRINGS", 13);
    fs->mount("COMMON", 14);
}

void LevelLoader::loadDFS(std::string dfsName)
{
    static const std::set<std::string> extensions{
        ".XBMP",
        ".RIGIDGEOM",
        ".SKINGEOM",
        ".ANIM",
        ".DECALPKG",
        ".ENVMAP",
        ".RIGIDCOLOR",
        ".STRINGBIN",
        ".FXO",
        ".AUDIOPKG",
        ".FONT"};

    DFSFile* dfsFile = fs->getMount(dfsName);
    if (dfsFile == nullptr) {
        std::cout << "ERROR: could not find mount for " << dfsName << std::endl;
        return;
    }
    int numFiles = dfsFile->numFiles();
    for (int i = 0; i < numFiles; ++i) {
        std::string extension = dfsFile->getFileExtension(i);
        //std::cout << "Extension is " << extension << std::endl;
        if (extensions.contains(extension)) {
            //std::cout << "Reading " << dfsFile->getFilename(i) << std::endl;
            ResourceHandleBase handle(resourceManager);
            handle.setName(dfsFile->getFilename(i));
            handle.getPointer();
        }
    }
}

void LevelLoader::loadLevel(bool fullLoad, const map_entry* pMapEntry)
{
    levelLoaded = false;
    loadRequested = true;
    this->fullLoad = fullLoad;
    mapEntry = pMapEntry;

    assert(loadThread == nullptr);
    loadThread = new std::thread(&LevelLoader::loadLevelThreadFunction, this);
}

void LevelLoader::finishLoading()
{
    assert(loadThread != nullptr);
    loadThread->join();
    delete loadThread;
    loadThread = nullptr;
    loadRequested = false;
}

int convertPathChar(int c)
{
    if (c == '\\') {
        return '/';
    }
    return toupper(c);
}

void fixup_path(char* str, int len)
{
    for (int i = 0; i < len; ++i) {
        char c = str[i];
        if (c == 0) {
            break;
        }
        if (c == '\\') {
            c = '/';
        } else {
            c = toupper(c);
        }
        str[i] = c;
    }
}

void LevelLoader::loadLevelThreadFunction()
{
    std::cout << "Loading level..." << std::endl;

    if (fullLoad) {
        // Reset the static player member variable that indicates death state.
        // this is used by the state manager to determine if any pre-level
        // cinematics can be played.
        //  player::s_bPlayerDied = FALSE;
    }

    //    m_VoiceID   = 0;

    //   g_PerceptionMgr.Init();

    //   g_level_loading = TRUE;

    char pPath[256];
    char pPath2[256];
    char pPath3[256];
    //    slot_id SlotID;

    std::cout << "loading level  " << mapEntry->GetDisplayName() << std::endl;

    std::string levelDFS = "LEVELS/" + mapEntry->GetFilename() + "/LEVEL";

    // convert levelDFS to uppercase
    std::transform(levelDFS.begin(), levelDFS.end(), levelDFS.begin(), ::convertPathChar);
    std::cout << "LevelDFS: " << levelDFS << std::endl;
    fs->mount(levelDFS, 1);

    if (fullLoad) {
        // Initialize animation system
        // NOTE: This must be done BEFORE any data is loaded.
        //anim_event::Init();

        // Load the slide show script.
        InitSlideShow("SLIDESHOWSCRIPT.TXT");

        // Open the load script.
        text_in     TextIn;
        const char* textData = (const char*)resourceManager->getResourceData("LOADSCRIPT.TXT");
        TextIn.OpenText(textData);
        TextIn.ReadHeader();

        // Execute the load script.
        int nCommands = TextIn.GetHeaderCount();
        for (int i = 0; i < nCommands; i++) {
            char command[256];
            char arguments[256];

            TextIn.ReadFields();
            TextIn.GetString("command", command);
            TextIn.GetString("arguments", arguments);

            fixup_path(arguments, 256);

            if (strcmp(command, "load_dfs") == 0) {
                std::cout << "Loading dfs " << arguments << std::endl;
                fs->mount(arguments, 3);
                loadDFS(arguments);
                //fs->unmount(hddargs);
            } else if (strcmp(command, "load_resource") == 0) {
                std::cout << "Loading resource " << arguments << std::endl;
                //rhandle_base Handle;
                //Handle.SetName(arguments);
                //Handle.GetPointer();
            } else if (strcmp(command, "mount_dfs") == 0) {
                std::cout << "Mounting dfs " << arguments << std::endl;
                fs->mount(arguments, 3);
            } else if (strcmp(command, "unmount_dfs") == 0) {
                //fs->unmount(hddargs);
            }
        }
        /*
                                g_DataVault.Init();
                                LoadTweaks( g_FullPath );
                                LoadPain( g_FullPath );

                                // Create permanent objects
                                g_ObjMgr.CreateObject("god") ;

                                // Load the NavMap
                                x_makepath( pPath, NULL, g_FullPath, "level_data", ".nmp" );
                                g_NavMap.Load( pPath );

                                // Load Globals Variables...
                                x_makepath( pPath, NULL, g_FullPath, "level_data", ".glb" );
                                {
                                    MEMORY_OWNER( "GLOBAL VARIABLE DATA" );
                                    g_VarMgr.LoadGlobals( pPath );
                                }

                                // Setup resource handles to rigid color table
                                x_makepath( pPath, NULL, g_FullPath, "level_data", ".rigidcolor" );

                                // Force the rigid color instance to be loaded prior to level init
                                // since it requires a large allocation

                                {
                                    rhandle_base Handle;
                                    Handle.SetName(pPath);
                                    Handle.GetPointer();
                                }

                                x_makepath( pPath, NULL, g_FullPath, "level_data", ".info" );
                                LoadInfo( pPath );
                        */
    }
    /*
        // Create god, proxy play surface, load the globals.
        if( !bFullLoad )
        {
            g_ObjMgr.CreateObject("god");
            g_PlaySurfaceMgr.CreateProxyPlaySurfaceObject();

            // Load the NavMap
            x_makepath( pPath, NULL, g_FullPath, "level_data", ".nmp" );
            g_NavMap.Load( pPath );

            // Load Globals Variables...
            x_makepath( pPath, NULL, g_FullPath, "level_data", ".glb" );
            {
                MEMORY_OWNER( "GLOBAL VARIABLE DATA" );
                g_VarMgr.LoadGlobals( pPath );
            }
        }

        // Load the level
        x_makepath( pPath,      NULL, g_FullPath, "level_data", ".bin_level" );
        x_makepath( pPath2,     NULL, g_FullPath, "level_data", ".lev_dict" );
        x_makepath( pPath3,     NULL, g_FullPath, "level_data", ".load" );

        g_BinLevelMgr.LoadLevel( pPath, pPath2, pPath3 );

        if( bFullLoad )
        {
            {
                // load the rigid colors...they will be assigned to the
                // geometry in a moment
                rhandle<color_info> hRigidColor;
                x_makepath( pPath, NULL, g_FullPath, "level_data", ".rigidcolor" );
                hRigidColor.SetName( pPath );
                hRigidColor.GetPointer();
            }

            {
                MEMORY_OWNER("TEMPLATE DATA");
                //load templates
                x_makepath( pPath, NULL, g_FullPath, "level_data", ".templates" );
                x_makepath( pPath2, NULL, g_FullPath, "level_data", ".tmpl_dct" );
                g_TemplateMgr.LoadData(pPath, pPath2);
            }

            {
                MEMORY_OWNER("ZONE DATA");
                //load portal/zone list
                x_makepath( pPath, NULL, g_FullPath, "level_data", ".zone" );
                g_ZoneMgr.Load(pPath);
            }

            {
                MEMORY_OWNER("PLAYSURFACE DATA");
                //load playsurfaces
                x_makepath( pPath, NULL, g_FullPath, "level_data", ".playsurface" );
                g_PlaySurfaceMgr.OpenFile(pPath, TRUE);
                g_PlaySurfaceMgr.LoadAllZones();
                g_PlaySurfaceMgr.CloseFile();
            }

            {
                MEMORY_OWNER("STATIC DECAL DATA");
                //load static decals
                x_makepath( pPath, NULL, g_FullPath, "level_data", ".decals" );
                g_DecalMgr.LoadStaticDecals( pPath );
            }
        }

        //initialize player tracker
        SlotID = g_ObjMgr.GetFirst( object::TYPE_PLAYER );

        while(SlotID != SLOT_NULL)
        {
            object_ptr<player> PlayerObj( g_ObjMgr.GetObjectBySlot( SlotID ) );

            if (PlayerObj.IsValid())
            {
                PlayerObj.m_pObject->InitZoneTracking();
            }

            SlotID = g_ObjMgr.GetNext( SlotID );
        }


        // Clear the polycache
        g_PolyCache.InvalidateAllCells();

        g_level_loading = FALSE;

    #ifndef X_RETAIL
        // setup automonkey in non-retail builds
        g_Monkey.SetAutoMonkeyMode(g_Config.AutoMonkeyMode);
    #endif

        // reset the rigid color pointers
        x_makepath( pPath, NULL, g_FullPath, "level_data", ".rigidcolor" );
        g_BinLevelMgr.SetRigidColor( pPath );

        // reset any screen fades we might've had on
        g_PostEffectMgr.StartScreenFade( xcolor(0,0,0,0), 0.0f );

        // reset the audio that may have been faded at some point
        g_AudioMgr.SetMasterVolume( 1.0f );

        MsgMgr.Init();
        g_MusicMgr.Init();

        // Setup OccluderMgr and search invis walls for occluders
        g_OccluderMgr.Init();
        g_OccluderMgr.GatherOccluders();

        // Reset DecalMgr
        g_DecalMgr.ResetDynamicDecals();

        // Setup the global game timer
        g_GameTimer.Reset();
        g_GameTimer.Start();

        // inflate the world bounds a bit
        g_ObjMgr.InflateSafeBBox( 1000.0f );

        // reset the frame count to 0
        g_nLogicFramesAfterLoad = 0;

        if( bFullLoad )
        {
            KillSlideShow();
        }

        // Unmount the level.dfs file.
        if( pMapEntry->GetFlags() & MF_DOWNLOAD_MAP )
        {
            UnloadContent();
        }
        else
        {
            g_IOFSMgr.UnmountFileSystem( (const char*)LevelDFS );
        }
            */
}

void LevelLoader::InitSlideShow(const char* pSlideShowScriptFile)
{
}
