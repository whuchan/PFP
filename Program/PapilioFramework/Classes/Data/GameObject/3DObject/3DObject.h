#pragma once
//=========================================================
//
// File: 3DObject.h
//
//
//									2017/02/23
//										Author Shinya Ueba
//=========================================================

//---------------------------------------------------------
// ヘッダーインクルード
//---------------------------------------------------------
#include"../GameObject.h"



//=========================================================
//
// Class: C3DObject 3Dオブジェクトの基底クラス
//
//---------------------------------------------------------
class C3DObject : public CGameObject
{
public:
	/**
	* @desc コンストラク
	*/
	C3DObject();

	/**
	* @desc デストラクタ
	*/
	virtual ~C3DObject();

	/**
	* @desc　初期化処理
	* @return true...成功 false...終了
	*/
	virtual bool Initialize(void);

	/**
	* @desc 更新処理
	*/
	virtual void Update(void);

	/**
	* @desc 終了処理
	*/
	virtual void Finalize(void);
};


//EOF