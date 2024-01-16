#include "GameLogic.h"
#include "GameRender.h"
#include "AndroidOut.h"

using namespace PlanarPhysics;

GameLogic::GameLogic(GameRender* gameRender)
{
    this->gameRender = gameRender;
    this->keepTicking = true;
    this->threadHandle = 0;
    this->state = nullptr;
}

/*virtual*/ GameLogic::~GameLogic()
{
    delete this->state;
}

bool GameLogic::Setup()
{
    this->keepTicking = true;

    if(0 != pthread_create(&this->threadHandle, nullptr, &GameLogic::ThreadEntryPoint, this))
        return false;

    return true;
}

bool GameLogic::Shutdown()
{
    this->keepTicking = false;

    if(this->threadHandle)
    {
        pthread_join(this->threadHandle, nullptr);
        this->threadHandle = 0;
    }

    return true;
}

bool GameLogic::Tick()
{
    this->timeKeeper.Tick();

    this->physicsWorld.accelerationDueToGravity = this->gameRender->GetGravityVector();

    // Don't advance the physics unless we're also able to render it.
    if(this->gameRender->CanRender())
        this->physicsWorld.Tick();

    if(this->state)
    {
        State* newState = this->state->Tick(this->timeKeeper.GetElapsedTimeSeconds());
        if(newState != this->state)
            this->SetState(newState);
    }

    if(this->gameRender->CanRender())
    {
        DrawHelper *drawHelper = this->gameRender->GetDrawHelper();

        double transitionAlpha = this->state ? this->state->GetTransitionAlpha() : 0.0;

        double aspectRatio = this->gameRender->GetAspectRatio();
        drawHelper->BeginRender(&this->physicsWorld, aspectRatio);

        const std::vector<PlanarObject *> &planarObjectArray = this->physicsWorld.GetPlanarObjectArray();
        for (const PlanarObject *planarObject: planarObjectArray)
        {
            auto mazeObject = dynamic_cast<const MazeObject *>(planarObject);
            if (mazeObject)
            {
                mazeObject->Render(*drawHelper, transitionAlpha);
            }
        }

        Transform textTransform;
        textTransform.scale = (this->progress.GetLevel() < 5) ? (MAZE_CELL_SIZE / 4.0) : (MAZE_CELL_SIZE / 2.0);
        char text[64];
        Color textColor;
        const BoundingBox &worldBox = this->physicsWorld.GetWorldBox();

        textTransform.translation = Vector2D(worldBox.min.x, worldBox.max.y);
        sprintf(text, "Level %d", this->progress.GetLevel());
        textColor = Color(1.0, 1.0, 1.0);
        this->textRenderer.RenderText(text, textTransform, textColor, *drawHelper);

        textTransform.translation = Vector2D(worldBox.min.y, worldBox.min.y - textTransform.scale);
        double frameRate = this->timeKeeper.GetFrameRate();
        sprintf(text, "FPS = %06.2f", frameRate);
        textColor = (frameRate < 60) ? ((frameRate < 30) ? Color(1.0, 0.0, 0.0) : Color(1.0, 1.0, 0.0)) : Color(0.0, 1.0, 0.0);
        this->textRenderer.RenderText(text, textTransform, textColor, *drawHelper);

        this->state->Render(*drawHelper);

        drawHelper->EndRender();
    }

    return true;
}

void GameLogic::SetState(State* newState)
{
    if(this->state)
    {
        this->state->Leave();
        delete this->state;
    }

    this->state = newState;

    if(this->state)
        this->state->Enter();
}

/*static*/ void* GameLogic::ThreadEntryPoint(void* arg)
{
    auto gameLogic = static_cast<GameLogic*>(arg);
    gameLogic->ThreadFunc();
    return nullptr;
}

void GameLogic::ThreadFunc()
{
    this->SetState(new GenerateMazeState(this));

    while(this->keepTicking)
        if(!this->Tick())
            break;

    this->progress.SetTouches(this->physicsWorld.GetGoodMazeBlockTouchedCount());
    this->progress.Save(this->gameRender->GetApp());

    this->SetState(nullptr);

    this->maze.Clear();
    this->physicsWorld.Clear();
}

//------------------------------ GameLogic::State ------------------------------

GameLogic::State::State(GameLogic* game)
{
    this->game = game;
}

/*virtual*/ GameLogic::State::~State()
{
}

/*virtual*/ void GameLogic::State::Enter()
{
}

/*virtual*/ void GameLogic::State::Leave()
{
}

/*virtual*/ GameLogic::State* GameLogic::State::Tick(double deltaTime)
{
    return this;
}

/*virtual*/ double GameLogic::State::GetTransitionAlpha() const
{
    return 1.0;
}

/*virtual*/ void GameLogic::State::Render(DrawHelper& drawHelper) const
{
}

//------------------------------ GameLogic::GenerateMazeState ------------------------------

GameLogic::GenerateMazeState::GenerateMazeState(GameLogic* game) : State(game)
{
}

/*virtual*/ GameLogic::GenerateMazeState::~GenerateMazeState()
{
}

/*virtual*/ void GameLogic::GenerateMazeState::Enter()
{
}

/*virtual*/ void GameLogic::GenerateMazeState::Leave()
{
}

/*virtual*/ GameLogic::State* GameLogic::GenerateMazeState::Tick(double deltaTime)
{
    if(!this->game->gameRender->CanRender())
        return this;

    Maze& maze = this->game->maze;
    Engine& physicsEngine = this->game->physicsWorld;
    Options& options = this->game->gameRender->GetOptions();

    maze.Clear();

    if(!this->game->progress.Load(this->game->gameRender->GetApp()))
    {
        aout << "Failed to load progress!" << std::endl;
        this->game->progress.Reset();
    }

    int level = this->game->progress.GetLevel();
    int rows = level + 5;
    int cols = (int)::round(double(rows) * this->game->gameRender->GetAspectRatio());

    aout << "Level " << level << " is a maze of size " << rows << " by " << cols << "." << std::endl;

    bool queen = (level == FINAL_GRAVITY_MAZE_LEVEL);
    maze.Generate(rows, cols, this->game->progress.GetSeedModifier());
    maze.PopulatePhysicsWorld(&physicsEngine, this->game->progress.GetTouches(), queen, options.bounce);

    physicsEngine.accelerationDueToGravity = Vector2D(0.0, -options.gravity);

    return new FlyMazeInState(this->game);
}

//------------------------------ GameRender::FlyMazeInState ------------------------------

GameLogic::FlyMazeInState::FlyMazeInState(GameLogic* game) : State(game)
{
    this->animRate = 1.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ GameLogic::FlyMazeInState::~FlyMazeInState()
{
}

/*virtual*/ void GameLogic::FlyMazeInState::Enter()
{
    this->transitionAlpha = 0.0;

    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Random::Seed((unsigned)::time(nullptr));

    Vector2D verticalTranslation(0.0, worldBox.Height());
    Vector2D horizontalTranslation(worldBox.Width(), 0.0);
    Vector2D diagATranslation(worldBox.Width(), worldBox.Height());
    Vector2D diagBTranslation(worldBox.Width(), -worldBox.Height());

    std::vector<BoundingBox> outerWorldBoxArray;
    outerWorldBoxArray.push_back(worldBox.Translated(verticalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-verticalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(horizontalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-horizontalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(diagATranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-diagATranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(diagBTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-diagBTranslation));

    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject* mazeObject = dynamic_cast<MazeObject*>(planarObject);
        if(mazeObject)
        {
            int i = Random::Integer(0, outerWorldBoxArray.size() - 1);
            double angle = Random::Number(0.0, 2.0 * PLNR_PHY_PI);
            mazeObject->sourceTransform.Identity();
            mazeObject->sourceTransform.translation = outerWorldBoxArray[i].RandomPoint();
            mazeObject->sourceTransform.rotation = PScalar2D(angle).Exponent();
            mazeObject->targetTransform.Identity();
        }
    }
}

/*virtual*/ void GameLogic::FlyMazeInState::Leave()
{
}

/*virtual*/ GameLogic::State* GameLogic::FlyMazeInState::Tick(double deltaTime)
{
    this->transitionAlpha += this->animRate * deltaTime;
    if(this->transitionAlpha > 1.0)
        return new PlayGameState(this->game);

    return this;
}

/*virtual*/ double GameLogic::FlyMazeInState::GetTransitionAlpha() const
{
    return this->transitionAlpha;
}

//------------------------------ GameRender::FlyMazeOutState ------------------------------

GameLogic::FlyMazeOutState::FlyMazeOutState(GameLogic* game) : State(game)
{
    this->animRate = 1.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ GameLogic::FlyMazeOutState::~FlyMazeOutState()
{
}

/*virtual*/ void GameLogic::FlyMazeOutState::Enter()
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Vector2D center = worldBox.Center();

    // TODO: This doesn't work as expected.  Why?
    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject* mazeObject = dynamic_cast<MazeObject*>(planarObject);
        if(mazeObject)
        {
            mazeObject->sourceTransform.Identity();
            mazeObject->targetTransform.Identity();
            mazeObject->targetTransform.translation = center - mazeObject->GetPosition();
            mazeObject->targetTransform.scale = 0.0;
        }
    }
}

/*virtual*/ void GameLogic::FlyMazeOutState::Leave()
{
}

/*virtual*/ GameLogic::State* GameLogic::FlyMazeOutState::Tick(double deltaTime)
{
    this->transitionAlpha += this->animRate * deltaTime;
    if(this->transitionAlpha > 1.0)
    {
        MazeQueen* mazeQueen = this->game->physicsWorld.FindTheQueen();
        if((mazeQueen && !mazeQueen->alive))
            return new GameWonState(this->game);

        return new GenerateMazeState(this->game);
    }

    return this;
}

/*virtual*/ double GameLogic::FlyMazeOutState::GetTransitionAlpha() const
{
    return this->transitionAlpha;
}

//------------------------------ GameRender::PlayGameState ------------------------------

GameLogic::PlayGameState::PlayGameState(GameLogic* game) : State(game)
{
}

/*virtual*/ GameLogic::PlayGameState::~PlayGameState()
{
}

/*virtual*/ void GameLogic::PlayGameState::Enter()
{
}

/*virtual*/ void GameLogic::PlayGameState::Leave()
{
}

/*virtual*/ GameLogic::State* GameLogic::PlayGameState::Tick(double deltaTime)
{
    if(this->game->physicsWorld.IsMazeSolved())
    {
        MazeQueen* mazeQueen = this->game->physicsWorld.FindTheQueen();
        if(!mazeQueen)
            this->game->progress.SetLevel(this->game->progress.GetLevel() + 1);
        else
            this->game->progress.Reset();

        this->game->progress.SetTouches(0);
        this->game->progress.Save(this->game->gameRender->GetApp());

        return new FlyMazeOutState(this->game);
    }

    return this;
}

//------------------------------ GameLogic::GameWonState ------------------------------

GameLogic::GameWonState::GameWonState(GameLogic* game) : State(game)
{
}

/*virtual*/ GameLogic::GameWonState::~GameWonState()
{
}

/*virtual*/ void GameLogic::GameWonState::Enter()
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject *mazeObject = dynamic_cast<MazeObject *>(planarObject);
        if (mazeObject)
        {
            mazeObject->sourceTransform.Identity();
            mazeObject->targetTransform.Identity();
            mazeObject->targetTransform.translation = 2.0 * worldBox.max;   // Move them all off screen.
        }
    }
}

/*virtual*/ void GameLogic::GameWonState::Leave()
{
}

/*virtual*/ double GameLogic::GameWonState::GetTransitionAlpha() const
{
    return 1.0;
}

/*virtual*/ GameLogic::State* GameLogic::GameWonState::Tick(double deltaTime)
{
    return this;
}

/*virtual*/ void GameLogic::GameWonState::Render(DrawHelper& drawHelper) const
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Transform textToWorld;
    textToWorld.scale = 80.0;
    textToWorld.translation = Vector2D(0.0, worldBox.max.y / 2.0);
    this->game->textRenderer.RenderText("YOU WIN!!!", textToWorld, Color(1.0, 1.0, 1.0), drawHelper);

    // TODO: Can the user choose here to add their name to a database of game winners?
    //       Where could I host such a database?  Not for free, certainly, so maybe I won't bother.
}

//------------------------------ GameRender::PhysicsWorld ------------------------------

GameLogic::PhysicsWorld::PhysicsWorld()
{
}

/*virtual*/ GameLogic::PhysicsWorld::~PhysicsWorld()
{
}

bool GameLogic::PhysicsWorld::IsMazeSolved()
{
    return this->GetGoodMazeBlockCount() == this->GetGoodMazeBlockTouchedCount() && this->QueenDeadOrNonExistent();
}

int GameLogic::PhysicsWorld::GetGoodMazeBlockCount()
{
    int count = 0;
    for(auto planarObject : this->GetPlanarObjectArray())
        if(dynamic_cast<GoodMazeBlock*>(planarObject))
            count++;

    return count;
}

int GameLogic::PhysicsWorld::GetGoodMazeBlockTouchedCount()
{
    int count = 0;

    for(auto planarObject : this->GetPlanarObjectArray())
    {
        auto goodMazeBlock = dynamic_cast<GoodMazeBlock *>(planarObject);
        if (goodMazeBlock && goodMazeBlock->IsTouched())
            count++;
    }

    return count;
}

bool GameLogic::PhysicsWorld::QueenDeadOrNonExistent()
{
    MazeQueen* mazeQueen = this->FindTheQueen();
    return !mazeQueen || !mazeQueen->alive;
}

MazeQueen* GameLogic::PhysicsWorld::FindTheQueen()
{
    for(auto planarObject : this->GetPlanarObjectArray())
    {
        auto mazeQueen = dynamic_cast<MazeQueen *>(planarObject);
        if (mazeQueen)
            return mazeQueen;
    }

    return nullptr;
}