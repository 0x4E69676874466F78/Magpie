#pragma once
#include "pch.h"


// ���п�����Ļ����Ⱦ����Ļ���
class Renderable {
public:
	Renderable() {
	}

	virtual ~Renderable() {}

	virtual void Render() = 0;
};
