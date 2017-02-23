#pragma once
//=========================================================
//
// File: GameObject.h
//
//
//									2017/02/23
//										Author Shinya Ueba
//=========================================================


//=========================================================
//
// Class: CGameObject 全てのオブジェクトの基底クラス
//
//---------------------------------------------------------
class CGameObject
{
public:
	/**
	* @desc コンストラク
	*/
	CGameObject();

	/**
	* @desc デストラクタ
	*/
	~CGameObject();

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