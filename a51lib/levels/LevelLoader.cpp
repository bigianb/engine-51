#include "LevelLoader.h"

#include <set>
#include <iostream>
#include <cassert>

#include "../io/FileSystem.h"
#include "../DFSFile.h"
#include "../resourceManager/ResourceManager.h"

void LevelLoader::mountDefaultFilesystems(FileSystem& fs)
{
    fs.mount("BOOT", 1);
    fs.mount("PRELOAD", 1);
    fs.mount("AUDIO/MUSIC", 10);
    fs.mount("AUDIO/VOICE", 11);
    fs.mount("AUDIO/HOT", 12);
    fs.mount("AUDIO/AMBIENT", 12);
    fs.mount("STRINGS", 13);
    fs.mount("COMMON", 14);
}

void LevelLoader::loadDFS(FileSystem& fs, ResourceManager* rm, std::string dfsName)
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

    DFSFile* dfsFile = fs.getMount(dfsName);
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
            ResourceHandleBase handle(rm);
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

void LevelLoader::loadLevelThreadFunction()
{
    std::cout << "Loading level..." << std::endl;

    if (fullLoad)
    {
        // Reset the static player member variable that indicates death state.
        // this is used by the state manager to determine if any pre-level
        // cinematics can be played.
      //  player::s_bPlayerDied = FALSE;
    }

//    m_VoiceID   = 0;

 //   g_PerceptionMgr.Init();

 //   g_level_loading = TRUE;

    char pPath  [ 256 ];
    char pPath2 [ 256 ];
    char pPath3 [ 256 ];
//    slot_id SlotID;

    std::cout << "loading level id = " << mapEntry->GetMapID() << std::endl;

/*
    GameMgr.SetZoneMinimum( pMapEntry->GetMinPlayers() );

    xfs LevelDFS( "levels/%s/level", g_ActiveConfig.GetLevelPath() );

    LOG_MESSAGE( "LoadLevel", "BEGIN! Level:%s, Memory Free:%d bytes",pMapEntry->GetDisplayName(),x_MemGetFree() );

    // Mount the level.dfs file.
    g_IOFSMgr.MountFileSystem( (const char*)LevelDFS, 2 );
    
    if( bFullLoad )
    {      
        // Initialize animation system
        // NOTE: This must be done BEFORE any data is loaded.
        anim_event::Init();

        // Multiplayer: hasa to be here because BeginSession is creating render targets
        // and clearing them. If another thread is active during the Clear() BOOM!
        render::BeginSession( g_NetworkMgr.GetLocalPlayerCount() );

        // Load the slide show script.
        InitSlideShow( xfs("%s\\%s", g_RscMgr.GetRootDirectory(), "SlideShowScript.txt") );

        // Open the load script.
        text_in TextIn;
        TextIn.OpenFile( xfs("%s\\%s", g_RscMgr.GetRootDirectory(), "LoadScript.txt") );
        TextIn.ReadHeader();

        // Execute the load script.
        s32  nCommands = TextIn.GetHeaderCount();
        xtimer DeltaTime;

        DeltaTime.Reset();
        DeltaTime.Start();

        for( s32 i=0; i < nCommands; i++ )
        {
            char command[256];
            char arguments[256];
            char hddargs[256];

            TextIn.ReadFields();
            TextIn.GetString( "command",   command );
            TextIn.GetString( "arguments", arguments );

            x_strcpy( hddargs, arguments );
            if( x_strncmp( (const char*)LevelDFS, "HDD:",4 ) == 0 )
            {
                x_sprintf( hddargs, "HDD:%s", arguments );
            }

            if( x_strcmp( command, "load_dfs" ) == 0 )
            {
                g_IOFSMgr.MountFileSystem( hddargs, 3 );
                LoadDFS( hddargs );
                g_IOFSMgr.UnmountFileSystem( hddargs );
            }
            else if( x_strcmp( command, "load_resource" ) == 0 )
            {
                rhandle_base Handle;
                Handle.SetName( arguments );
                Handle.GetPointer();
            }
            else if( x_strcmp( command, "mount_dfs" ) == 0 )
            {
                g_IOFSMgr.MountFileSystem( hddargs, 3 );
            }
            else if( x_strcmp( command, "unmount_dfs" ) == 0 )
            {
                g_IOFSMgr.UnmountFileSystem( hddargs );
            }

        }

        TextIn.CloseFile();

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
    }

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