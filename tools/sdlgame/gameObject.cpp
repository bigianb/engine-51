#include "gameObject.h"

#include "../../a51lib/levels/Level.h"
#include "../../a51lib/levels/LevelLoader.h"
#include "../../a51lib/io/FileSystem.h"
#include "../../a51lib/resourceManager/ResourceManager.h"
#include "../../a51lib/objectManager/ObjectManager.h"
#include "../../a51lib/objects/ObjectRegistrar.h"
#include "../../a51lib/resourceManager/ResourceLoaders.h"
#include "../../a51lib/ui/UIManager.h"
#include "../../a51lib/state/StateMachine.h"
#include "../../a51lib/spatialDBase/SpatialDBase.h"
#include "../../a51lib/Guid.h"
#include "../../a51lib/collisionMgr/CollisionMgr.h"
#include "../../a51lib/collisionMgr/PolyCache.h"
#include "../../a51lib/PlaysurfaceMgr.h"
#include "system/SDL_Engine.h"
#include "system/SDL_Renderer.h"

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

bool GameObject::init()
{
    renderer = new SDLRenderer();
    if (!renderer->init()){
        return false;
    }
    render::Init(resourceManager, renderer);
    
    fs = new FileSystem();
    resourceManager = new ResourceManager();
    resourceManager->setFilesystem(fs);
    resourceLoaders = new ResourceLoaders();
    
    objectManager = new ObjectManager();
    polyCache = new poly_cache(objectManager);

    playsurfaceManager = new PlaysurfaceMgr();

    collisionManager = new collision_mgr();
    collisionManager->setPolycache(polyCache);
    collisionManager->setObjectManager(objectManager);
    collisionManager->setPlaysurfaceManager(playsurfaceManager);
    
    ObjectRegistrarInterface* objectRegistrar = new ObjectRegistrar();
    spatial_dbase* spatialDatabase = new spatial_dbase();
    spatialDatabase->Init(400.0f);
    objectManager->Init(objectRegistrar, spatialDatabase, collisionManager, resourceManager, playsurfaceManager);
    delete objectRegistrar;

    engine = new SDLEngine();
    engine->init();
    guid_Init();

    //    g_IoMgr.Init();

    level = new Level();

    levelLoader = new LevelLoader(fs, resourceManager, objectManager);
    levelLoader->mountDefaultFilesystems();

    /*
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

    levelLoader->loadDFS("BOOT");
    levelLoader->loadDFS("PRELOAD");
    

/*
        g_TracerMgr.Init();
        g_PhysicsMgr.Init();

        LoadCamera();
*/
    uiManager = new ui::Manager();
    uiManager->init(*renderer, resourceManager);
    uiManager->setRes();

    // In strings.dfs
    resourceManager->loadStringTable( "Inventory", "ENG_Inventory_strings.STRINGBIN" );

    stateMachine = new StateMachine();
    stateMachine->init(uiManager, resourceManager);

    //g_RscMgr.TagResources();

    //g_CheckPointMgr.Init(0);
    //g_DebugMenu.Init();
    return true;
}

void GameObject::quit()
{
    if (renderer){
        renderer->quit();
    }
}