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
// Class: CGameObject �S�ẴI�u�W�F�N�g�̊��N���X
//
//---------------------------------------------------------
class CGameObject
{
public:
	/**
	* @desc �R���X�g���N
	*/
	CGameObject();

	/**
	* @desc �f�X�g���N�^
	*/
	~CGameObject();

	/**
	* @desc�@����������
	* @return true...���� false...�I��
	*/
	virtual bool Initialize(void);

	/**
	* @desc �X�V����
	*/
	virtual void Update(void);

	/**
	* @desc �I������
	*/
	virtual void Finalize(void);
};


//EOF