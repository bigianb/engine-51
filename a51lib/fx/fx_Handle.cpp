
#include "fx_Mgr.h"

fx_handle::fx_handle()
{
    Index = -1;
}

fx_handle::fx_handle(const fx_handle& Handle)
{
    FXMgr.BindHandle(*this, Handle.Index);
}

fx_handle::~fx_handle()
{
    FXMgr.UnbindHandle(*this);
}

const fx_handle& fx_handle::operator=(const fx_handle& Handle)
{
    FXMgr.BindHandle(*this, Handle.Index);
    return (*this);
}

bool fx_handle::InitInstance(const char* pName)
{
    FXMgr.UnbindHandle(*this);

    int  Index = -1;
    bool Success = FXMgr.InitEffect(Index, pName);

    if (Success) {
        FXMgr.BindHandle(*this, Index);
    }

    return (Success);
}

void fx_handle::AdvanceLogic(float DeltaTime)
{
    FXMgr.AdvanceLogic(*this, DeltaTime);
}

void fx_handle::KillInstance()
{
    FXMgr.UnbindHandle(*this);
}

void fx_handle::Render() const
{
    FXMgr.Render(*this);
}

void fx_handle::Restart()
{
    FXMgr.RestartEffect(*this);
}

bool fx_handle::IsFinished() const
{
    return (FXMgr.IsFinished(*this));
}

bool fx_handle::IsInstanced() const
{
    return (FXMgr.IsInstanced(*this));
}

void fx_handle::SetScale(const Vector3& Scale)
{
    FXMgr.SetScale(*this, Scale);
}

void fx_handle::SetRotation(const Radian3& Rotation)
{
    FXMgr.SetRotation(*this, Rotation);
}

void fx_handle::SetTranslation(const Vector3& Translation)
{
    FXMgr.SetTranslation(*this, Translation);
}

Vector3 fx_handle::GetTranslation() const
{
    return FXMgr.GetTranslation(*this);
}

void fx_handle::SetTransform(const Matrix4& L2W)
{
    FXMgr.SetScale(*this, L2W.GetScale());
    FXMgr.SetRotation(*this, L2W.GetRotation());
    FXMgr.SetTranslation(*this, L2W.GetTranslation());
}

void fx_handle::SetColor(const Colour& Color)
{
    FXMgr.SetColor(*this, Color);
}

Colour fx_handle::GetColor()
{
    return FXMgr.GetColor(*this);
}

void fx_handle::SetSuspended(bool Suspended)
{
    FXMgr.SetSuspended(*this, Suspended);
}

const BBox& fx_handle::GetBounds() const
{
    return (FXMgr.GetBounds(*this));
}

bool fx_handle::Validate() const
{
    return (FXMgr.Validate(*this));
}
