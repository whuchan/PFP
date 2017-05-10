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
// �w�b�_�[�C���N���[�h
//---------------------------------------------------------
#include"../GameObject.h"



//=========================================================
//
// Class: C3DObject 3D�I�u�W�F�N�g�̊��N���X
//
//---------------------------------------------------------
class C3DObject : public CGameObject
{
public:
	/**
	* @desc �R���X�g���N
	*/
	C3DObject();

	/**
	* @desc �f�X�g���N�^
	*/
	virtual ~C3DObject();

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