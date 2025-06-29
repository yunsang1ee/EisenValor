#pragma once

struct Mat4 {
#pragma region Constructor

#pragma endregion

#pragma region Operator

#pragma endregion

#pragma region Utility

#pragma endregion

#pragma region Static

#pragma endregion
	float m[4][4] = {};

	Mat4() = default;

	// 행 우선/열 우선 모두 지원하려면 추가 생성자 작성 가능
	Mat4(const float* arr16) {
		for (int i = 0; i < 16; ++i)
			m[i / 4][i % 4] = arr16[i];
	}

	// DirectXMath 변환자 (XMMATRIX <-> Mat4)
	Mat4(const DirectX::XMFLOAT4X4& mat) {
		for (int i = 0; i < 16; ++i)
			m[i / 4][i % 4] = ((float*)&mat)[i];
	}
	operator DirectX::XMFLOAT4X4() const {
		DirectX::XMFLOAT4X4 mat;
		for (int i = 0; i < 16; ++i)
			((float*)&mat)[i] = m[i / 4][i % 4];
		return mat;
	}
};
