#include "gameObject.h"

#include "../../a51lib/levels/LevelLoader.h"
#include "../../a51lib/io/FileSystem.h"
#include "../../a51lib/resourceManager/ResourceManager.h"
#include "../../a51lib/resourceManager/ResourceLoaders.h"
#include "../../a51lib/ui/UIManager.h"
#include "../../a51lib/state/stateMachine.h"

#include "system/QtEngine.h"
#include "system/QtRenderer.h"

GameObject::GameObject()
{
    levelLoader = nullptr;
    fs = nullptr;
    resourceLoaders = nullptr;
    resourceManager = nullptr;
    uiManager = nullptr;
    stateMachine = nullptr;
    renderer = nullptr;
}

GameObject::~GameObject()
{
    delete stateMachine;
    delete uiManager;
    delete levelLoader;
    delete resourceManager;
    delete resourceLoaders;
    delete fs;
    delete renderer;
}

void GameObject::init()
{
    renderer = new QtRenderer();
    levelLoader = new LevelLoader();
    fs = new FileSystem();
    resourceManager = new ResourceManager();
    resourceManager->setFilesystem(fs);
    resourceLoaders = new ResourceLoaders();
    
    engine = new QtEngine();
    engine->init();

    /*
        guid_Init();

        g_IoMgr.Init();
        */

    levelLoader->mountDefaultFilesystems(*fs);

    /*
     g_ObjMgr.Init();
     g_SpatialDBase.Init( 400.0f );
     g_PostEffectMgr.Init();
     g_PlaySurfaceMgr.Init();
     g_DecalMgr.Init();
     g_AudioManager.Init( 5512*1024 );

     anim_event::Init();

     g_MusicMgr.Init();
     g_ConverseMgr.Init();

     g_NetworkMgr.Init();
     g_GameTextMgr.Init();

     // Init stats
     x_memset( &g_Stats, 0, sizeof( g_Stats ) );
     g_Stats.Interval = 1;
*/

    resourceManager->init();
    resourceManager->setRootDirectory("");
    resourceManager->setOnDemandLoading(false);
    resourceLoaders->registerLoaders(resourceManager);

    levelLoader->loadDFS(*fs, resourceManager, "BOOT");
    levelLoader->loadDFS(*fs, resourceManager, "PRELOAD");
    /*
        render::Init();

        g_TracerMgr.Init();
        g_PhysicsMgr.Init();

        LoadCamera();
*/
    uiManager = new ui::Manager();
    uiManager->init(*renderer, resourceManager);
    uiManager->setRes();

//        g_StringTableMgr.LoadTable( "Inventory", xfs("%s\\%s", g_RscMgr.GetRootDirectory(), "ENG_Inventory_strings.stringbin" ) );

    stateMachine = new StateMachine();
    stateMachine->init(uiManager);

    //g_RscMgr.TagResources();

    //g_CheckPointMgr.Init(0);
    //g_DebugMenu.Init();
}
