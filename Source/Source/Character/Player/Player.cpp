#include "GameTimer.h"
#include "Player.h"

using namespace DirectX;

XMFLOAT3 videoCharacterPos = { 0, 0, 0 };
//XMVECTOR originPlayerPosition = {0, 0, 0};

Player::Player()
	: Character(),
	mPlayerInfo(),
	mFullHealth(100),
	mDamage(10)
{
	//XMVECTOR P = XMVectorSet(-180.0f, 0.0f, -170.0f, 0.0f);
	XMVECTOR P = XMVectorSet(5.0f, 0.0f, 5.0f, 0.0f);
	//XMVECTOR P = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	mPlayerInfo.mMovement.SetPlayerPosition(P);

	//XMVECTOR rotAngle = XMVectorSet(0.0f, MathHelper::Pi / 2, 0.0f, 0.0f);
	//mPlayerInfo.mMovement.SetPlayerRotation(rotAngle);
	mPlayerInfo.mMovement.AddYaw(MathHelper::Pi / 2);
	XMStoreFloat3(&originPlayerPosition, P);
	//originPlayerPosition = P;
	//float radius =
	/*XMVECTOR originPlayerPosition = mPlayerInfo.mMovement.GetPlayerPosition();*/
}

Player::~Player()
{

}


int Player::GetHealth(int i) const
{
	return mPlayerInfo.mHealth;
}

CharacterInfo & Player::GetCharacterInfo(int cIndex)
{
	return mPlayerInfo;
}

void Player::Damage(int damage, XMVECTOR Position, XMVECTOR Look)
{
	XMVECTOR mP = Position;
	XMVECTOR P = mPlayerInfo.mMovement.GetPlayerPosition();

	if (MathHelper::getDistance(mP, P) > 15.0f)
		return;

	if (mPlayerInfo.mHealth >= 0)
	{
		mSkinnedModelInst->TimePos = 0.0f;
	}
	else
	{
		return;
	}

	SetClipName("HitReaction");
	mPlayerInfo.mHealth -= damage;

	mUI.SetDamageScale(static_cast<float>(mPlayerInfo.mHealth) / static_cast<float>(mFullHealth));
}

void Player::Attack(Character * inMonster, std::string clipName)
{
	SetClipName(clipName);
	SetClipTime(0.0f);

	if (clipName == "Hook")
		mDamage = 10;
	else if (clipName == "Kick")
		mDamage = 20;
	else if (clipName == "Kick2")
		mDamage = 30;

	inMonster->Damage(
		mDamage,
		mPlayerInfo.mMovement.GetPlayerPosition(),
		mPlayerInfo.mMovement.GetPlayerLook());
}

bool Player::isClipEnd()
{
	auto clipEndTime = mSkinnedInfo.GetAnimation(mPlayerInfo.mClipName).GetClipEndTime();
	auto curTimePos = mSkinnedModelInst->TimePos;
	if (clipEndTime - curTimePos < 0.001f)
		return true;
	return false;
}

eClipList Player::GetCurrentClip() const
{
	return mSkinnedModelInst->mState;
}

XMMATRIX Player::GetWorldTransformMatrix() const
{
	auto T = mPlayerInfo.mMovement.GetWorldTransformInfo();
	XMMATRIX P = XMMatrixTranslation(T.Position.x, T.Position.y, T.Position.z);
	XMMATRIX R = XMLoadFloat4x4(&T.Rotation);
	XMMATRIX S = XMMatrixScaling(T.Scale.x, T.Scale.y, T.Scale.z);

	return  S * R * P;
}

UINT Player::GetAllRitemsSize() const
{
	return (UINT)mAllRitems.size();
}

const std::vector<RenderItem*> Player::GetRenderItem(RenderLayer Type) const
{
	return mRitems[(int)Type];
}


void Player::SetClipName(const std::string& inClipName)
{
	if (mPlayerInfo.mClipName != "Death")
	{
		mPlayerInfo.mClipName = inClipName;
	}
}

void Player::SetClipTime(float time)
{
	mSkinnedModelInst->TimePos = time;
}


void Player::BuildGeometry(
	ID3D12Device * device,
	ID3D12GraphicsCommandList* cmdList,
	const std::vector<CharacterVertex>& inVertices,
	const std::vector<std::uint32_t>& inIndices,
	const SkinnedData& inSkinInfo,
	std::string geoName)
{
	mSkinnedInfo = inSkinInfo;

	mSkinnedModelInst = std::make_unique<SkinnedModelInstance>();
	mSkinnedModelInst->SkinnedInfo = &mSkinnedInfo;
	mSkinnedModelInst->FinalTransforms.resize(mSkinnedInfo.BoneCount());
	mSkinnedModelInst->TimePos = 0.0f;

	Character::BuildGeometry(
		device, cmdList,
		inVertices, inIndices,
		mSkinnedInfo, geoName);
}

void Player::BuildRenderItem(
	Materials& mMaterials,
	std::string matrialPrefix)
{
	int playerIndex = 0;
	int boneCount = (UINT)mSkinnedInfo.BoneCount();
	auto vBoneName = mSkinnedInfo.GetBoneName();

	// Character Mesh
	for (int submeshIndex = 0; submeshIndex < boneCount - 1; ++submeshIndex)
	{
		std::string SubmeshName = vBoneName[submeshIndex];

		auto PlayerRitem = std::make_unique<RenderItem>();
		//XMStoreFloat4x4(&PlayerRitem->World, XMMatrixScaling(4.0f, 4.0f, 4.0f));
		//XMStoreFloat4x4(&PlayerRitem->World, XMMatrixScaling(40.0f, 40.0f, 40.0f));
		XMStoreFloat4x4(&PlayerRitem->World, XMMatrixScaling(40.0f, 40.0f, 40.0f) * XMMatrixRotationX(-MathHelper::Pi / 2) * XMMatrixTranslation(0.0f, 6.0f, 0.0f) );
		// scaling * translation * rotation;
		PlayerRitem->TexTransform = MathHelper::Identity4x4();
		PlayerRitem->Mat = mMaterials.Get(matrialPrefix);
		PlayerRitem->Geo = GetMeshGeometry();
		PlayerRitem->NumFramesDirty = gNumFrameResources;
		PlayerRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		PlayerRitem->StartIndexLocation = PlayerRitem->Geo->DrawArgs[SubmeshName].StartIndexLocation;// 0, 9, ..., 4935,
		PlayerRitem->BaseVertexLocation = PlayerRitem->Geo->DrawArgs[SubmeshName].BaseVertexLocation;// 0, 0, 
		PlayerRitem->IndexCount = PlayerRitem->Geo->DrawArgs[SubmeshName].IndexCount;// 9, 2151, ....,1254, 
		PlayerRitem->SkinnedModelInst = mSkinnedModelInst.get();
		PlayerRitem->PlayerCBIndex = playerIndex++;// 0, 1, 2, 3, 4, 5, 6,.....

		/*auto ShadowedRitem = std::make_unique<RenderItem>();
		*ShadowedRitem = *PlayerRitem;
		ShadowedRitem->Mat = mMaterials.Get("shadow0");
		ShadowedRitem->NumFramesDirty = gNumFrameResources;
		ShadowedRitem->SkinnedModelInst = mSkinnedModelInst.get();
		ShadowedRitem->PlayerCBIndex = playerIndex++;*/

		mRitems[(int)RenderLayer::Character].push_back(PlayerRitem.get());
		mAllRitems.push_back(std::move(PlayerRitem));
		//mRitems[(int)RenderLayer::Shadow].push_back(ShadowedRitem.get());
		//mAllRitems.push_back(std::move(ShadowedRitem));
	}
}


void Player::UpdateCharacterCBs(
	FrameResource* mCurrFrameResource,
	const Light& mMainLight,
	float* Delay,
	const GameTimer & gt)
{
	bool needMoveAround = true;

	if (needMoveAround)
	{
		XMVECTOR pos = XMLoadFloat3(&videoCharacterPos);
		MoveAround(pos, 0.5, gt);
	}
	
	if (mPlayerInfo.mHealth <= 0 && mPlayerInfo.mClipName != "Death")
	{
		SetClipName("Death");
		mSkinnedModelInst->TimePos = 0.0f;
	}
	mSkinnedModelInst->UpdateSkinnedAnimation(mPlayerInfo.mClipName, gt.DeltaTime());

	auto currPlayerCB = mCurrFrameResource->PlayerCB.get();
	for (auto& e : mRitems[(int)RenderLayer::Character])
	{

		CharacterConstants skinnedConstants;

		std::copy(
			std::begin(mSkinnedModelInst->FinalTransforms),
			std::end(mSkinnedModelInst->FinalTransforms),
			&skinnedConstants.BoneTransforms[0]);

		XMMATRIX world = XMLoadFloat4x4(&e->World) * GetWorldTransformMatrix();
		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

		XMStoreFloat4x4(&skinnedConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&skinnedConstants.TexTransform, XMMatrixTranspose(texTransform));

		currPlayerCB->CopyData(e->PlayerCBIndex, skinnedConstants);
	}

	//mInitBoundsBox.Transform(mPlayerInfo.mBoundingBox, GetWorldTransformMatrix());
	GetBoundingBox().Transform(mPlayerInfo.mBoundingBox, GetWorldTransformMatrix());

	/*
	UpdateCharacterShadows(mMainLight);// ¸üÐÂÒõÓ°
	for (auto& e : mRitems[(int)RenderLayer::Shadow])
	{

		CharacterConstants skinnedConstants;

		std::copy(
			std::begin(mSkinnedModelInst->FinalTransforms),
			std::end(mSkinnedModelInst->FinalTransforms),
			&skinnedConstants.BoneTransforms[0]);

		XMMATRIX world = XMLoadFloat4x4(&e->World);
		XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

		XMStoreFloat4x4(&skinnedConstants.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&skinnedConstants.TexTransform, XMMatrixTranspose(texTransform));

		currPlayerCB->CopyData(e->PlayerCBIndex, skinnedConstants);
	}
	*/

	mCamera.UpdateViewMatrix();

	XMVECTOR translation = 0.9 * XMVectorSubtract(mCamera.GetEyePosition(), mPlayerInfo.mMovement.GetPlayerPosition());
	mUI.SetPosition(translation);

	// UI
	auto curUICB = mCurrFrameResource->UICB.get();
	mUI.UpdateUICBs(
		curUICB,
		GetWorldTransformMatrix(),
		-mPlayerInfo.mMovement.GetPlayerRight(),
		Delay,
		mTransformDirty);
}

void Player::UpdateCharacterShadows(const Light& mMainLight)
{
	int i = 0;
	for (auto& e : mRitems[(int)RenderLayer::Shadow])
	{
		// Load the object world
		auto& o = mRitems[(int)RenderLayer::Character][i];

		XMMATRIX shadowWorld = XMLoadFloat4x4(&o->World) * GetWorldTransformMatrix();
		XMVECTOR shadowPlane = XMVectorSet(0.0f, 0.1f, 0.0f, 0.0f);
		XMVECTOR toMainLight = -XMLoadFloat3(&mMainLight.Direction);
		XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
		XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
		XMStoreFloat4x4(&e->World, shadowWorld * S * shadowOffsetY);

		++i;
	}
}

void Player::UpdatePlayerPosition(ePlayerMoveList moveName, float velocity)
{

	switch (moveName)
	{
	case ePlayerMoveList::Walk:
		mPlayerInfo.mMovement.Walk(velocity);
		break;

	case ePlayerMoveList::SideWalk:
		mPlayerInfo.mMovement.SideWalk(velocity);
		break;

	case ePlayerMoveList::AddYaw:
		mPlayerInfo.mMovement.AddYaw(velocity);
		break;

	case ePlayerMoveList::AddPitch:
		mPlayerInfo.mMovement.AddPitch(velocity);
		break;
	case ePlayerMoveList::Death:

		break;
	}

	mTransformDirty = true;
}

//void Player::MoveAround(XMVECTOR centerPos, int axis, float radius, float velocity)
//{
//	//ChracterMovement::SetPlayerPosition()
//		//ChracterMovement::SetPlayerRotation()
//
//
//}

void Player::MoveAround(DirectX::XMVECTOR centerPos, float velocity)
{



}

void Player::MoveAround(DirectX::XMVECTOR centerPos, float velocity, const GameTimer& gt)
{
	

	if (aroundRadius < 0)
	{
		XMVECTOR playerPos = XMLoadFloat3(&originPlayerPosition);
		XMVECTOR xtDistance = playerPos - centerPos;
		XMVECTOR distance = DirectX::XMVector3Length(xtDistance);
		XMFLOAT3 distanceF = { 0.0f, 0.0f, 0.0f };
		DirectX::XMStoreFloat3(&distanceF, distance);
		aroundRadius = distanceF.x;
		lastTime = curTime = passedTime = gt.TotalTime();
	}

	curTime = passedTime = gt.TotalTime();
	float deltaTime1 = gt.DeltaTime();
	deltaTime = curTime - lastTime;

	float posX = aroundRadius * std::sin(passedTime * velocity) + centerPos.m128_f32[0];
	//float posY = aroundRadius * std::cos(passedTime) + centerPos.m128_f32[1];

	//float posY = aroundRadius * std::sin(passedTime) + centerPos.m128_f32[1];
	float posZ = aroundRadius * std::cos(passedTime * velocity) + centerPos.m128_f32[1];

	//std::cout << "Player::MoveAround() : passedTime == " << passedTime << ", deltaTime1 == " << deltaTime1 << ", deltaTime == " << deltaTime << endl;

	/*std::wstring text =
		L"Player::MoveAround() : curTime == passedTime == passedTime == " + std::to_wstring(passedTime) +
		L", gt.DeltaTime() = " + std::to_wstring(gt.DeltaTime()) +
		L", lastTime = " + std::to_wstring(lastTime) +
		L", curTime - lastTime = " + std::to_wstring(deltaTime) +
		L"\n";

	::OutputDebugString(text.c_str());*/
	

	//XMVECTOR P = XMVectorSet(posX, posY, 0.0f, 0.0f);
	XMVECTOR P = XMVectorSet(posX, 0.0f, posZ, 0.0f);
	mPlayerInfo.mMovement.SetPlayerPosition(P);
	mPlayerInfo.mMovement.AddYaw(deltaTime * velocity);
	
	lastTime = curTime;

}

void Player::UpdateTransformationMatrix()
{
	mPlayerInfo.mMovement.UpdateTransformationMatrix();
}