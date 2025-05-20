#include "LevelLoader.h"

#include <set>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <cstring>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#include "../io/FileSystem.h"
#include "../DFSFile.h"
#include "../resourceManager/ResourceManager.h"
#include "../objectManager/ObjectManager.h"
#include "../dataUtil/TextIn.h"
#include "../objects/Player.h"

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

void LevelLoader::loadLevel(bool fullLoad, const map_entry* pMapEntry, Level* level)
{
    m_bSpawnInfoSet= false;
    levelLoaded = false;
    loadRequested = true;
    this->fullLoad = fullLoad;
    this->level = level;
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
        player::s_bPlayerDied = false;
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
                fs->unmount(arguments);
            } else if (strcmp(command, "load_resource") == 0) {
                std::cout << "Loading resource " << arguments << std::endl;
                ResourceHandleBase handle(resourceManager);
                handle.setName(arguments);
                handle.getPointer();
            } else if (strcmp(command, "mount_dfs") == 0) {
                std::cout << "Mounting dfs " << arguments << std::endl;
                fs->mount(arguments, 3);
            } else if (strcmp(command, "unmount_dfs") == 0) {
                fs->unmount(arguments);
            }
        }
        /*
                g_DataVault.Init();
                LoadTweaks( g_FullPath );
                LoadPain( g_FullPath );
*/
        /*

                        // Setup resource handles to rigid color table
                        x_makepath( pPath, NULL, g_FullPath, "level_data", ".rigidcolor" );

                        // Force the rigid color instance to be loaded prior to level init
                        // since it requires a large allocation

                        {
                            rhandle_base Handle;
                            Handle.SetName(pPath);
                            Handle.GetPointer();
                        }
        */

        const char* levelInfoData = (const char*)resourceManager->getResourceData("LEVEL_DATA.INFO");
        loadInfo( levelInfoData );
    } else {
        // not full load
        //g_PlaySurfaceMgr.CreateProxyPlaySurfaceObject();
    }

    // Create permanent objects
    objectManager->CreateObject("god");

    int            nmpLength = 0;
    const uint8_t* nmpData = fs->readFile("LEVEL_DATA.NMP", nmpLength);
    level->navMap.Load(nmpData, nmpLength);

    int            glbLength = 0;
    const uint8_t* glbData = fs->readFile("LEVEL_DATA.GLB", glbLength);
    level->varMgr.Load(glbData, glbLength);

    int            binLevelLength = 0;
    const uint8_t* binLevelData = fs->readFile("LEVEL_DATA.BIN_LEVEL", binLevelLength);
    int            levDictLength = 0;
    const uint8_t* levelDictData = fs->readFile("LEVEL_DATA.LEV_DICT", levDictLength);

    level->binLevel.loadLevel(objectManager, binLevelData, binLevelLength, levelDictData, levDictLength);

    if (fullLoad) {
        /*
        {
            // load the rigid colors...they will be assigned to the
            // geometry in a moment
            rhandle<color_info> hRigidColor;
            x_makepath( pPath, NULL, g_FullPath, "level_data", ".rigidcolor" );
            hRigidColor.SetName( pPath );
            hRigidColor.GetPointer();
        }
            */
        /*
                    {
                        //load templates
                        x_makepath( pPath, NULL, g_FullPath, "level_data", ".templates" );
                        x_makepath( pPath2, NULL, g_FullPath, "level_data", ".tmpl_dct" );
                        g_TemplateMgr.LoadData(pPath, pPath2);
                    }

                    {
                        //load portal/zone list
                        x_makepath( pPath, NULL, g_FullPath, "level_data", ".zone" );
                        g_ZoneMgr.Load(pPath);
                    }

                    {
                        //load playsurfaces
                        x_makepath( pPath, NULL, g_FullPath, "level_data", ".playsurface" );
                        g_PlaySurfaceMgr.OpenFile(pPath, TRUE);
                        g_PlaySurfaceMgr.LoadAllZones();
                        g_PlaySurfaceMgr.CloseFile();
                    }

                    {
                        //load static decals
                        x_makepath( pPath, NULL, g_FullPath, "level_data", ".decals" );
                        g_DecalMgr.LoadStaticDecals( pPath );
                    }
                        */
    }

    //initialize player tracker
    /*
    SlotID = objectManager->GetFirst( Object::TYPE_PLAYER );

    while(SlotID != SLOT_NULL)
    {
        object_ptr<player> PlayerObj( g_ObjMgr.GetObjectBySlot( SlotID ) );

        if (PlayerObj.IsValid())
        {
            PlayerObj.m_pObject->InitZoneTracking();
        }

        SlotID = g_ObjMgr.GetNext( SlotID );
    }
*/
    /*
            // Clear the polycache
            g_PolyCache.InvalidateAllCells();

            g_level_loading = FALSE;

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
        */
    fs->unmount(levelDFS);

    levelLoaded = true;
}

void LevelLoader::loadInfo( const char* levelInfoData )
{
    text_in InfoTextIn;
    InfoTextIn.OpenText( levelInfoData );

    while ( InfoTextIn.ReadHeader() )
    {
        if ( strcasecmp( InfoTextIn.GetHeaderName(), "Info" ) == 0 )
        {
            BBox WorldBBox;
            /*
            WorldBBox.Clear();
            InfoTextIn.ReadFields();
            InfoTextIn.GetBBox( "WorldBBox", WorldBBox );
            g_ObjMgr.SetSafeBBox( WorldBBox );
            */
        }

        if ( strcasecmp( InfoTextIn.GetHeaderName(), "PlayerInfo" ) == 0 )
        {
            Vector3 Position;
            Radian  Pitch;
            Radian  Yaw;
            int     Zone;
            guid    Guid;
            InfoTextIn.ReadFields();
            InfoTextIn.GetVector3(  "Position", Position    );
            InfoTextIn.GetF32(      "Pitch",    Pitch       );
            InfoTextIn.GetF32(      "Yaw",      Yaw         );
            InfoTextIn.GetS32(      "Zone",     Zone        );
            InfoTextIn.GetGuid(     "PlayerGuid", Guid      );

            SetPlayerSpawnInfo( Position, Pitch, Yaw, Zone, Guid );
        }
    }
}

void LevelLoader::SetPlayerSpawnInfo(const Vector3& Position,
                                        Radian         Pitch,
                                        Radian         Yaw,
                                        int            Zone,
                                        const guid&    Guid)
{
    m_bSpawnInfoSet = true;
    m_PlayerSpawnPosition = Position;
    m_PlayerSpawnPitch = Pitch;
    m_PlayerSpawnYaw = Yaw;
    m_PlayerSpawnZone = Zone;
    m_PlayerSpawnGuid = Guid;
}

void LevelLoader::spawnPlayer()
{
    if (m_bSpawnInfoSet) {
        objectManager->CreateObject("Player", m_PlayerSpawnGuid);
        player* pPlayer = (player*)objectManager->GetObjectByGuid(m_PlayerSpawnGuid);

        assert(pPlayer);
        if (pPlayer) {
            // Setting position in orientation
            Matrix4 L2W;
            L2W.Identity();
            L2W.RotateY(m_PlayerSpawnYaw);
            L2W.Translate(m_PlayerSpawnPosition);
            pPlayer->OnTransform(L2W);

            // set this player up
            pPlayer->SetPitch(m_PlayerSpawnPitch);
            pPlayer->SetZone1(m_PlayerSpawnZone);
            pPlayer->SetZone2(0);
            pPlayer->InitZoneTracking();
        }
    } else {
        assert(false);
    }
}

void LevelLoader::InitSlideShow(const char* pSlideShowScriptFile)
{
}
